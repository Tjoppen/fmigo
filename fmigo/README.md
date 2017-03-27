fmigo
=====

Runs FMUs over TCP (via ZMQ) or MPI. FMUs can be either Co-Simulation or Model Exchange type.
FMUs can be connected in various ways.

# Typical usage

fmigo can be run in two ways: either over TCP using ZMQ, or using MPI.
See the manpage for more detailed information about command line options.

## FMI/TCP (ZMQ)

1. Serve each FMU using fmigo-server, like ```fmigo-server --port 1234 foo.fmu```
2. Run master, list connections and server URIs, like ```fmigo-master -c 0,1,1,2 tcp://localhost:1234 tcp://localhost:2345```
3. Simulation is run, output is printed in CSV form to stdout

## FMI/MPI

1. Run fmigo-mpi with connections, parameters and FMU paths all on one line, like ```mpiexec -np 3 fmigo-mpi -c 0,1,1,2 foo.fmu bar.fmu```.
   The number of nodes should be equal to the number of FMUs plus one (node 0 is the master node).
3. Simulation is run, output is printed in CSV form to stdout

# Build/install instructions

## Debian/Ubuntu

Use CMake and make/ninja to build.
For a list of required packages see ../Dockerfile.*

## Windows

See ../Windows-build-instructions.txt
