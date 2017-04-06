#!/bin/sh

SPRINGS=springs/springs.fmu
mpirun -np  2 fmigo-mpi -t 1.5 bouncingBall/bouncingBall.fmu > /tmp/result
cat /tmp/result
echo Bouncing Ball ok
mpirun -np 3 fmigo-mpi -t 12 -p r,1,11,1 -c 0,x0,1,1 ${SPRINGS} ${SPRINGS} > /tmp/result
cat /tmp/result
echo Bouncing Ball ok
