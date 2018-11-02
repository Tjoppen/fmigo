#!/bin/bash
set -e

FMU="${FMUS_DIR}/tests/alltypestest/alltypestest.fmu"

# I don't really care about the output, only that all of these can be run
# CSV sans header
mpiexec -np 2 fmigo-mpi -t 0.4 -d 0.1 $FMU > /dev/null
# CSV with header
mpiexec -np 2 fmigo-mpi -t 0.4 -d 0.1 -H $FMU > /dev/null
# TikZ
mpiexec -np 2 fmigo-mpi -t 0.4 -d 0.1 -f tikz $FMU > /dev/null
# Two copies of the same FMU, CSV with header
mpiexec -np 3 fmigo-mpi -t 0.4 -d 0.1 -H $FMU $FMU > /dev/null

if [ $USE_MATIO = 1 ]
then
    # mat5 output with two FMUs
    mpiexec -np 3 fmigo-mpi -t 0.4 -d 0.1 -f mat5      -o temp.mat $FMU $FMU
    # mat5_zlib output with two FMUs
    mpiexec -np 3 fmigo-mpi -t 0.4 -d 0.1 -f mat5_zlib -o temp.mat $FMU $FMU
    rm temp.mat
else
    echo "USE_MATIO=OFF, skipping mat5 test"
fi

echo alltypestest seems to work fine

