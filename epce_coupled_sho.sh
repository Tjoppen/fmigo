#!/bin/sh
set -e

cd gsl2/coupled_sho
mpiexec -np 1 fmi-mpi-master -d 0.25 -p i,0,98,2 -p b,0,97,true -p 0,0,1:0,1,10:0,2,0.5:0,3,0:0,5,1:0,7,0:0,8,0:0,9,0 : \
    -np 1 fmi-mpi-server coupled_sho.fmu > out.csv

octave --persist --eval "d=load('sho.m'); d2=load('out.csv'); plot(d(:,1),d(:,2:end),'x-'); legend('position','velocity','dx','z(position)','z(velocity)','z(dx)'); hold on; plot(d2(:,1),d2(:,[3,4]),'ko-')"