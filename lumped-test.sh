#!/bin/sh

TAU1=r,0,10,4
TAU2=r,0,11,-4
J0=r,0,16,20
DS2=r,0,23,1
OMEGAD1=r,0,15,-2
OMEGAD2=r,0,13,2
COMPLIANCE=r,0,17,2
RELAX=r,0,18,2
KD1=r,0,19,100
DD1=r,0,20,10
KD2=r,0,21,100
DD2=r,0,22,10
N=i,0,28,6
mpirun \
    -np  1 fmi-mpi-master\
    -d 0.1 \
    -p $KD1:$DD1:$KD2:$DD2:$TAU1:$TAU2:$COMPLIANCE:$RELAX:$N:$J0 : \
-np  1 fmi-mpi-server ./lumpedrod/lumpedrod.fmu

    
