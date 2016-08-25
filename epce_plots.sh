#!/bin/bash
set -e

run_coupled_sho() {
    N=$1
    dt=$2
    omega=$(python <<< "print($N * $3)")
    damping=$(python <<< "print($4)")
    filter_length=$5
    pulse_length=$(python <<< "print(int(1 / $dt))")
    output_filename=$6
    tmax=20

    FMUS=": -np 1 fmi-mpi-server ../../impulse/impulse.fmu"
    CONNS="-c 0,1,1,5"
    PARAMS="-p i,0,4,1:i,0,6,${pulse_length}"
    for i in $(seq 1 $N)
    do
        i2=$(bc <<< "$i + 1")
        if [ $i -lt $N ]
        then
            CONNS="${CONNS} -c $i,12,$i2,5:$i2,10,$i,6"
        fi
        PARAMS="${PARAMS} -p i,$i,98,${filter_length} -p $i,0,1:$i,1,${omega}:$i,2,${damping}:$i,3,0:$i,4,0:$i,95,0:$i,7,0:$i,8,0:$i,9,0"
        FMUS="${FMUS} : -np 1 fmi-mpi-server coupled_sho.fmu"
    done

    mpiexec -np 1 fmi-mpi-master -m jacobi -t ${tmax} -d $dt ${CONNS} ${PARAMS} ${FMUS} > temp.csv || true
    # Cut out mpi crap if GSL exploded
    sed -i '/---.*/{N;N;N;N;N;N;N;N;N;N;s/---.*//}' temp.csv
    octave --no-gui --eval "
        d=csvread('temp.csv');
        plot(d(:,1),d(:,3),'k-');
        hold on;
        plot(d(:,1),max(-5,min(5,d(:,7:4:end))),'.-');
        title('N=$N, omega=$omega, damping=$damping, dt=$dt, filter\_length=$filter_length');
        axis([0,${tmax},-0.5,1.5], 'manual');
        name='$output_filename'
        page_screen_output(0);
        page_output_immediately(1);
        print(name);
        sleep(0.3); %there's some issue with syncing files to disk, so sleep for a bit...
"
    rm temp.csv
}

pushd gsl2/coupled_sho

#run_coupled_sho 16 0.1  0.1 1 2 out-on-0.1.pdf
#run_coupled_sho 16 0.1  1   1 2 out-on-1.pdf
#run_coupled_sho 16 0.1 10   1 2 out-on-10.pdf
#run_coupled_sho 16 0.1  0.1 1 0 out-off-0.1.pdf
#run_coupled_sho 16 0.1  1   1 0 out-off-1.pdf
#run_coupled_sho 16 0.1 10   1 0 out-off-10.pdf

popd

run_chained_sho() {
    N=$1

    # dt must decrease with N since k_c increases with N
    dt=$(python <<< "print($2 / $N)")
    #dt=$2

    filter_length=$3
    output_filename=$4
    echo $dt

    mass=$(python <<< "print(1.0 / $N)")
    k_i=$(python <<< "print(1.0 / $N)")
    damping_i=$(python <<< "print(1.0 / $N)")
    k_c=$(python <<< "print(10*$N)")

    # It seems that we don't want damping in the coupling springs,
    # which kind of makes sense. If we did we'd lose energy
    damping_c=0
    #damping_c=$(python <<< "print(10/$N)")
    #damping_c=1
    #damping_c=$(python <<< "print(10*$N)")

    tmax=40

    amp=$(python <<< "print(1.0 / $dt)")
    start=$(python <<< "print(int(0.0 / $dt))")
    length=$(python <<< "print(int(20.0 / $dt))")

    FMUS=": -np 1 fmi-mpi-server ../../impulse/impulse.fmu"
    # Connect impulse generator as force input on last system
    CONNS="-c 0,0,$N,6"
    PARAMS="-p i,0,4,0:i,0,5,${start}:i,0,6,${length}:0,7,1"
    # Disable coupling spring on first system
    PARAMS="${PARAMS} -p 1,1,0:1,2,0"
    # Dump state of first system in sho.m
    PARAMS="${PARAMS} -p b,1,97,true"

    for i in $(seq 1 $N)
    do
        i2=$(bc <<< "$i + 1")
        if [ $i -lt $N ]
        then
            CONNS="${CONNS} -c $i,12,$i2,5:$i2,10,$i,6"
        fi

        if [ $i -ne 1 ]
        then
            # Set coupling springs except for the first system which has none
            PARAMS="${PARAMS} -p $i,1,${k_c}:$i,2,${damping_c}"
        fi

        PARAMS="${PARAMS} -p i,$i,98,${filter_length} -p $i,0,${mass}:$i,4,${damping_i}:$i,95,${k_i}:$i,7,0"
        FMUS="${FMUS} : -np 1 fmi-mpi-server chained_sho.fmu"
    done

    mpiexec -np 1 fmi-mpi-master -m gs -t ${tmax} -d $dt ${CONNS} ${PARAMS} ${FMUS} > ${output_filename} || true
    # Cut out mpi crap if GSL exploded
    sed -i '/---.*/{N;N;N;N;N;N;N;N;N;N;s/---.*//}' ${output_filename}
}

pushd gsl2/chained_sho

run_chained_sho 1  0.5 0 out-1.csv
run_chained_sho 4  0.5 0 out-4.csv

octave --no-gui --persist --eval "
        d=csvread('out-1.csv');

        % This is the force impulse
        plot(d(:,1),d(:,2),'k-')
        hold on;

        % This is the N=1 system's behavior
        plot(d(:,1),d(:,6),'kx-')

        % Finally, plot the N=4 system
        d=csvread('out-4.csv');
        plot(d(:,1),d(:,6:4:end),'.-');
"

popd
