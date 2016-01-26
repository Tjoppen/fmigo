fmi-tcp-master   {#mainpage}
==============
Co-simulates slaves (FMUs) over TCP. Has built in support for weak and strong coupling.

# Typical usage

1. The FMI host starts the slave ```server```.
2. A user that wants to simulate starts the ```master``` application, which connects to one or more slaves.
3. Simulation is run and results are stored on disk.

# Install instructions
Prerequisities are
* [ZMQ] (http://zeromq.org/)
* [Protobuf](https://developers.google.com/protocol-buffers)
* [FMITCP](https://github.com/umitresearchlab/fmi-co-simulation)
* [Strong Coupling Core](https://github.com/umitresearchlab/strong-coupling-core)

To build, run:

    cd fmi-tcp-master/;
    mkdir build && cd build;
    cmake .. -DSTRONG_COUPLING_INCLUDE_DIR=<Strong coupling core include files location> \
             -DSTRONG_COUPLING_LIBS_DIR=<Strong coupling core libraries location>
             -DFMIL_INCLUDE_DIR=<FMILibrary include files location> \
             -DFMIL_LIBS_DIR=<FMILibrary libraries location>

# Usage
Typically, you start the server and listen to a port. You you provide the path to an FMU or just "dummy" to serve a dummy FMU.

    ./bin/slave --host localhost --port 3000 dummy

Start client and connect to server. This code below will connect and simulate until time=10:

    ./bin/master --stopAfter 10 tcp://localhost:3000

Run any of the commands with the --help flag to see the help pages.
