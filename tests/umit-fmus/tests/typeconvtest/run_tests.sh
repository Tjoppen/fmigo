#!/bin/bash
set -e

FMU="${FMUS_DIR}/tests/typeconvtest/typeconvtest.fmu"

# Simplest test - check that the default output is all zeroes
mpiexec -np 2 fmigo-mpi -t 0.2 $FMU | python check.py

# Check basic type conversion, including float->int truncation
mpiexec -np 2 fmigo-mpi -t 0.2 -p r,0,8,10.9  -c r,0,0,i,0,5 -c r,0,0,b,0,6 $FMU | python check.py
mpiexec -np 2 fmigo-mpi -t 0.2 -p i,0,9,10    -c i,0,1,r,0,4 -c i,0,1,b,0,6 $FMU | python check.py
mpiexec -np 2 fmigo-mpi -t 0.2 -p b,0,10,true -c b,0,2,r,0,4 -c b,0,2,i,0,5 $FMU | python check.py

# Negative values
mpiexec -np 2 fmigo-mpi -t 0.2 -p r,0,8,-10.9  -c r,0,0,i,0,5 -c r,0,0,b,0,6 $FMU | python check.py
mpiexec -np 2 fmigo-mpi -t 0.2 -p i,0,9,-10    -c i,0,1,r,0,4 -c i,0,1,b,0,6 $FMU | python check.py

# Scaling (k=10, m=1)
mpiexec -np 2 fmigo-mpi -t 0.2 -p r,0,8,10.9  -c r,0,0,i,0,5,10,1 -c r,0,0,b,0,6,10,1 $FMU | python check.py 10 1
mpiexec -np 2 fmigo-mpi -t 0.2 -p i,0,9,10    -c i,0,1,r,0,4,10,1 -c i,0,1,b,0,6,10,1 $FMU | python check.py 10 1
mpiexec -np 2 fmigo-mpi -t 0.2 -p b,0,10,true -c b,0,2,r,0,4,10,1 -c b,0,2,i,0,5,10,1 $FMU | python check.py 10 1

echo Type conversion seems to work fine

mpiexec -np 2 fmigo-mpi -t 0.2 -p 0,r0,10.9  -c 0,r_out,0,i_in,10,1 -c 0,r_out,0,b_in,10,1 $FMU | python check.py 10 1
mpiexec -np 2 fmigo-mpi -t 0.2 -p 0,i0,10    -c 0,i_out,0,r_in,10,1 -c 0,i_out,0,b_in,10,1 $FMU | python check.py 10 1
mpiexec -np 2 fmigo-mpi -t 0.2 -p 0,b0,true  -c 0,b_out,0,r_in,10,1 -c 0,b_out,0,i_in,10,1 $FMU | python check.py 10 1

echo String input seems to work fine
