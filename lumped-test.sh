#!/bin/sh

FMIPATH=`pwd`/../build/install/bin


mpirun \
    -np  1 $FMIPATH/fmi-mpi-master -d 0.1 -p r,0,10,0:r,0,11,0:r,0,16,0.1:r,0,22,1:r,0,23,-1:r,0,15,1e2 : \
-np  1 $FMIPATH/fmi-mpi-server ./lumpedrod/lumpedrod.fmu  
