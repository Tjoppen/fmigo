#!/bin/bash
set -e

COMPARE=../../compare_csv.py
SPRINGSCHECK=tests/springs.txt
BOUNCINGCHECK=tests/bouncingBall.txt
SPRINGS=springs/springs.fmu
RESULT=/tmp/result.csv
mpirun -np 2 fmigo-mpi -t 1.5 bouncingBall/bouncingBall.fmu > ${RESULT} 2>/dev/null

#cat ${RESULT} > ${BOUNCINGCHECK}
python ${COMPARE} ${RESULT} ${BOUNCINGCHECK}

mpirun -np 3 fmigo-mpi -t 12 -p r,1,11,1 -c 0,x0,1,1 ${SPRINGS} ${SPRINGS} > ${RESULT} 2>/dev/null
#cat ${RESULT} > ${SPRINGSCHECK}
python ${COMPARE} ${RESULT}  ${SPRINGSCHECK}


rm ${RESULT}
echo ModelExchange
