#!/bin/sh

mpirun -np  2 fmigo-mpi -t 1.5 bouncingBall/bouncingBall.fmu
echo Bouncing Ball ok
