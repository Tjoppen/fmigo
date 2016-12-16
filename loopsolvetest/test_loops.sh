#!/bin/bash
set -e

# Simplest test not solvable with fixed point iteration
mpiexec -np 1 fmi-mpi-master -t 0.1 -L -p 0,1,1 -c 0,3,0,2  : -np 1 fmi-mpi-server sub/sub.fmu > temp.csv
python ../../compare_csv.py temp.csv simple.ref

# A bit more contrived
mpiexec -np 1 fmi-mpi-master -t 0.1 -L \
  -p 0,1,1  -p 2,1,1 \
  -c 0,3,1,1:0,3,3,2 \
  -c 1,3,0,2 \
  -c 2,3,1,2:2,3,3,1 \
  -c 3,3,2,2 \
  : -np 1 fmi-mpi-server sub/sub.fmu : -np 1 fmi-mpi-server add/add.fmu \
  : -np 1 fmi-mpi-server sub/sub.fmu : -np 1 fmi-mpi-server mul/mul.fmu > temp.csv
python ../../compare_csv.py temp.csv complicated.ref
rm temp.csv

echo Loop solving = OK
