#!/bin/bash
set -e
pushd ../../..
source boilerplate.sh
popd

testit() {
  s=$1

  echo "Testing $s: $5"
  mpiexec -np 3 fmigo-mpi -t 100 \
    -C shaft,0,1,theta,omega,alpha,tau,x1,v1,a1,f1 \
    -c 1,v1,0,omega_l \
    -p 1,m1,100:1,m2,100 \
    -p 1,k_internal,10000:1,gamma_internal,10 \
    ${FMUS_DIR}/kinematictruck/engine/engine.fmu \
    ${FMUS_DIR}/meWrapper/${s}/wrapper_${s}.fmu > ${s}.csv

  # Each variant should behave the same
  # TODO: compare everything but the last six columns?
  # a not-exactly-the-same mobility estimate results in
  # some slight phase shifts on the forces, which throws
  # the comparison entirely off
  python ../../../compare_csv.py ref.csv ${s}.csv "," "$2" "$3" "$4"
  rm ${s}.csv
  echo "OK"
}

# Test the three different ways to get directional derivatives in wrapper.c
testit springs2 0.00001 ""               0     "pass-through (providesDirectionalDerivatives = true)"
testit springs3 0.00001 ""               0     "directional.txt"
testit springs4 0.00001 "3,4,7,12,15,18" 0.05  "numerical derivative"
