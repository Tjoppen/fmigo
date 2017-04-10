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
    mpirun -np $3 fmigo-mpi $4 ${WRAPPER} #> ${RESULT}  || (echo "FAILED: " $1 && exit 1)
    if [ ${REStoCHECK} = "set" ]; then
        cat  ${RESULT} > ${CHECK}
    fi
    cat  ${RESULT}
    python ${COMPARE} ${RESULT} ${CHECK}

    rm ${RESULT}
}
#REStoCHECK="set"
test ${FMUS_DIR}/meWrapper/springs/wrapper_springs.fmu       tests/springs.csv      2

#test ${FMUS_DIR}/meWrapper/bouncingBall/wrapper_bouncingBall.fmu tests/bouncingBall.csv 2 "-t 1.5"

SPRINGS2=springs2/wrapper_springs2.fmu
conn="-p 0,v1,10:0,k_internal,1:0,k1,1"
#test ${FMUS_DIR}/meWrapper/${SPRINGS2}     tests/springs2.csv     2 "${conn}"
REStoCHECK=""

conn="${conn} -p 1,v1,10:1,k_internal,1:1,k1,1"
conn="${conn} -C shaft,0,1,omega1,phi1,a1,f1,omega2,phi2,a2,f2"
#test "${FMUS_DIR}/meWrapper/${SPRINGS2} ${FMUS_DIR}/meWrapper/${SPRINGS2}"          tests/connectedsprings.csv 3 "${conn}"

#test ${FMUS_DIR}/meWrapper/scania-driveline/wrapper_drivetrainMFunctionOnly.fmu tests/springs2.csv     2

echo Wrapper OK
