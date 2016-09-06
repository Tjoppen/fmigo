MASTER=fmi-mpi-master
SERVER=fmi-mpi-server
FMUS_DIR=`pwd`


run_forcevelocity() {
    for method in gs jacobi
    do
        dt=0.02
        #for dt in 0.002 0.005 0.01 0.02
        #do
            gain=0.2
            #for gain in 0.2 0.5 1 2 5 10 20
            #do
                DT2=$(printf "%04i\n" $(bc -l <<< "$dt * 1000" | sed -e "s/\\..*//"))
                GAIN2=$(printf "%03i\n" $(bc -l <<< "$gain * 10" | sed -e "s/\\..*//"))
                mpiexec -np 1 ${MASTER} -F fieldnames.txt -t 100 -m $method -d $dt -p 0,8,$gain\
			-c 0,0,1,0:0,1,1,1:1,2,0,3 -c 1,4,2,0:1,5,2,1:2,2,1,6 -c 1,5,0,6 :\
                        -np 1 ${SERVER} ${FMUS_DIR}/kinematictruck/engine/engine.fmu :\
                        -np 1 ${SERVER} ${FMUS_DIR}/forcevelocitytruck/gearbox/gearbox.fmu :\
                        -np 1 ${SERVER} ${FMUS_DIR}/forcevelocitytruck/body/body.fmu  > out.csv

                handle_forcevelocity forcevelocity/out-$method-$DT2-$GAIN2.csv
            #done
        #done
    done
}

run_kinematic_clutch() {
    NN="-N"
    dt=0.1
    DT2=$(printf "%03i\n" $(bc -l <<< "$dt * 100" | sed -e "s/\\..*//"))
    METHOD=gs
    GAIN=0.2
    OUTNAME="clutch-h-$METHOD-$DT2-gain-$GAIN.csv"

    mpiexec -np 1 ${MASTER} $NN -F fieldnames.txt \
	    -m $METHOD \
	    -t 100 -d 0.01 -p\
            0,8,$GAIN \
	    -C shaft,0,1,0,1,2,3,0,1,2,3 -C shaft,1,2,5,6,7,8,0,1,2,3 -C shaft,2,3,6,7,8,9,0,1,2,3 -c 3,1,0,6 :\
            -np 1 ${SERVER} ${FMUS_DIR}/kinematictruck/engine/engine.fmu :\
            -np 1 ${SERVER} ${FMUS_DIR}/kinematictruck/clutch/clutch.fmu :\
            -np 1 ${SERVER} ${FMUS_DIR}/kinematictruck/gearbox2/gearbox2.fmu :\
            -np 1 ${SERVER} \
            ${FMUS_DIR}/kinematictruck/body/body.fmu  > $OUTNAME

            #sed -e "s/,/ /g" < out.csv > out.ssv    #CSV -> space separated values
            #mv out.csv $OUTNAME

            #handle_kinematic_clutch_result
}

run_kinematic_clutch

python2 plotforcevelocity.py $OUTNAME
