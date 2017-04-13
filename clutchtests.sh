#!/bin/sh

FMIPATH=`pwd`/../build/install/bin 
# masses:  5, 5000
# torque in:  100
# velocity: 8, 4
# mass1   vr 4
# mass2   vr 6
# v1   vr 11
# v2   vr 12

mpirun \
    -np  1 $FMIPATH/fmi-mpi-master -d 0.001 -t 3 \
    -p r,0,4,5:r,0,6,5000:r,0,force_in1,100:r,0,11,8:r,0,12,4\
    : \
-np  1 $FMIPATH/fmi-mpi-server ./gsl/clutch_ef/clutch_ef.fmu 
