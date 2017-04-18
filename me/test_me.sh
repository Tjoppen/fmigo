#!/bin/bash
set -e

SPRINGSCHECK=tests/springs.txt
BOUNCINGCHECK=tests/bouncingBall.txt
SPRINGS=springs/springs.fmu
COMPARE_CSV=$(pwd)/../../compare_csv.py

mpiexec -np 2 fmigo-mpi -t 1.5 bouncingBall/bouncingBall.fmu > result
python $COMPARE_CSV result $BOUNCINGCHECK
echo Bouncing Ball ok

mpiexec -np 3 fmigo-mpi -t 12 -p r,1,11,1 -c 0,x0,1,x_in ${SPRINGS} ${SPRINGS} > result
python $COMPARE_CSV result $SPRINGSCHECK
echo Springs ok

rm result

