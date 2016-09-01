#!/bin/bash
set -e

# Simplest test - check that the default output is all zeroes
mpirun -np 1 fmi-mpi-master -t 0.2 : -np 1 fmi-mpi-server typeconvtest.fmu | python check.py

# Check basic type conversion, including float->int truncation
mpirun -np 1 fmi-mpi-master -t 0.2 -p r,0,8,10.9  -c r,0,0,i,0,5 -c r,0,0,b,0,6 : -np 1 fmi-mpi-server typeconvtest.fmu | python check.py
mpirun -np 1 fmi-mpi-master -t 0.2 -p i,0,9,10    -c i,0,1,r,0,4 -c i,0,1,b,0,6 : -np 1 fmi-mpi-server typeconvtest.fmu | python check.py
mpirun -np 1 fmi-mpi-master -t 0.2 -p b,0,10,true -c b,0,2,r,0,4 -c b,0,2,i,0,5 : -np 1 fmi-mpi-server typeconvtest.fmu | python check.py

# Negative values
mpirun -np 1 fmi-mpi-master -t 0.2 -p r,0,8,-10.9  -c r,0,0,i,0,5 -c r,0,0,b,0,6 : -np 1 fmi-mpi-server typeconvtest.fmu | python check.py
mpirun -np 1 fmi-mpi-master -t 0.2 -p i,0,9,-10    -c i,0,1,r,0,4 -c i,0,1,b,0,6 : -np 1 fmi-mpi-server typeconvtest.fmu | python check.py

# Scaling (k=10, m=1)
mpirun -np 1 fmi-mpi-master -t 0.2 -p r,0,8,10.9  -c r,0,0,i,0,5,10,1 -c r,0,0,b,0,6,10,1 : -np 1 fmi-mpi-server typeconvtest.fmu | python check.py 10 1
mpirun -np 1 fmi-mpi-master -t 0.2 -p i,0,9,10    -c i,0,1,r,0,4,10,1 -c i,0,1,b,0,6,10,1 : -np 1 fmi-mpi-server typeconvtest.fmu | python check.py 10 1
mpirun -np 1 fmi-mpi-master -t 0.2 -p b,0,10,true -c b,0,2,r,0,4,10,1 -c b,0,2,i,0,5,10,1 : -np 1 fmi-mpi-server typeconvtest.fmu | python check.py 10 1

echo Type conversion seems to work fine
