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


namespace uhal {  

  uint64_t UIO::SearchDeviceTree(std::string const & dvtPath,
				 std::string const & name) {
    uint64_t address = 0;
    FILE *labelfile=0; 
    char label[128];
    // traverse through the device-tree    
    for (directory_iterator x(dvtPath); x!=directory_iterator(); ++x) {
      if (!is_directory(x->path()) || !exists(x->path()/"label")) {
	      continue;
      }
      labelfile = fopen((x->path().native()+"/label").c_str(),"r");
      fgets(label,128,labelfile); 
      fclose(labelfile);

      if(!strcmp(label, name.c_str())) {
	      //Get endpoint AXI address from path
	      // looks something like LABEL@DEADBEEFXX
	      std::string stringAddr=x->path().filename().native();        

	      //Check if we find the @
	      size_t addrStart = stringAddr.find("@");
	      if (addrStart == std::string::npos) {
	        log ( Debug() , "directory name ", x->path().filename().native().c_str() ," has incorrect format. Missing \"@\" " );
	        break; //expect the name to be in x@xxxxxxxx format for example myReg@0x41200000
        }

        //Convert the found string into binary
        if (addrStart+1 > stringAddr.size()) {
          log ( Debug() , "directory name ", x->path().filename().native().c_str() ," has incorrect format. Missing size " );
          break; //expect the name to be in x@xxxxxxxx format for example myReg@0x41200000
        }

	      stringAddr = stringAddr.substr(addrStart+1);

        //Get the names's address from the path (in hex)
        address = std::strtoull(stringAddr.c_str() , 0, 16);
        break;
      }
    }
    return address;
  }

  int UIO::symlinkFindUIO(std::string nodeId) {
    // check if debug mode is enabled
    char* UIOUHAL_DEBUG = getenv("UIOUHAL_DEBUG");
    int size = 0;
    std::string uioName = "";
    char sizechar[128]="", addrchar[128]="";
    uint64_t address = 0;
    // uio name set by the "linux,uio-name" device-tree property -> ex: "uio_K_C2C_PHY"
    std::string prefix = "/dev/";
    uioName = std::string(uio_prefix) + nodeId;
    // first check if /dev/uio_name exists and if so get the uio device file it points to: /dev/uio_NAME -> /dev/uioN
    std::string deviceFile;
    if (NULL != UIOUHAL_DEBUG) {
      printf("searching for /dev/%s symlink\n", uioName.c_str());
    }
    for (directory_iterator itUIO(prefix); itUIO != directory_iterator(); ++itUIO) {
      if ((is_directory(itUIO->path())) || (itUIO->path().string().find(uioName) == std::string::npos)) {
        continue;
      }
      else {
        // found /dev/uio_name, now resolve symlink
        if (is_symlink(itUIO->path())) {
          deviceFile = read_symlink(itUIO->path()).string();
        }
        else {
          if (NULL != UIOUHAL_DEBUG) {
            printf("unable to resolve symlink /dev/%s -> /dev/uioN, using legacy method", uioName.c_str());
          }
          log (Debug(), "Symlink ", prefix, uioName, " could not be resolved.");
          return 0;
        }
      }
    }
    // at this point we can simply grab the proper uio from /sys/class/uio/uioN
    FILE *addrfile=0;
    FILE *sizefile=0;
    std::string uiopath = "/sys/class/uio/";

    addrfile = fopen((uiopath + deviceFile + "/maps/map0/addr").c_str(), "r");
    if (addrfile != NULL) {
      fgets(addrchar, 128, addrfile);
      fclose(addrfile);
    }
    else {
      // try longer method
      if (NULL != UIOUHAL_DEBUG) {
        printf("Simple UIO finding method could not find address file at %s, using legacy method\n", (uiopath+deviceFile+"/maps/map0/addr").c_str());
      }
      log(Debug(), "Simple UIO finding method could not find address file at ", (uiopath + deviceFile + "/maps/map0/addr").c_str());
      return 0;
    }
    // get address
    address = std::strtoull(addrchar, 0, 16);
    sizefile = fopen((uiopath + deviceFile + "/maps/map0/size").c_str(), "r");
    if (sizefile != NULL) {
      fgets(sizechar, 128, sizefile);
      fclose(sizefile);
    }
    else {
      // try longer method
      if (NULL != UIOUHAL_DEBUG) {
        printf("Simple UIO finding method could not find size file at %s, using legacy method\n", (uiopath + deviceFile + "/maps/map0/size").c_str());
      }
      log(Debug(), "Simple UIO finding method could not find size file at ", (uiopath + deviceFile + "/maps/map0/size").c_str());
      return 0;
    }
    size = std::strtoul(sizechar, 0, 16)/4;

    // check size
    if (!size) {
      if (NULL != UIOUHAL_DEBUG) {
        printf("Errror: Simple UIO finding method could load device %s, cannot find device or size 0. Using legacy method instead.\n", nodeId.c_str());
      }
      log(Debug(), "Errror: Simple UIO finding method could load device ", nodeId.c_str(), "cannot find device or size 0");
    }
    // finally, save the mapping
    sUIODevice device;
    device.addr = address;
    device.uioName = uioName;
    device.hwNodeName = nodeId;
    device.size = size;
    devices[address] = device;

    // map the memory
    openDevice(devices[address]);
    

    if (NULL != getenv("UIOUHAL_DEBUG")) {
      printf("Added:\n");
      printf("  addr:     0x%08X\n",devices[address].addr);
      printf("  uio name: \"%s\"\n",devices[address].uioName.c_str());
      printf("  hw  name: \"%s\"\n",devices[address].hwNodeName.c_str());
      printf("  size:     0x%08X\n",devices[address].size);
      printf("  map:      %p\n"    ,devices[address].hw);
    }

    //Check that the device (will throw if it is bad)
    checkDevice(devices[address]);

    
    return 1;
  }

  void UIO::dtFindUIO( std::string nodeId) {
    if (NULL != getenv("UIOUHAL_DEBUG")) {
      printf("Using legacy method for UIO device mapping: %s\n", nodeId.c_str());
    }
    // copied from Siqi's original code
    int size = 0;
    std::string uioName;
    char sizechar[128]="", addrchar[128]="";
    uint64_t address1 = 0, address2 = 0;

    // iterate thru filesys to get the matching uio device 
    std::string uiopath = "/sys/class/uio/";
    std::string dvtpath = "/proc/device-tree/";
    FILE *addrfile=0;
    FILE *sizefile=0;
    // loop over all amba, amba_pl paths
    for (directory_iterator itDVTPath(dvtpath); itDVTPath!=directory_iterator(); ++itDVTPath) {
      //Check that this is a path with amba in its name
      if ((!is_directory(itDVTPath->path())) || (itDVTPath->path().string().find("amba")==std::string::npos)) {
        continue;
      }
      else {
        address1=SearchDeviceTree(itDVTPath->path().string(),(nodeId));
        if (address1 != 0) {
          //we found the correct entry
          break;
        }
      }
    }
    //check if we found anything
    if(address1==0) log (Debug(), "Cannot find a device that matches label ", (nodeId).c_str(), " device not opened!" );
    // Traverse through the /sys/class/uio directory
    for (directory_iterator x(uiopath); x!=directory_iterator(); ++x) {
      if (!is_directory(x->path())) {
        continue;
      }
      if (!exists(x->path()/"maps/map0/addr")) {
        continue;
      }
      if (!exists(x->path()/"maps/map0/size")) {
        continue;
      }
      addrfile = fopen((x->path()/"maps/map0/addr").native().c_str(),"r");
      fgets(addrchar,128,addrfile); 
      fclose(addrfile);

      address2 = std::strtoull( addrchar, 0, 16);
      if (address1 == address2) {
        sizefile = fopen((x->path().native()+"/maps/map0/size").c_str(),"r");
        fgets(sizechar,128,sizefile); fclose(sizefile);
        //the size was in number of bytes, convert into number of uint32
        size=std::strtoul( sizechar, 0, 16)/4;  
        //strcpy(uioName,x->path().filename().native().c_str());
        uioName = x->path().filename().native();
        break;
      }
    }

    // finally, save the mapping
    sUIODevice device;
    device.addr = address1;
    device.uioName = uioName;
    device.hwNodeName = nodeId;
    device.size = size;
    devices[address1] = device;

    // map the memory
    openDevice(devices[address1]);
    

    if (NULL != getenv("UIOUHAL_DEBUG")) {
      printf("Added:\n");
      printf("  addr:     0x%08X\n",devices[address1].addr);
      printf("  uio name: \"%s\"\n",devices[address1].uioName.c_str());
      printf("  hw  name: \"%s\"\n",devices[address1].hwNodeName.c_str());
      printf("  size:     0x%08X\n",devices[address1].size);
      printf("  map:      %p\n"    ,devices[address1].hw);
    }

    //Check that the device (will throw if it is bad)
    checkDevice(devices[address1]);

  }


  void UIO::openDevice(sUIODevice & dev) {
    std::string devpath = "/dev" + dev.uioName;
    dev.fd = open(devpath.c_str(), O_RDWR|O_SYNC);
    if (-1==dev.fd) {
      uhal::exception::BadUIODevice* lExc = new uhal::exception::BadUIODevice();
      log( *lExc , "Failed to open ", devpath, ": ", strerror(errno));
      throw *lExc;
    }
    dev.hw = (uint32_t*)mmap(NULL, dev.size*sizeof(uint32_t),
			                      PROT_READ|PROT_WRITE, MAP_SHARED,
			                      dev.fd, 0x0);
    if (dev.hw==MAP_FAILED) {
      uhal::exception::BadUIODevice* lExc = new uhal::exception::BadUIODevice();
      log ( Debug() , "Failed to map ", devpath, ": ",  strerror(errno));
      dev.hw=NULL;
      throw lExc;
    }
    log ( Debug(), "Mapped ", devpath,
	  " size ", Integer( dev.size, IntFmt<hex, fixed>()));
    
  }

  int UIO::checkDevice (sUIODevice & dev) {
    if (dev.hw == NULL) {
      // include name of device in log output:
      uhal::exception::BadUIODevice* lExc = new uhal::exception::BadUIODevice();
      log (*lExc , "No mapping for Device: ", dev.hwNodeName);
      throw *lExc;
      return 1;
    }
    return 0;
  }
}
