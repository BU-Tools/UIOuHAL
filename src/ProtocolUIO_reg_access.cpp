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

//This macro handles the possibility of a SIG_BUS signal and property throws an exception
//The command you want to run is passed via ACESS and will be in a if{}else{} block, so
//Call it appropriately. 
// ex
//   old:
//     uint32_t readval = hw[da.device][da.word];
//   new:
//     uint32_t readval;
//     BUS_ERROR_PROTECTION(readval = hw[da.device][da.word])
#define BUS_ERROR_PROTECTION(ACCESS,ADDRESS)					\
  if(SIGBUS == sigsetjmp(env,1)){						\
    uhal::exception::UIOBusError * e = new uhal::exception::UIOBusError();\
    char error_message[] = "Reg: 0x00000000"; \
    snprintf(error_message,strlen(error_message),"Reg: 0x%08X",ADDRESS); \
    e->append(error_message); \
    throw *e;\
  }else{ \
    ACCESS;					\
  }



//Signal handling for sigbus
sigjmp_buf static env;
void static signal_handler(int sig){
  if(SIGBUS == sig){
    siglongjmp(env,sig);    
  }
}

namespace uhal {  

  void UIO::SetupSignalHandler(){
    //this is here so the signal_handler can stay static
    memset(&saBusError,0,sizeof(saBusError)); //Clear struct
    saBusError.sa_handler = signal_handler; //assign signal handler
    sigemptyset(&saBusError.sa_mask);
    sigaction(SIGBUS, &saBusError,&saBusError_old);  //install new signal handler (save the old one)
  }
  void UIO::RemoveSignalHandler(){    
    sigaction(SIGBUS,&saBusError_old,NULL); //restore the signal handler from before creation for SIGBUS
  }

  ValHeader UIO::implementWrite (const uint32_t& aAddr, const uint32_t& aValue) {
    DevAddr da = decodeAddress(aAddr);
    if (checkDevice(da.device)){ return ValWord<uint32_t>();}
    uint32_t writeval = aValue;
    
    hw[da.device][da.word] = writeval;
    return ValHeader();
  }

  ValHeader UIO::implementBOT() {
    log ( Debug() , "Byte Order Transaction");
    uhal::exception::UnimplementedFunction* lExc = new uhal::exception::UnimplementedFunction();
    log (*lExc, "Function implementBOT() is not yet implemented.");
    throw *lExc;
    return ValHeader();
  }

  ValHeader UIO::implementWriteBlock (const uint32_t& aAddr, const std::vector<uint32_t>& aValues, const defs::BlockReadWriteMode& aMode) {
    DevAddr da = decodeAddress(aAddr);
    if (checkDevice(da.device)) return ValWord<uint32_t>();
    uint32_t lAddr = da.word ;
    std::vector<uint32_t>::const_iterator ptr;
    for (ptr = aValues.begin(); ptr < aValues.end(); ptr++) {
      uint32_t writeval = *ptr;
      BUS_ERROR_PROTECTION(hw[da.device][lAddr] = writeval,aAddr)
      if ( aMode == defs::INCREMENTAL ) {
        lAddr ++;
      }
    }
    return ValHeader();
  }

  ValWord<uint32_t> UIO::implementRead (const uint32_t& aAddr, const uint32_t& aMask) {
    DevAddr da = decodeAddress(aAddr);
    if (checkDevice(da.device)) {
      return ValWord<uint32_t>();
    }
    uint32_t readval;
    BUS_ERROR_PROTECTION(readval = hw[da.device][da.word],aAddr)
    ValWord<uint32_t> vw(readval, aMask);
    valwords.push_back(vw);
    primeDispatch();
    return vw;
  }
    
  ValVector< uint32_t > UIO::implementReadBlock (const uint32_t& aAddr, const uint32_t& aSize, const defs::BlockReadWriteMode& aMode) {
    DevAddr da = decodeAddress(aAddr);
    uint32_t lAddr = da.word ;
    if (checkDevice(da.device)) return ValVector<uint32_t>();
    std::vector<uint32_t> read_vector(aSize);
    std::vector<uint32_t>::iterator ptr;
    for (ptr = read_vector.begin(); ptr < read_vector.end(); ptr++) {
      uint32_t readval;
      BUS_ERROR_PROTECTION(readval = hw[da.device][lAddr],aAddr)
      *ptr = readval;
      if ( aMode == defs::INCREMENTAL ) {
	      lAddr ++;
      }
    }
    return ValVector< uint32_t> (read_vector);
  }

  void UIO::primeDispatch () {
    // uhal will never call implementDispatch unless told that buffers are in
    // use (even though the buffers are not actually used and are length zero).
    // implementDispatch will only be called once after each checkBufferSpace.
    uint32_t sendcount = 0, replycount = 0, sendavail, replyavail;
    checkBufferSpace ( sendcount, replycount, sendavail, replyavail);
  }

#if UHAL_VER_MAJOR >= 2 && UHAL_VER_MINOR >= 8
  void UIO::implementDispatch (std::shared_ptr<Buffers> /*aBuffers*/) {
#else
  void UIO::implementDispatch (boost::shared_ptr<Buffers> /*aBuffers*/) {
#endif
    log ( Debug(), "UIO: Dispatch");
    for (unsigned int i=0; i<valwords.size(); i++)
      valwords[i].valid(true);
    valwords.clear();
  }

  ValWord<uint32_t> UIO::implementRMWbits (const uint32_t& aAddr , const uint32_t& aANDterm , const uint32_t& aORterm) {
    DevAddr da = decodeAddress(aAddr);
    if (checkDevice(da.device)) return ValWord<uint32_t>();

    //read the current value
    uint32_t readval;
    BUS_ERROR_PROTECTION(readval = hw[da.device][da.word],aAddr)

    //apply and and or operations
    readval &= aANDterm;
    readval |= aORterm;
    BUS_ERROR_PROTECTION(hw[da.device][da.word] = readval,aAddr)
    BUS_ERROR_PROTECTION(readval = hw[da.device][da.word],aAddr)
    return ValWord<uint32_t>(readval);
  }


  ValWord<uint32_t> UIO::implementRMWsum (const uint32_t& aAddr, const int32_t& aAddend) {
    DevAddr da = decodeAddress(aAddr);
    if (checkDevice(da.device)) {
      return ValWord<uint32_t>();
    }
    //read the current value
    uint32_t readval;
    BUS_ERROR_PROTECTION(readval = hw[da.device][da.word],aAddr)
    //apply and and or operations
    readval += aAddend;
    BUS_ERROR_PROTECTION(hw[da.device][da.word] = readval,aAddr)
    BUS_ERROR_PROTECTION(readval = hw[da.device][da.word],aAddr)
    return ValWord<uint32_t>(readval);
  }

  exception::exception* UIO::validate (uint8_t* /*aSendBufferStart*/,
					uint8_t* /*aSendBufferEnd */,
					std::deque< std::pair< uint8_t* , uint32_t > >::iterator /*aReplyStartIt*/ ,
					std::deque< std::pair< uint8_t* , uint32_t > >::iterator /*aReplyEndIt*/ ) {
    return NULL;
  }
  
}   // namespace uhal

