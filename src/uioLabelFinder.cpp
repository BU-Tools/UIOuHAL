#include <uioLabelFinder.hh>


//===============================================================================
// sUIODevice struct
//===============================================================================
uioaxi::sUIODevice::sUIODevice() : 
  fd(-1),
  hw(NULL),
  size(0){
}

uioaxi::sUIODevice::~sUIODevice()
{
  if(NULL != hw) {
    munmap((void *)(hw),size);
  }
  if(fd != -1){
    close(fd);
  }
}  

//===============================================================================
// uioaxi calls
//===============================================================================

//given a uio device file name, open it and memory map it. 
uioaxi::RET_CODE uioaxi::openDevice(sUIODevice & dev) {
  std::string devpath = "/dev/" + dev.uioName;
  dev.hw=NULL;
  dev.fd = open(devpath.c_str(), O_RDWR|O_SYNC);
  if (-1==dev.fd) {
    return RET_CODE::BAD_FD;
  }
  dev.hw = (uint32_t*)mmap(NULL, dev.size*sizeof(uint32_t),
			   PROT_READ|PROT_WRITE, MAP_SHARED,
			   dev.fd, 0x0);
  if (dev.hw==MAP_FAILED) {
    dev.hw=NULL;
    return RET_CODE::BAD_MAP;

  }
  return RET_CODE::OK;
}

//Validate that the uio mappint is non-null
uioaxi::RET_CODE uioaxi::checkDevice (sUIODevice & dev) {
  if (dev.hw == NULL) {
    // include name of device in log output:
    return RET_CODE::BAD_MAP;
  }
  return RET_CODE::OK;
}


//Find the UIO device for a given node
//try both current (symlink) and legacy(devtree) methods for this.
uioaxi::RET_CODE findUIO(std::string nodeId) {
  uioaxi::RET_CODE ret;
  //First try the uio label method
  ret = findUIO_symlink(nodeID);
  if(ret.uioName.size() != RET_CODE::OK ){
    //try the old way
    ret = findUIO_devtree(nodeID);
  }
  return ret;
}


static std::string symlinkSearch(std::string baseName){
  // check if debug mode is enabled
  char* UIOUHAL_DEBUG = getenv("UIOUHAL_DEBUG");

  std::string uioName = "";
  // uio name set by the "linux,uio-name" device-tree property -> ex: "uio_K_C2C_PHY"
  std::string prefix = "/dev/";
  uioName = std::string(uio_prefix) + nodeId;
  // first check if /dev/uio_name exists and if so get the uio device file it points to: /dev/uio_NAME -> /dev/uioN
  std::string deviceFile;
  if (NULL != UIOUHAL_DEBUG) {
    printf("searching for /dev/%s symlink\n", uioName.c_str());
  }
  //loop over files in prefix dir
  for (directory_iterator itUIO(prefix); 
       itUIO != directory_iterator(); 
       ++itUIO) {
    if ((is_directory(itUIO->path())) || 
	(itUIO->path().string().find(uioName) == std::string::npos)) {     
      //file does not contain uioNAME, keep looking
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
	//bail
	return deviceFile;
      }
    }
  }
  return deviceFile;
}

RET_CODE uioaxi::findUIO_symlink(std::string nodeId, uioaxi::sUIODevice & device) {
  // check if debug mode is enabled
  char* UIOUHAL_DEBUG = getenv("UIOUHAL_DEBUG");
  int size = 0;

  std::string buffer(128,'\0');
  memset(buffer,Are
  uint64_t address = 0;
  
  //Search for a symlink with this nodeID
  std::string uioName = symlinkSearch(nodeID);
  //Check that we found something
  if(0 == deviceFile.size()){
    //no symlink found
    return RET_CODE::UIO_DEVICE_NOT_FOUND
  }

  // at this point we can simply grab the proper uio from /sys/class/uio/uioN
  FILE *addrfile=0;
  FILE *sizefile=0;
  std::string uiopath = "/sys/class/uio/";

  //Check for maps in sysfs
  addrfile = fopen((uiopath + deviceFile + "/maps/map0/addr").c_str(), "r");
  if (addrfile != NULL) {
    fgets(addrchar, 128, addrfile); //save address
    address = std::strtoull(addrchar, 0, 16);
    fclose(addrfile);
  } else {
    if (NULL != UIOUHAL_DEBUG) {
      printf("No map found for UIO device: %s\n", (uiopath+deviceFile+"/maps/map0/addr").c_str());
    }
    return RET_CODE::UIO_DEVICE_NO_MEMMAP;
  }
  


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
  device.uhalAddr = nodeAddress;
  device.addr = address;
  device.uioName = uioName;
  device.hwNodeName = nodeId;
  device.size = size;

  return ret;
}
