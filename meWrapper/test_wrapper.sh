#!/bin/bash
set -e

COMPARE=../../compare_csv.py
WRAPPERCHECK=tests/wrapper.txt
SPRINGSCHECK=tests/springs.txt
WRAPPER=wrapper/wrapper_bouncingBall.fmu
RESULT=/tmp/result.csv
mpirun -np 2 fmigo-mpi -t 1.5 ${WRAPPER} > ${RESULT} 2>/dev/null

cat  ${RESULT} > ${WRAPPERCHECK}
python ${COMPARE} ${RESULT} ${WRAPPERCHECK}


rm ${RESULT}
echo Wrapper OK
