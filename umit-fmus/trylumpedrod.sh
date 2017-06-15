#!/bin/sh
FMUS_DIR=`pwd`

mpiexec -np 6 fmigo-mpi -t 100 -d 0.1 \
        -C shaft,0,1,0,1,2,3,0,1,2,3 \
        -C shaft,1,2,5,6,7,8,0,1,2,3 \
        -C shaft,2,3,6,7,8,9,0,2,4,10 \
        -C shaft,3,4,1,3,5,11,0,1,2,3 \
        -c 4,1,0,6 \
        ${FMUS_DIR}/kinematictruck/engine/engine.fmu \
        ${FMUS_DIR}/kinematictruck/kinclutch/kinclutch.fmu \
        ${FMUS_DIR}/kinematictruck/gearbox2/gearbox2.fmu \
        ${FMUS_DIR}/lumpedrod/lumpedrod.fmu \
        ${FMUS_DIR}/kinematictruck/body/body.fmu  > out.csv
#END

