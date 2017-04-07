#!/bin/bash

WRAPPERCHECK=tests/wrapper.txt
SPRINGSCHECK=tests/springs.txt
WRAPPER=wrapper/wrapper_bouncingBall.fmu

mpirun -np 2 fmigo-mpi -t 1.5 ${WRAPPER} > /tmp/result 2>/dev/null
#cat /tmp/result > ${WRAPPERCHECK}
DIFF=$(diff /tmp/result ${WRAPPERCHECK})
if [ "$DIFF" != "" ]
then
    cat /tmp/result
    exit 1
fi

echo Wrapper OK
