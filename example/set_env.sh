#!/bin/bash

# Sets the LD_LIBRARY_PATH such that the lib/ of UIOuHAL
# is contained in the LD_LIBRARY_PATH
# Source this file before running bin/main

UIOUHAL_ROOT="/opt/UIOuHAL"

LD_LIBRARY_PATH=${UIOUHAL_ROOT}/lib:${LD_LIBRARY_PATH}

echo "Set the LD_LIBRARY_PATH to: ${LD_LIBRARY_PATH}"

# Set environment variable to enable MMAP protocol
export UHAL_ENABLE_IPBUS_MMAP=1
