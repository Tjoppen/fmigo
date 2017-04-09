#!/bin/bash
set -e
echo ${FMUS_DIR}
pushd ../../
source boilerplate.sh
popd
COMPARE=../../compare_csv.py
RESULT=/tmp/result.csv
function test(){
    WRAPPER=$1
    CHECK=$2
    #2>/dev/null
    mpirun -np $3 fmigo-mpi $4 ${WRAPPER} > ${RESULT}  || (echo "FAILED: " $1 && exit 1)
    if [ ${REStoCHECK} = "set" ]; then
        cat  ${RESULT} > ${CHECK}
    fi
    cat  ${RESULT}
    python ${COMPARE} ${RESULT} ${CHECK}

    rm ${RESULT}
}
REStoCHECK="set"
REStoCHECK="no"

test ${FMUS_DIR}/me/springs/springs.fmu tests/springs.csv 2
conn="-p 0,v1,10:0,k_internal,1:0,k1,1"
test ${FMUS_DIR}/me/springs2/springs2.fmu tests/springs2.csv 2 "${conn}"
test ${FMUS_DIR}/me/bouncingBall/bouncingBall.fmu tests/bouncingBall.csv 2 "-t 1.5"

echo ModelExchange ok
