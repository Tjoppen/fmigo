# FMU co-simulation master CLI
Command line interface for doing co-simulation using FMUs. Built on top of [FMILibrary](http://www.jmodelica.org/FMILibrary) from [jModelica](http://www.jmodelica.org).

### Build
The build was done successfully on Ubuntu Linux 12.10.

Begin with installing [FMILibrary](http://www.jmodelica.org/FMILibrary). Make sure the libraries and include files ends up in /usr/lib and /usr/include (or whatever suits you best).

Build and install the master using the following commands. You'll need [CMake](http://www.cmake.org/).
```
cd path/to/fmilib-master;
mkdir build;
cd build;
cmake ..;
make;
sudo make install;
```

Now you should be able to run ```fmu-master``` from the command line. Run it without arguments to view the help page.
