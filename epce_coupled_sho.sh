#!/bin/bash
set -e

cd gsl2/coupled_sho

if [ 1 == 0 ]
then
    mpiexec -np 1 fmi-mpi-master -d 0.25 -p i,0,98,2 -p b,0,97,true -p 0,0,1:0,1,10:0,2,0.5:0,3,0:0,5,1:0,7,0:0,8,0:0,9,0 : \
        -np 1 fmi-mpi-server coupled_sho.fmu > out.csv
    octave --persist --eval "d=load('sho.m'); d2=load('out.csv'); plot(d(:,1),d(:,2:end),'x-'); legend('position','velocity','dx','z(position)','z(velocity)','z(dx)'); hold on; plot(d2(:,1),d2(:,[3,4]),'ko-')"
fi

if [ 1 == 1 ]
then
    OMEGAS=$(seq 0.1 0.1 10|sed -e 's/,/./g')
    #"0.1 0.3 1 3 10 30"
    OCTAVE_SCRIPT=""
    for omega in ${OMEGAS}
    do
        #mpiexec -np 1 fmi-mpi-master -t 1000 -d 1 -p i,0,98,2 -p b,0,97,false -p 0,0,1:0,1,${omega}:0,2,0.5:0,3,0:0,5,1:0,7,0:0,8,0:0,9,0 : \
        #    -np 1 fmi-mpi-server coupled_sho.fmu > out-${omega}.csv
        OCTAVE_SCRIPT="${OCTAVE_SCRIPT}
    temp = load('out-${omega}.csv'); d(:,size(d,2)+1) = temp(:,4);
    "
    done

    octave --no-gui --persist -q --eval "
    d = load('out-0.1.csv');
    d = d(:,1);
    ${OCTAVE_SCRIPT}
    t = d(:,1);
    d = d(:,2:end);

    omegas = 0.1:0.1:10;

    [XX,YY] = meshgrid( omegas, 1:20 );

    subplot(2,1,1);
    surface(XX,YY,d(1:20,:));
    xlabel('h*omega');
    ylabel('t');
    legend('v')

    subplot(2,1,2);
    plot(omegas, max(d));
    xlabel('h*omega');
    ylabel('max(v)');

    "
fi

if [ 1 == 0 ]
then
    omega=60
    mpiexec -np 1 fmi-mpi-master -m gs -t 100 -d 0.1 \
        -c 0,12,1,5:1,10,0,6 \
        -c 1,12,2,5:2,10,1,6 \
        -c 2,12,3,5:3,10,2,6 \
        -p i,0,98,2 -p 0,0,1:0,1,${omega}:0,2,0.5:0,3,0:0,5,1:0,7,0:0,8,0:0,9,0 \
        -p i,1,98,2 -p 1,0,1:1,1,${omega}:1,2,0.5:1,3,0    -p 1,7,0:1,8,0:1,9,0 \
        -p i,2,98,2 -p 2,0,1:2,1,${omega}:2,2,0.5:2,3,0    -p 2,7,0:2,8,0:2,9,0 \
        -p i,3,98,2 -p 3,0,1:3,1,${omega}:3,2,0.5:3,3,0    -p 3,7,0:3,8,0:3,9,0 \
        : -np 1 fmi-mpi-server coupled_sho.fmu \
        : -np 1 fmi-mpi-server coupled_sho.fmu \
        : -np 1 fmi-mpi-server coupled_sho.fmu \
        : -np 1 fmi-mpi-server coupled_sho.fmu > out.csv
    echo done running fmi
    octave --no-gui -q --persist --eval "d=load('out.csv'); plot(d(:,1),d(:,[5,9,13,17]),'.-')"
fi
