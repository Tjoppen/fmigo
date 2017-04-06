#!/bin/bash

SPRINGSCHECK=tests/springs.txt
BOUNCINGCHECK=tests/bouncingBall.txt
SPRINGS=springs/springs.fmu
mpirun -np 2 fmigo-mpi -t 1.5 bouncingBall/bouncingBall.fmu > /tmp/result 2>/dev/null
DIFF=$(diff /tmp/result ${BOUNCINGCHECK})
if [ "$DIFF" != "" ]
then
    cat /tmp/result
    exit 1
fi
echo Bouncing Ball ok

mpirun -np 3 fmigo-mpi -t 12 -p r,1,11,1 -c 0,x0,1,1 ${SPRINGS} ${SPRINGS} > /tmp/result 2>/dev/null
DIFF=$(diff /tmp/result ${SPRINGSCHECK})
if [ "$DIFF" != "" ]
then
    cat /tmp/result
    exit 1
fi
rm /tmp/result
echo Bouncing Ball ok
