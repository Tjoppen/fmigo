#!/bin/bash
# https://www.hpc2n.umu.se/documentation/batchsystem/slurm-submit-file-design

# Project id
#SBATCH -A SNIC2018-5-35
#SBATCH -n 6
#SBATCH --time=00:10:00

# Purge modules before loading new ones in a script
#ml purge
#ml load intel/2017a itac/2018.1.017
#export VT_ROOT=/pfs/nobackup/home/t/thardin/vt

set -e
pushd ../../../../../
source boilerplate.sh
popd

./perftest3.sh

#mpiicc -trace mpi-speed-test.c -o mpi-speed-test-icc
#time mpirun -trace -n 8 ./mpi-speed-test-icc 1
