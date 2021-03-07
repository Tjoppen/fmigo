set -e
pushd ../../../..
source boilerplate.sh
popd

# Designed not to oversubscribe on 8-core machine (ThinkPad W540)
N=7
FMU=${FMUS_DIR}/gsl/clutch/clutch.fmu
FMUS=
CONNS=

for i in $(seq 0 $(python3 <<< "print($N - 1)"))
do
  for j in $(seq 0 $(python3 <<< "print($N - 1)"))
  do
    CONNS="$CONNS -c $i,x_e,$j,x_in_e -c $i,v_e,$j,v_in_e -c $i,a_e,$j,force_in_e -c $i,force_e,$j,force_in_ex"
    CONNS="$CONNS -c $i,x_s,$j,x_in_s -c $i,v_s,$j,v_in_s -c $i,a_s,$j,force_in_s -c $i,force_s,$j,force_in_sx"
  done

  FMUS="$FMUS $FMU"
done

#echo $CONNS
#echo $FMUS

echo $(python3 <<< "print($N * $N * 8)") connections
for method in jacobi #gs
do
  #echo ---------------------
  #echo "MPI, method=$method"
 if true
 then
  (for x in `seq 1 11`
  do
   time ${MPIEXEC} -np $(python3 <<< "print($N + 1)") fmigo-mpi -m $method -t 10 -d 0.0005 -f none -l 2 -a - $FMUS <<< "$CONNS"
  done) 2>&1 | grep real | sort
 fi

 if false
 then
   # real 0m41,211s
   for x in `seq 1 5`
   do
   time perf record -a -F 5000 -g -- ${MPIEXEC} -np $(python3 <<< "print($N + 1)") fmigo-mpi -m $method -t 10 -d 0.0005 -f none -a - $FMUS <<< "$CONNS"
   done
   perf script | ~/FlameGraph/stackcollapse-perf.pl > out.perf-folded-2
   #~/FlameGraph/flamegraph.pl out.perf-folded-2 > perf-fmigo-mpi.svg
   ~/FlameGraph/difffolded.pl out.perf-folded out.perf-folded-2 | ~/FlameGraph/flamegraph.pl > perf-fmigo-mpi.svg
 fi

 if false
 then
  URIS=
  for j in $(seq 1 $N)
  do
    PORT=$(python3 <<< "print(1023 + $j)")
    if [ $j -eq 1 ]
    then
        valgrind --tool=callgrind --callgrind-out-file=fmigo-server.callgrind fmigo-server -p $PORT $EXTRA -l 4 $FMU &
        #fmigo-server -p $PORT $EXTRA $FMU &
    else
        fmigo-server -p $PORT $EXTRA $FMU &
    fi
    URIS="$URIS tcp://localhost:$PORT"
  done

  #echo ---------------------
  echo "ZMQ, method=$method"
  time valgrind --tool=callgrind --callgrind-out-file=fmigo-master-2.callgrind  fmigo-master -l 4 -m $method -t 5 -d 0.0005 -f none -a - $URIS <<< "$CONNS"
  #time fmigo-master -l 4 -m $method -t 50 -d 0.0005 -f none -a - $URIS <<< "$CONNS"
  # real 0m17,574s
  #time perf record -a -F 999 -g -- fmigo-master -l 4 -m $method -t 5 -d 0.0005 -f none -a - $URIS <<< "$CONNS"
  #perf script | ~/FlameGraph/stackcollapse-perf.pl > out.perf-folded
  #~/FlameGraph/flamegraph.pl out.perf-folded > perf-fmigo-master.svg
 fi
done

echo
#echo mpi-speed-test comparison:
#time ${MPIEXEC} -np $(python3 <<< "print($N + 1)") mpi-speed-test
