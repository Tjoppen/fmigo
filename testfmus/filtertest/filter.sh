#!/bin/sh
set -e

for filter_length in 0 1 2
do
  mpirun \
    -np  2 fmigo-mpi -t 0.01 -d 0.0004 \
    -p 0,filter_length,$filter_length \
    -p 0,v_in_e,1 \
    -p 0,k_ec,1000000000.0 \
    -p 0,gamma_ec,44271.887242357305 \
    -p 0,is_gearbox,true \
    -p 0,gear_k,4934802.200544679 \
    -p 0,gear_d,2199.114857512855 \
    -p 0,gear,13 \
    -p 0,mass_e,0.5 \
    -p 0,mass_s,0.5 \
    -p 0,integrate_dx_e,true \
    -p 0,integrate_dx_s,true \
    ../../gsl2/clutch2/clutch2.fmu    > out-$filter_length.csv
done

python filtertest.py
#rm out-0.csv out-1.csv out-2.csv

