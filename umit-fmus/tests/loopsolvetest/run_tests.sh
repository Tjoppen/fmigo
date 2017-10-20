#!/bin/bash
set -e

# Need GSL (and thus GPL) to solve algebraic loop
if [ $USE_GPL -eq 0 ]
then
  exit 0
fi

DIR="${FMUS_DIR}/tests/loopsolvetest"

# Simplest test not solvable with fixed point iteration
mpiexec -np 2 fmigo-mpi -t 0.1 -L -p 0,1,1 -c 0,3,0,2 ${DIR}/sub/sub.fmu > temp.csv
python ../../../compare_csv.py temp.csv simple.ref

# A bit more contrived
mpiexec -np 5 fmigo-mpi -t 0.1 -L \
  -p 0,1,1  -p 2,1,1 \
  -c 0,3,1,1:0,3,3,2 \
  -c 1,3,0,2 \
  -c 2,3,1,2:2,3,3,1 \
  -c 3,3,2,2 \
  ${DIR}/sub/sub.fmu ${DIR}/add/add.fmu ${DIR}/sub/sub.fmu ${DIR}/mul/mul.fmu > temp.csv
python ../../../compare_csv.py temp.csv complicated.ref
rm temp.csv

echo Loop solving = OK
