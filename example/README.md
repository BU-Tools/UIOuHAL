## Example Code To Read Apollo Registers via UIUuHAL

This directory contains an example source code to read several registers from the Apollo (tested on the SM02 blade). The simplified address table contains two top-level nodes:

- `CM_V_INFO`: Information related to programming of Virtex FPGA. Intended to reproduce a `BUS_ERROR`, when the Virtex FPGA is not properly programmed.
- `PL_MEM`: Memory and CPU usage information. Intended to reproduce a successful read.

### Setting up the environment 

First, the `LD_LIBRARY_PATH` must be set so that uHAL libraries in `/opt/cactus` are within the search path. This can be done via:

```
source set_env.sh
```

Then the code can be built via `make`:

```
# Builds the executable under bin
make

# To clean build products
make clean
```

After executing `make`, the resulting executable is located in the following path: `bin/main`.

### Doing the reads

The reads can be done by invoking the `main` executable, together with an argument specifying the register name to read. For the purpose of this example, user can use the following two registers:

- `PL_MEM.ARM.CPU_LOAD`: Will read correctly.
- `CM_V_INFO.BUILD_TIME.SEC`: Will cause a `BUS_ERROR` due to Virtex FPGA not being programmed.

The executable can be invoked as such:

```
# Note that if number of arguments is different than one, the script will exit
./bin/main <REGISTER_NAME>
```

In case of a `BUS_ERROR`, the error is caught within the program and the error message is printed to the screen.

