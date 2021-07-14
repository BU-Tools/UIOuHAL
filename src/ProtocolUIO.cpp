/*
---------------------------------------------------------------------------

    This is an extension of uHAL to directly access AXI slaves via the linux
    UIO driver. 

    This file is part of uHAL.

    uHAL is a hardware access library and programming framework
    originally developed for upgrades of the Level-1 trigger of the CMS
    experiment at CERN.

    uHAL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    uHAL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with uHAL.  If not, see <http://www.gnu.org/licenses/>.


      Andrew Rose, Imperial College, London
      email: awr01 <AT> imperial.ac.uk

      Marc Magrans de Abril, CERN
      email: marc.magrans.de.abril <AT> cern.ch

      Tom Williams, Rutherford Appleton Laboratory, Oxfordshire
      email: tom.williams <AT> cern.ch

      Dan Gastler, Boston University 
      email: dgastler <AT> bu.edu
      
---------------------------------------------------------------------------
*/
/**
	@file
	@author Siqi Yuan / Dan Gastler / Theron Jasper Tarigo
*/

#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include <uhal/Node.hpp>
#include <uhal/NodeTreeBuilder.hpp>
#include <pugixml.hpp>
#include "uhal/log/LogLevels.hpp"
#include "uhal/log/log_inserters.integer.hpp"
#include "uhal/log/log.hpp"

#include "uhal/ClientFactory.hpp" //for runtime linking

#include <ProtocolUIO.hpp>

#include <setjmp.h> //for BUS_ERROR signal handling

#include <inttypes.h> //for PRI macros

using namespace uioaxi;
using namespace boost::filesystem;

UHAL_REGISTER_EXTERNAL_CLIENT(uhal::UIO, "uioaxi-1.0", "Direct access to AXI slave via UIO")


namespace uhal {  


  UIO::UIO (
	    const std::string& aId, const URI& aUri,
	    const boost::posix_time::time_duration&aTimeoutPeriod
	    ) :
    ClientInterface(aId,aUri,aTimeoutPeriod)
  { 
    for (int i=0; i<uioaxi::DEVICES_MAX ; i++){ fd[i]=-1; hw[i]=NULL; sizes[i]=0; }
    // Get the filename of the address table from the connection file. Then read it through the NodeTreeBuilder
    // The NodeTreeBuilder should be able to just use the existing node tree rather than rebuild a new one
    NodeTreeBuilder & mynodetreebuilder = NodeTreeBuilder::getInstance();
    //boost::shared_ptr< Node > lNode ( mynodetreebuilder.getNodeTree ( tabfname , boost::filesystem::current_path() / "." ) );
    Node* lNode = ( mynodetreebuilder.getNodeTree ( std::string("file://")+aUri.mHostname , boost::filesystem::current_path() / "." ) );
    // Getting the IDs for only first layer nodes (nodes that contain device labels). matching names that doesn't contain a "."
    std::vector< std::string > top_node_Ids = lNode->getNodes("^[^.]+$");
    // For each device label, search for its matching device
    for (std::vector<std::string>::iterator nodeId = top_node_Ids.begin(); nodeId != top_node_Ids.end(); ++nodeId) {
      // try the simple method using "linux,uio-name" patch, else use the complex method (iterating thru dirs)
      if (!symlinkFindUIO(lNode, *nodeId)) {
        dtFindUIO(lNode, *nodeId);
      }
    }
  
    //Now that everything created sucessfully, we can deal with signal handling
    SetupSignalHandler();
  }

  UIO::~UIO () {
    //unmap all mmaps
    for (int i=0; i<uioaxi::DEVICES_MAX ; i++) {
	    if(NULL != hw[i]) {
	      munmap((void *)(hw[i]),sizes[i]);
	      hw[i] = NULL;
	      sizes[i] = 0;
	      close(fd[i]);
	      fd[i] = -1;
      }
    }
    log ( Debug() , "UIO: destructor" );
    RemoveSignalHandler();

  }

  DevAddr UIO::decodeAddress (uint32_t uaddr) {
    DevAddr da;
    da.device = (uaddr&ADDR_DEV_MASK)>>ADDR_DEV_OFFSET;
    da.word = (uaddr&ADDR_WORD_MASK);
    return da;
  }
  
}   // namespace uhal

