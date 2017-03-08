#!/bin/sh

mpirun \
    -np  4 fmigo-mpi -d 0.001  -c 0,1,1,6 -c 1,9,0,3 \
    -c 1,8,2,0 -c 2,8,1,7 -c 2,3,0,6 \
    -p 2,5,1 \
    ./kinematictruck/engine/engine.fmu \
    ./gsl2/clutch/clutch.fmu \
    ./gsl2/mass_force_fe/mass_force_fe.fmu
