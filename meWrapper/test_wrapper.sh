#!/bin/bash
set -e

function test(){
COMPARE=../../compare_csv.py
RESULT=/tmp/result.csv
WRAPPER=$1
CHECK=$2
mpirun -np 2 fmigo-mpi -t 1.5 ${WRAPPER} > ${RESULT} 2>/dev/null
cat  ${RESULT} > ${CHECK}
python ${COMPARE} ${RESULT} ${CHECK}
}

test wrapper/wrapper_bouncingBall.fmu tests/bouncingBall.csv
test spring/wrapper_springs.fmu       tests/springs.csv
test spring2/wrapper_springs2.fmu     tests/springs2.csv

rm ${RESULT}
echo Wrapper OK
