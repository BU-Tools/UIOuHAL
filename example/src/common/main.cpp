#include <iostream>
#include <memory>

#include "uhal/uhal.hpp"

int main(int argc, char* argv[])
{
  // Check if the register name is given as argument
  if (argc != 2) {
    std::cout << "Incorrect number of arguments, should be exactly one!" << std::endl;
    std::cout << "Usage: ./main <registerName>" << std::endl;
    return 1;
  }

  // Get the register name to read from the command line
  const std::string registerName = argv[1];

  // Connection file to use
  std::string connectionFile("file:///home/cms/alp/UIOuHAL/example/address_table/connections.xml");
  std::string connectionFileEntry("test.0");

  std::cout << "Using connection file: " << connectionFile << std::endl;

  // Get the connection manager
  uhal::ConnectionManager manager( connectionFile.c_str(), std::vector<std::string>(1,"uioaxi-1.0") ); 
  std::cout << "Constructed ConnectionManager" << std::endl;

  // Get device interface from connection manager
  std::cout << "Getting HW interface" << std::endl;
  uhal::HwInterface * hw;
  hw = new uhal::HwInterface(manager.getDevice ( connectionFileEntry.c_str() ));
  std::cout << "Got the HW interface" << std::endl;

  // Now, read the register
  // std::string registerName("PL_MEM.ARM.CPU_LOAD");
  std::cout << "Trying to read register: " << registerName << std::endl;
  uhal::ValWord<uint32_t> ret;

  try {
    ret = hw->getNode(registerName).read();
    hw->dispatch();
    std::cout << "Succesfully read register" << std::endl;
    std::cout << "Value: 0x" << std::hex << ret.value() << std::endl; 
  }
  catch (uhal::exception::exception & e) {
    std::string errorMessage(e.what());
    std::cout << "UHAL error thrown: " << errorMessage << std::endl;
    return 2;
  }
  catch (std::exception & e) {
    std::string errorMessage(e.what());
    std::cout << "Generic error thrown: " << errorMessage << std::endl;
    return 3;
  }

  return 0;
}

