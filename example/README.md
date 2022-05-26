## Example Code To Read Apollo Registers via UIOuHAL

This directory contains an example source code to read several registers from the Apollo (tested on the SM02 blade, IP address: 192.168.30.22). More specifically, the `main_for_tom` executable will set up two hardware interfaces, one with `UIOuHAL` and one with `MemMap`, and will try to read one register with each protocol. This is intended to reproduce a segmentation fault with the `MemMap` reading.

### Building & Setting Up the Environment 

The code can be built via `make`:

```
# Builds the executables under bin/
make

# To clean build products
make clean
```

After executing `make`, the executables will be located under `bin` directory. There should be two of them, `main` and `main_for_tom`. `main_for_tom` is the latest example code and the one that should be used.

Before running the `bin/main_for_tom` executable, the `LD_LIBRARY_PATH` must be set so that UIOuHAL library located in `/opt/UIOuHAL` is within the search path.  This script also sets `UHAL_ENABLE_IPBUS_MMAP=1` so that the MemMap protocol can be used as well. This script can be executed as follows:

```
source set_env.sh
```

### Doing the Reads

The `main_for_tom` executable will try to do two things:
- Set up a `UIOuHAL` hardware interface and do a read of `PL_MEM.ARM.CPU_LOAD` node, intended to produce a successful read
- Set up a `MemMap` hardware interface and do a read of `info.magic` node, which should result in a segmentation fault

```
cd bin
./main_for_tom
```

### Getting the Stack Trace

By default, only a `Segmentation fault` message will be displayed. To get more information, user needs to specify that the core should be dumped when a seg-fault happens, and the core file can be examined for the stack trace.

The following workflow is tested to work to get the core dump and generate the stack trace:

```bash
# Dump the core if a seg-fault happens
ulimit -c unlimited

# Run the program as usual
cd bin/
./main_for_tom

# A core file should be there after the seg-fault, run gdb to get the stack trace
gdb main_for_tom <name of the core file>
```

