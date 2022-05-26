#include <iostream>
#include <memory>

#include "uhal/uhal.hpp"

int main(int argc, char* argv[])
{
  // Connection file to use: Path is relative to the bin/main executable
  // This connection file has two set of hardware interfaces:
  // 1. UIOuHAL interface, 2. MemMap interface
  std::string connectionFile("file://../address_table/connections_for_tom.xml");

  std::cout << "Using connection file: " << connectionFile << std::endl;

  // Get the connection manager
  uhal::ConnectionManager manager( connectionFile.c_str(), std::vector<std::string>(1,"uioaxi-1.0") ); 
  std::cout << "Constructed ConnectionManager" << std::endl;

  // Get device interface for UIOuHAL 
  std::cout << "Getting HW interface for UIOuHAL" << std::endl;
  std::string connectionFileEntryUIO("test.0");
  uhal::HwInterface * hw_uio;
  hw_uio = new uhal::HwInterface(manager.getDevice ( connectionFileEntryUIO.c_str() ));
  std::cout << "Got the HW interface for UIOuHAL" << std::endl;

  // Get device interface for MemMap 
  std::cout << "Getting HW interface for MemMap" << std::endl;
  std::string connectionFileEntryMemMap("apollo.c2c.vu7p");
  uhal::HwInterface * hw_mmap;
  hw_mmap = new uhal::HwInterface(manager.getDevice ( connectionFileEntryMemMap.c_str() ));
  std::cout << "Got the HW interface for MemMap" << std::endl;

  // Now, read the registers with both interfaces
  std::string registerName("PL_MEM.ARM.CPU_LOAD");
  std::cout << "Trying to read register: " << registerName << " with UIOuHAL interface" << std::endl;
  uhal::ValWord<uint32_t> ret_uio;

  // UIOuHAL read 
  ret_uio = hw_uio->getNode(registerName).read();
  hw_uio->dispatch();
  std::cout << "Succesfully read register" << std::endl;
  std::cout << "Value: 0x" << std::hex << ret_uio.value() << std::endl; 
  
  std::string registerNameMMap("info.magic");
  std::cout << "Trying to read register: " << registerNameMMap << " with MemMap interface" << std::endl;
  uhal::ValWord<uint32_t> ret_mmap;

  // MemMap read, it should go bad here..
  ret_mmap = hw_mmap->getNode(registerNameMMap).read();
  hw_mmap->dispatch();
  std::cout << "Succesfully read register" << std::endl;
  std::cout << "Value: 0x" << std::hex << ret_mmap.value() << std::endl; 

  return 0;
}

