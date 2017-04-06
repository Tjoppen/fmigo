#!/bin/sh

mpirun \
    -np  2 fmigo-mpi  -F names.txt -d 0.01  \
    -p i,0,filter_length,0 \
    -p 0,v_in_s,1 \
    -p 0,k_sc,100000000 \
    -p 0,gamma_sc,100000 \
    -p b,0,17,true \
    ./gsl2/clutch2/clutch2.fmu    > /tmp/f.csv

