#!/bin/bash
set -e

pushd ../../
source boilerplate.sh
popd

COMPARE_CSV=$(pwd)/../../compare_csv.py

mpiexec -np 2 fmigo-mpi -t 1.5 bouncingBall/bouncingBall.fmu > result
python $COMPARE_CSV result tests/bouncingBall.csv
echo Bouncing Ball ok

mpiexec -np 3 fmigo-mpi -t 12 -p r,1,11,1 -c 0,x0,1,x_in springs/springs.fmu springs/springs.fmu > result
python $COMPARE_CSV result tests/springs.txt
echo Springs ok

mpiexec -np 2 fmigo-mpi -p 0,v1,10:0,k_internal,1:0,k1,1 springs2/springs2.fmu > result
python $COMPARE_CSV result tests/springs2.csv
echo Springs2 ok

echo ModelExchange ok
rm result
