#!/bin/sh

mpirun \
    -np  2 fmigo-mpi -t 0.003 -d 0.00001 -H \
    -p 0,filter_length,0 \
    -p 0,v_in_s,1 \
    -p 0,k_sc,1000000000.0 \
    -p 0,gamma_sc,44271.887242357305 \
    -p 0,is_gearbox,true \
    -p 0,gear_k,4934802.200544679 \
    -p 0,gear_d,2199.114857512855 \
    -p 0,gear,13 \
    -p 0,mass_e,0.5 \
    -p 0,mass_s,0.5 \
    ./gsl2/clutch2/clutch2.fmu    > /tmp/f.csv

