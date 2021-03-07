set -e
pushd ../../../..
source boilerplate.sh
popd

# Designed not to oversubscribe on 8-core machine (ThinkPad W540, granular)
n=6
N=$(bc <<< "$n - 1")
FMU=${FMUS_DIR}/gsl/clutch/clutch.fmu
FMUS="$FMU"
CONNS=

for i in $(seq 0 $(python3 <<< "print($N - 2)"))
do
  ip1=$(python3 <<< "print($i + 1)")
  CONNS="$CONNS -C shaft,$i,$ip1,x_s,v_s,a_s,force_in_s,x_e,v_e,a_e,force_in_e"
  FMUS="$FMUS $FMU"
done

echo $(python3 <<< "print($N - 1)") kinematic connections

# Walltime test
# 100k steps
if true
then
  #(
  #for x in `seq 1 11`
  #do
   # 100k steps
   time ${MPIEXEC} -np $(python3 <<< "print($N + 1)") ../../../../buildi/fmigo-mpi -t 1 -d 0.0001 -f none -l 2 -a - $FMUS <<< "$CONNS"
  #done
  #) 2>&1 | grep real | sort
fi

# Valgrind slow awesomeness
# 10k steps
if false
then
  URIS=
  for j in $(seq 1 $N)
  do
    PORT=$(python3 <<< "print(1023 + $j)")
    if [ $j -eq 1 ]
    then
        valgrind --tool=callgrind --callgrind-out-file=fmigo-server-2.callgrind fmigo-server -p $PORT $EXTRA -l 4 $FMU &
    else
        fmigo-server -p $PORT $EXTRA $FMU &
    fi
    URIS="$URIS tcp://localhost:$PORT"
  done

  # 10k steps
  time valgrind --tool=callgrind --callgrind-out-file=fmigo-master-2.callgrind fmigo-master -l 4 -t 5 -d 0.0005 -f none $CONNS $URIS
  #time fmigo-master -l 4 -t 50 -d 0.0005 -f none -a - $URIS <<< "$CONNS"
  # real 0m17,574s
  #time perf record -a -F 999 -g -- fmigo-master -l 4 -t 5 -d 0.0005 -f none -a - $URIS <<< "$CONNS"
  #perf script | ~/FlameGraph/stackcollapse-perf.pl > out.perf-folded
  #~/FlameGraph/flamegraph.pl out.perf-folded > perf-fmigo-master.svg
fi

# Flamegraph test
if false
then
   # real 0m41,211s
   for x in `seq 1 5`
   do
   time perf record -a -F 5000 -g -- ${MPIEXEC} -np $(python3 <<< "print($N + 1)") fmigo-mpi -t 10 -d 0.0005 -f none -a - $FMUS <<< "$CONNS"
   done
   perf script | ~/FlameGraph/stackcollapse-perf.pl > out.perf-folded-2
   #~/FlameGraph/flamegraph.pl out.perf-folded-2 > perf-fmigo-mpi.svg
   ~/FlameGraph/difffolded.pl out.perf-folded out.perf-folded-2 | ~/FlameGraph/flamegraph.pl > perf-fmigo-mpi.svg
fi

#echo mpi-speed-test comparison:
#time ${MPIEXEC} -np $(python3 <<< "print($N + 1)") mpi-speed-test
