#!/bin/bash
set -e

FMU="${FMUS_DIR}/tests/alltypestest/alltypestest.fmu"

# I don't really care about the output, only that all of these can be run
# CSV sans header
mpiexec -np 2 fmigo-mpi -t 0.4 -d 0.1 $FMU
# CSV with header
mpiexec -np 2 fmigo-mpi -t 0.4 -d 0.1 -H $FMU
# TikZ
mpiexec -np 2 fmigo-mpi -t 0.4 -d 0.1 -f tikz $FMU
# Two copies of the same FMU, CSV with header
mpiexec -np 3 fmigo-mpi -t 0.4 -d 0.1 -H $FMU $FMU

echo alltypestest seems to work fine

