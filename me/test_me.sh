#!/bin/bash

SPRINGSCHECK=tests/springs.txt
BOUNCINGCHECK=tests/bouncingBall.txt
SPRINGS=springs/springs.fmu
mpiexec -np 2 fmigo-mpi -t 1.5 bouncingBall/bouncingBall.fmu > result 2>/dev/null
DIFF=$(diff result ${BOUNCINGCHECK})
if [ "$DIFF" != "" ]
then
    cat result
    exit 1
fi
echo Bouncing Ball ok

mpiexec -np 3 fmigo-mpi -t 12 -p r,1,11,1 -c 0,x0,1,1 ${SPRINGS} ${SPRINGS} > result 2>/dev/null
DIFF=$(diff result ${SPRINGSCHECK})
if [ "$DIFF" != "" ]
then
    cat result
    exit 1
fi
rm result
echo Bouncing Ball ok
