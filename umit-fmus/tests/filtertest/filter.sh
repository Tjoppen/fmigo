#!/bin/sh
set -e

zeta=0.7

kc=1E9
dc=`echo "2*$zeta*sqrt($kc) | bc -l `
J=1
J1=`echo "$J / 2" | bc -l`
freq="1/2/pi"
k=`echo "(2 * pi * freq)^2 * J" | bc -l`

for filter_length in 0 1 2
do
  mpirun \
    -np  2 fmigo-mpi -t 0.01 -d 0.0004 \
    -p 0,filter_length,$filter_length \
    -p 0,v_in_e,1 \
    -p 0,k_ec,1000000000.0 \
    -p 0,gamma_ec,44271.887242357305 \
    -p 0,is_gearbox,true \
    -p 0,gear_k,1 \
    -p 0,gear_d,0.7 \
    -p 0,gear,1 \
    -p 0,mass_e,0.5 \
    -p 0,mass_s,0.5 \
    -p 0,integrate_dx_e,true \
    -p 0,integrate_dx_s,true \
    ../../gsl2/clutch2/clutch2.fmu    > out-$filter_length.csv
done

python filtertest.py
#rm out-0.csv out-1.csv out-2.csv

