####################################################################################
#
# Example Makefile for building a project against the cactus/uhal library
# -----------------------------------------------------------------------
#
# This Makefile assumes you have a standard YUM/RPM cactus install in: /opt/cactus
#
# In your working directory:
#   Your header files (.hpp files) must go into a folder called: include/
#   Your source files (.cpp and .cxx files) must go into a folder called: src/common/
#
# This makefile builds all the .hpp and .cpp files into a single library, and builds
# individual executables from all the files ending in .cxx that are linked against
# the aforementioned library.
#
# Change all references to "MY_PROJECT_ROOT" and "myprojectlibname", to more sensible names.
#
# To give an explicit example of how to set up your code and what will result from using
# this Makefile, an example folder structure might be:
#
#      include/MyFirstClass.hpp
#      include/MySecondClass.hpp
#      include/utils.hpp
#
#      src/common/MyFirstClass.cpp
#      src/common/MySecondClass.cpp
#      src/common/utils.cpp
#      src/common/myExecutable1.cxx
#      src/common/myExecutable2.cxx
#
# This makefile will then make a library of MyFirstClass, MySecondClass and utils, and
# put this library in the lib/ directory.  It will also make two executables called
# myExecutable1 and myExecutable2 and put them in the bin/ directory.
#
# To run your executables, you will need to ensure that the LD_LIBRARY_PATH contains
# both /opt/cactus/lib and the explicit path to the lib/ directory where this makefile
# has built your library.
#
# Robert Frazier
# September 2012
#
####################################################################################


# Set the project home directory to the current directory
MY_PROJECT_ROOT = $(shell pwd)

# The root folder for all the cactus
CACTUS_ROOT = /opt/cactus

# Root folder for the UIOuHAL
UIOUHAL_ROOT = /opt/UIOuHAL

# The include path is your own include directory and the cactus includes
INCLUDE_PATH =  \
                -I${UIOUHAL_ROOT}/include  \
                -I${CACTUS_ROOT}/include 


EXECUTABLE_SOURCES = $(wildcard src/common/*.cpp)
EXECUTABLE_OBJECT_FILES = $(patsubst src/common/%.cpp,obj/%.o,${EXECUTABLE_SOURCES})
EXECUTABLES = $(patsubst src/common/%.cpp,bin/%,${EXECUTABLE_SOURCES})

LIBRARY_PATH =  \
	-L${CACTUS_ROOT}/lib \
	-L${UIOUHAL_ROOT}/lib

LIBRARIES =     \
                -lpthread \
                \
                -lboost_filesystem \
                -lboost_regex \
                -lboost_system \
                -lboost_thread \
                -lpugixml \
                \
                -lcactus_uhal_log \
                -lcactus_uhal_grammars \
                -lcactus_uhal_uhal \
		-lUIOuHAL


CPP_FLAGS = -std=c++11  -O3 -Wall -fPIC -g ${INCLUDE_PATH}
LINK_EXECUTABLE_FLAGS = -Wall -g -O3 ${LIBRARY_PATH} ${LIBRARIES}

.PHONY: all _all build _buildall clean _cleanall

default: build

clean: _cleanall
_cleanall:
	rm -rf bin
	rm -rf obj
  
all: _all
build: _all
buildall: _all
_all: obj bin ${EXECUTABLES}

bin:
	mkdir -p bin

obj:
	mkdir -p obj

${EXECUTABLES}: bin/%: obj/%.o ${EXECUTABLE_OBJECT_FILES}
	g++ ${LINK_EXECUTABLE_FLAGS}  $< -o $@

${EXECUTABLE_OBJECT_FILES}: obj/%.o : src/common/%.cpp
	g++ -c ${CPP_FLAGS}  $< -o $@
