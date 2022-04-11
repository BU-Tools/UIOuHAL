SHELL=bash
UHAL_VER_MAJOR ?= 2
UHAL_VER_MINOR ?= 8

CXX?=g++

INCLUDE_PATH = \
		-Iinclude  

LIBRARY_PATH = \
		-Llib 

INSTALL_ROOT ?= /opt/UIOuHAL/




ifdef BOOST_INC
INCLUDE_PATH +=-I$(BOOST_INC)
endif
ifdef BOOST_LIB
LIBRARY_PATH +=-L$(BOOST_LIB)
endif

LIBRARIES =    	-lboost_regex \
		-lboost_filesystem


CXX_FLAGS = -std=c++11 -g -O3 -rdynamic -Wall -MMD -MP -fPIC ${INCLUDE_PATH} -Wno-literal-suffix -DUHAL_VER_MAJOR=${UHAL_VER_MAJOR} -DUHAL_VER_MINOR=${UHAL_VER_MINOR}

CXX_FLAGS +=-fno-omit-frame-pointer -Wno-ignored-qualifiers -Werror=return-type -Wextra -Wno-long-long -Winit-self -Wno-unused-local-typedefs  -Woverloaded-virtual ${COMPILETIME_ROOT} ${FALLTHROUGH_FLAGS}

LINK_LIBRARY_FLAGS = -shared -fPIC -Wall -g -O3 -rdynamic ${LIBRARY_PATH} ${LIBRARIES} -Wl,-rpath=$(RUNTIME_LDPATH)/lib ${COMPILETIME_ROOT}


# ------------------------
# IPBUS stuff
# ------------------------
UHAL_LIBRARIES = -lcactus_uhal_log 		\
                 -lcactus_uhal_grammars 	\
                 -lcactus_uhal_uhal 		

# Search uHAL library from $IPBUS_PATH first then from $CACTUS_ROOT
ifdef IPBUS_PATH
UHAL_INCLUDE_PATH = \
			-isystem$(IPBUS_PATH)/uhal/uhal/include \
	         	-isystem$(IPBUS_PATH)/uhal/log/include \
	         	-isystem$(IPBUS_PATH)/uhal/grammars/include 
UHAL_LIBRARY_PATH = \
			-L$(IPBUS_PATH)/uhal/uhal/lib \
	         	-L$(IPBUS_PATH)/uhal/log/lib \
	         	-L$(IPBUS_PATH)/uhal/grammars/lib \
			-L$(IPBUS_PATH)/extern/pugixml/pugixml-1.2/ 
else
UHAL_INCLUDE_PATH = \
	         	-isystem$(CACTUS_ROOT)/include 

UHAL_LIBRARY_PATH = \
			-L$(CACTUS_ROOT)/lib  -Wl,-rpath=$(CACTUS_ROOT)/lib
endif

UHAL_CXX_FLAGS = ${UHAL_INCLUDE_PATH}


UHAL_LIBRARY_FLAGS = ${UHAL_LIBRARY_PATH}



CXX_FLAGS          +=${UHAL_CXX_FLAGS}
LINK_LIBRARY_FLAGS +=${UHAL_LIBRARY_FLAGS}
LIBRARIES          += ${UHAL_LIBRARIES}

.PHONY: all _all clean _cleanall build _buildall _cactus_env

default: build
clean: _cleanall
_cleanall:
	rm -rf obj
	rm -rf lib


all: _all
build: _all
buildall: _all
_all: _cactus_env lib/libUIOuHAL.so


_cactus_env:
ifdef IPBUS_PATH
	$(info using uHAL lib from user defined IPBUS_PATH=${IPBUS_PATH})
else ifdef CACTUS_ROOT
	$(info using uHAL lib from user defined CACTUS_ROOT=${CACTUS_ROOT})
else
	$(error Must define IPBUS_PATH or CACTUS_ROOT to include uHAL libraries (define through Makefile or command line\))
endif



lib/libUIOuHAL.so : obj/ProtocolUIO.o obj/ProtocolUIO_io.o obj/ProtocolUIO_reg_access.o 
	mkdir -p lib
	${CXX} ${LINK_LIBRARY_FLAGS}  $^ -o $@

obj/%.o : src/%.cpp
	mkdir -p obj
	${CXX} ${CXX_FLAGS} -c $^ -o $@

install: lib/libUIOuHAL.so
	@cp -r lib     ${INSTALL_ROOT}
	@cp -r include ${INSTALL_ROOT}
