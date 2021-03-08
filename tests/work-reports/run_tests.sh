#!/bin/bash
set -e

GAIN=20

for N in holonomic nonholonomic
do
  for dt in 1 0.1 0.01
  do
    # Ten iterations is enough
    t=$(bc <<< "$dt * 10")

    NN=""
    if [ "$N" == "nonholonomic" ]
    then
        NN="-N"
    fi

    ${MPIEXEC} -np 4 fmigo-mpi $NN -f tikz -t $t -d $dt -p 0,8,$GAIN -C shaft,0,1,0,1,2,3,0,1,2,3 -C shaft,1,2,6,7,8,9,0,1,2,3 -c 2,1,0,6 \
            ${FMUS_DIR}/kinematictruck/engine/engine.fmu \
            ${FMUS_DIR}/kinematictruck/gearbox2/gearbox2.fmu \
            ${FMUS_DIR}/kinematictruck/body/body.fmu  > out.tikz.data

    DT2=$(printf "%03i\n" $(bc -l <<< "$dt * 100" | sed -e "s/\\..*//"))
    OUTNAME="fmigo-mpi-h-$DT2-gain-$GAIN-$N.tikz.data"

    python3 $COMPARE_CSV out.tikz.data octave/${OUTNAME} ' ' 0.00006
    rm out.tikz.data
  done
done

for method in gs jacobi
do
    for dt in 0.02
    do
        # Ten iterations is enough
        t=$(bc <<< "$dt * 10")
        for gain in 0.2
        do
            DT2=$(printf "%04i\n" $(bc -l <<< "$dt * 1000" | sed -e "s/\\..*//"))
            GAIN2=$(printf "%03i\n" $(bc -l <<< "$gain * 10" | sed -e "s/\\..*//"))
            ${MPIEXEC} -np 4 fmigo-mpi -t $t -m $method -d $dt -p 0,8,$gain  -c 0,0,1,0:0,1,1,1:1,2,0,3 -c 1,4,2,0:1,5,2,1:2,2,1,6 -c 1,5,0,6 \
                    ${FMUS_DIR}/kinematictruck/engine/engine.fmu \
                    ${FMUS_DIR}/forcevelocitytruck/gearbox/gearbox.fmu \
                    ${FMUS_DIR}/forcevelocitytruck/fvbody/fvbody.fmu  > out.csv

            filename=out-$method-$DT2-$GAIN2.csv
            python3 $COMPARE_CSV out.csv forcevelocity/$filename
            rm out.csv
        done
    done
done
