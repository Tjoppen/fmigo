fmi-tcp       {#mainpage}
=======

FMI over TCP library. In addition to the library, it includes a protocol, a C++ class layers for Client and Server, and a test application. The library is built on top of
[FMILibrary](http://www.jmodelica.org/FMILibrary) from
[jModelica](http://www.jmodelica.org),
[Lacewing](http://lacewing-project.org/) and
[Google Protocol Buffers](https://developers.google.com/protocol-buffers/).

### Typical usage

1. The FMI host starts the slave server.
2. A user that wants to simulate starts the master application, which connects to one or more slaves.
3. Simulation is run and results are stored on disk.

### Build

The build was done successfully on Ubuntu Linux 13.10 & Windows 7.

* FMILibrary version used is FMILibrary-2.0b2.
* Lacewing version used is 0.5.4.
* Google Protocol Buffers version used is 2.4.1.

Begin with installing [FMILibrary](http://www.jmodelica.org/FMILibrary) and [Lacewing](http://lacewing-project.org/). Make sure the libraries and include files ends up in /usr/lib and /usr/include.
Note that code is forced to link against the static lib of Lacewing.

Build and install using the following commands. You'll need [CMake](http://www.cmake.org/).

    git clone https://github.com/umitresearchlab/fmi-co-simulation.git;
    cd fmi-co-simulation;
    mkdir build;
    cd build;
    cmake ..;
    make;
    sudo make install;

If the libraries and include files are not in /usr/lib and /usr/include then use the ```FMIL_INCLUDE_DIR``` , ```LACEWING_INCLUDE_DIR``` && ```PROTOBUF_INCLUDE_DIR``` variables to specify the include files location and ```FMIL_LIBS_DIR``` , ```LACEWING_LIBS_DIR``` & ```PROTOBUF_LIBRARY``` variables to specify the libraries location.

```PROTOBUF_BIN_DIR``` variable is used to define the Google Protocol Buffers binaries location.

    cmake .. -DFMIL_INCLUDE_DIR=<FMIL include files location> -DLACEWING_INCLUDE_DIR=<Lacewing include files location> -DPROTOBUF_INCLUDE_DIR=<Google Protocol Buffers include files location> -DPROTOBUF_BIN_DIR=<Google Protocol Buffers binaries location> -DFMIL_LIBS_DIR=<FMIL libraries location> -DLACEWING_LIBS_DIR=<Lacewing libraries location> -DPROTOBUF_LIBRARY=<Google Protocol Buffers libraries location>
