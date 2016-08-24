#!/bin/bash
set -e

cd gsl2/coupled_sho

run_test() {
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

run_test 16 0.1  0.1 1 2 out-on-0.1.pdf
run_test 16 0.1  1   1 2 out-on-1.pdf
run_test 16 0.1 10   1 2 out-on-10.pdf
run_test 16 0.1  0.1 1 0 out-off-0.1.pdf
run_test 16 0.1  1   1 0 out-off-1.pdf
run_test 16 0.1 10   1 0 out-off-10.pdf


