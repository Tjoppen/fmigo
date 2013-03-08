# FMU co-simulation master CLI
Command line interface for doing co-simulation using FMUs. Built on top of [FMILibrary](http://www.jmodelica.org/FMILibrary) from [jModelica](http://www.jmodelica.org).

### Build
The build was done successfully on Ubuntu Linux 12.10.

Begin with installing [FMILibrary](http://www.jmodelica.org/FMILibrary). Make sure the libraries and include files ends up in /usr/lib and /usr/include (or whatever suits you best).

Build the master using the following commands.
```
cmake .;
make;
```

### Test
If everything went alright, the executable generated will be ```build/fmu-master```. To view the command help page, run it without arguments:
```
./build/master
```

You can also use the test script:
```
./test.sh
```

To be able to run the command ```fmu-master``` directly, add a symbolic link to ```/usr/bin```: 
```
sudo ln -s /home/user/fmilib-master/bin/fmu-master /usr/bin/fmu-master
```