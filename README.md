# UIOuHAL library
Library for using UIO mapped AXI slaves via uHAL

This links against https://github.com/ipbus/ipbus-software (branch feature/external-client-registration)
Depending on your build enviornment you will need to set IPBUS_PATH or CACTUS_ROOT to point to the built ipbus-software location. 

This library now REQUIRES uHAL `2.8.6` or greater to build and to properly handle any SIG_BUG signals from the memory mapped interface.

Usage:

To use this in code, you need to do the following to properly setup the signal handling. 
`uhal::SigBusGuard::blockSIGBUS();`

The exeception that is thrown when a bus error happens has changed from `uhal::exception::UIOBusError` to `uhal::exception::SigBusError` now that this is handled more generally by the ipbus software.

Since bus errors will happen when remote FPGAs are not yet programmed, it is recommended to set `uhal::setLogLevelTo(uhal::Fatal());` to reduce the number of log messages printed to the screen.




