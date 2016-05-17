#!/bin/sh

FMIPATH=`pwd`/../build/install/bin


mpirun \
    -np  1 $FMIPATH/fmi-mpi-master\
    -d 0.1 \
    -p r,0,10,1:r,0,11,-1:r,0,16,2:r,0,22,20:r,0,23,-20:r,0,15,10:r,0,17,0:r,0,18,0:r,0,19,0 : \
-np  1 $FMIPATH/fmi-mpi-server ./lumpedrod/lumpedrod.fmu  
