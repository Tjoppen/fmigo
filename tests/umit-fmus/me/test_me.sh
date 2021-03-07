#!/bin/bash
set -e

pushd ../../..
source boilerplate.sh
popd

${MPIEXEC} -np 2 fmigo-mpi -t 1.5 ${FMUS_DIR}/me/bouncingBall/bouncingBall.fmu > result
python3 $COMPARE_CSV result tests/bouncingBall.csv
echo Bouncing Ball ok

${MPIEXEC} -np 3 fmigo-mpi -t 12 -p r,1,11,1 -c 0,x0,1,x_in ${FMUS_DIR}/me/springs/springs.fmu ${FMUS_DIR}/me/springs/springs.fmu > result
python3 $COMPARE_CSV result tests/springs.txt
echo Springs ok

${MPIEXEC} -np 2 fmigo-mpi -p 0,v1,10:0,k_internal,1:0,k1,1 ${FMUS_DIR}/me/springs2/springs2.fmu > result
python3 $COMPARE_CSV result tests/springs2.csv
echo Springs2 ok

echo ModelExchange ok
rm result
