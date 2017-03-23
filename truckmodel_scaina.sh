#!/bin/sh

mpirun \
    -np  5 fmigo-mpi -F headers_scania_model.txt -t 3 -d 0.001  -c 0,1,1,6 -c 1,9,0,3 \
    -c 1,8,2,32 -c 2,31,1,7 -c 3,30,0,6 \
    -c 2,33,3,27 -c 3,36,2,34 \
    -p 2,7,1 -p 2,8,10 -p 2,17,1 -p 2,10,0.5 -p 2,11,1 \
    -p 2,12,1 -p 2,16,1 -p 2,18,1 -p 2,30,1 \
    -p 3,4,1 -p 3,8,0 \
    ./kinematictruck/engine/engine.fmu \
    ./gsl2/clutch/clutch.fmu \
    ./gsl2/scania-driveline/drivetrain_G5IO_m_function_only.fmu \
    ./gsl2/trailer/trailer.fmu > /tmp/f.csv
