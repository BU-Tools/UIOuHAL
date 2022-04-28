#ifndef __UIO_LABEL_FINDER_HH__
#define __UIO_LABEL_FINDER_HH__

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


//=======================================================
//In ProtocolUIO_io.cpp
//=======================================================

  enum RET_CODE {OK = 0, 
		 BAD_FD = -1, 
		 BAD_MAP = -2,
		 BAD_UIO_SYMLINK = -3,
		 UIO_DEVICE_NOT_FOUND = -4,
		 UIO_DEVICE_NO_MEMMAP = -5,
  };

  RET_CODE openDevice  (uioaxi::sUIODevice & dev);
  RET_CODE checkDevice (uioaxi::sUIODevice & dev);    
  
  RET_CODE findUIO(std::string nodeId, uint32_t nodeAddress);
  RET_CODE findUIO_symlink(std::string nodeId);
  RET_CODE findUIO_devtree(std::string nodeId);

  uint64_t SearchDeviceTree(std::string const & dvtPath,
			    std::string const & name);
}
#endif
