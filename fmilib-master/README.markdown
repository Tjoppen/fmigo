# FMU co-simulation master
Built on top of FMILibrary from JModelica. The idea is to build a master algorithm that can be integrated with other softwares, e.g. a web service.

### Build
The build was done successfully on Ubuntu Linux 12.10.

Begin with installing FMILibrary from http://www.jmodelica.org/FMILibrary. Make sure the libraries and include files ends up in /usr/lib and /usr/include (or whatever suits you best).

Build the master using the following commands.

```
cmake .;
make;
```

### Test
If everything went alright, the executable generated will be ```build/master```. To view the command help page, run it without arguments:
```
./build/master
```

You can also use the test script:
```
./test.sh
```