# fmu-builder

Build FMUs simply.

## Installation
Make sure you have python, cmake and make installed. To install, run:
```
python setup.py install
```
This software has successfully been tested on Ubuntu 12.10.

## Usage
Assumed myFmuDir contains a file "modelDescription.xml", and a folder with source files "sources", do:
```
cd myFmuDir;
fmu-builder;
```
This will create an FMU called "modelIdentifier.fmu" in current directory.

For help, run ```fmu-builder -h```.
