#!/bin/bash
set -e
mpiexec -np 1 fmi-mpi-master -t 12 -m me : \
        -np 1 fmi-mpi-server ${FMUS_DIR}/me/springs/springs.fmu
