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

#ifndef __PROTOCOL_UIO_HH__
#define __PROTOCOL_UIO_HH__

#include <uhal/ClientInterface.hpp>
#include <uhal/ValMem.hpp>
#include "uhal/log/exception.hpp"
#include <signal.h> //for handling of SIG_BUS signals

/*
  The kernel patch would allow the device-tree property "linux,uio-name" to override the default label of uio devices.
  The patch creates a symlink from /dev/uio_NAME -> /dev/uioN
*/
#define uio_prefix "uio_"

namespace uioaxi {


  struct sUIODevice{
    sUIODevice();
    ~sUIODevice();
    int fd;
    uint32_t volatile * hw;
    uint64_t addr;
    uint32_t uhalAddr;
    size_t   size;
    std::string uioName;
    std::string hwNodeName;
  };
}

namespace uhal {

  namespace exception
  {
    UHAL_DEFINE_EXCEPTION_CLASS ( UnmatchedLabel , "Exception class to handle the case where matching a label to a device failed." )
    UHAL_DEFINE_EXCEPTION_CLASS ( BadUIODevice , "Exception class to handle the case where uio device cannot be opened." )
    UHAL_DEFINE_EXCEPTION_CLASS ( UnimplementedFunction , "Exception class to handle the case where an unimplemented function is called." )
    UHAL_DEFINE_EXCEPTION_CLASS ( UIOBusError , "Exception class for when an axi transaction causes a BUS_ERROR." )
    UHAL_DEFINE_EXCEPTION_CLASS ( UIODevOOR , "Exception class for when a transaction would be out of mapped range." )
    UHAL_DEFINE_EXCEPTION_CLASS ( UIOMISSING , "No UIO endpoints found. Endpoints must be labeled with fwinfo=\"uio_endpoint\".  Are you using an old style address table?" )
  }

  class UIO : public ClientInterface {
  public:

    //In ProtocolUIO.cpp
    UIO (
	 const std::string& aId, const URI& aUri,
	 const boost::posix_time::time_duration&aTimeoutPeriod =
	 boost::posix_time::milliseconds(10)
	 );
    virtual ~UIO ();


  private:

    // In ProtocolUIO_reg_access
    ValHeader implementWrite (const uint32_t& aAddr, const uint32_t& aValue);
    ValWord<uint32_t> implementRead (const uint32_t& aAddr,
				     const uint32_t& aMask = defs::NOMASK);
#if UHAL_VER_MAJOR >= 2 && UHAL_VER_MINOR >= 8
    void implementDispatch (std::shared_ptr<Buffers> aBuffers) override;
#else
    void implementDispatch (boost::shared_ptr<Buffers> aBuffers) /*override*/ ;
#endif

    ValHeader implementBOT();
    ValHeader implementWriteBlock (const uint32_t& aAddr, const std::vector<uint32_t>& aValues, const defs::BlockReadWriteMode& aMode=defs::INCREMENTAL);
    ValVector< uint32_t > implementReadBlock ( const uint32_t& aAddr, const uint32_t& aSize, const defs::BlockReadWriteMode& aMode=defs::INCREMENTAL );
    ValWord< uint32_t > implementRMWbits ( const uint32_t& aAddr , const uint32_t& aANDterm , const uint32_t& aORterm );
    ValWord< uint32_t > implementRMWsum ( const uint32_t& aAddr , const int32_t& aAddend );
    uint32_t getMaxNumberOfBuffers() {return 0;}
    uint32_t getMaxSendSize() {return 0;}
    uint32_t getMaxReplySize() {return 0;}
    exception::exception* validate ( uint8_t* aSendBufferStart ,
				     uint8_t* aSendBufferEnd ,
				     std::deque< std::pair< uint8_t* , uint32_t > >::iterator aReplyStartIt ,
				     std::deque< std::pair< uint8_t* , uint32_t > >::iterator aReplyEndIt );
    //Local store of valwords for dispatch (legacy from uHAL being IP based)
    std::vector< ValWord<uint32_t> > valwords;
    void primeDispatch ();

    //structs for handling Bus errors
    void SetupSignalHandler();
    void RemoveSignalHandler();
    struct sigaction saBusError;
    struct sigaction saBusError_old;

  private:

    //UHAL to UIO mappings
    std::map<uint32_t,uioaxi::sUIODevice> devices;

    //=======================================================
    //In ProtocolUIO_io.cpp
    //=======================================================
    void openDevice  (uioaxi::sUIODevice & dev);
    int  checkDevice (uioaxi::sUIODevice & dev);    
    int  symlinkFindUIO(std::string nodeId, uint32_t nodeAddress);
    void dtFindUIO     (std::string nodeId, uint32_t nodeAddress);
    uint64_t SearchDeviceTree(std::string const & dvtPath,
			      std::string const & name);
  };

}
#endif
