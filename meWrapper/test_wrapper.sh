#!/bin/bash
set -e

function test(){
COMPARE=../../compare_csv.py
RESULT=/tmp/result.csv
WRAPPER=$1
echo $4
CHECK=$2
mpirun -np $3 fmigo-mpi $4 ${WRAPPER} > ${RESULT} #2>/dev/null
cat  ${RESULT} > ${CHECK}
#cat  ${RESULT}
python ${COMPARE} ${RESULT} ${CHECK}

rm ${RESULT}
}

conn="-p 0,v1,10:0,k_internal,1:0,k1,1"
conn="${conn} -p 1,v1,10:1,k_internal,1:1,k1,1"
conn="${conn} -C shaft,0,1,omega1,phi1,a1,f1,omega2,phi2,a2,f2"

SPRING2=spring2/wrapper_springs2.fmu

test "${SPRING2} ${SPRING2}"          tests/connectedsprings.csv 3 "${conn}"

test wrapper/wrapper_bouncingBall.fmu tests/bouncingBall.csv 2 "-t 1.5"
test spring/wrapper_springs.fmu       tests/springs.csv      2
test spring2/wrapper_springs2.fmu     tests/springs2.csv     2


echo Wrapper OK
