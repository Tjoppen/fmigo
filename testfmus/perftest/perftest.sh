set -e
pushd ../../..
source boilerplate.sh
popd

# Designed not to oversubscribe on 8-core machine (ThinkPad W540)
N=7
FMU=${FMUS_DIR}/gsl2/clutch2/clutch2.fmu
FMUS=
CONNS=

for i in $(seq 0 $(python <<< "print($N - 1)"))
do
  for j in $(seq 0 $(python <<< "print($N - 1)"))
  do
    CONNS="$CONNS -c $i,x_e,$j,x_in_e -c $i,v_e,$j,v_in_e -c $i,a_e,$j,force_in_e -c $i,force_e,$j,force_in_ex"
    CONNS="$CONNS -c $i,x_s,$j,x_in_s -c $i,v_s,$j,v_in_s -c $i,a_s,$j,force_in_s -c $i,force_s,$j,force_in_sx"
  done

  FMUS="$FMUS $FMU"
done

#echo $CONNS
#echo $FMUS

echo $(python <<< "print($N * $N * 8)") connections
for method in jacobi gs
do
  echo ---------------------
  echo "MPI, method=$method"
  time mpiexec -np $(python <<< "print($N + 1)") fmigo-mpi -m $method -t 10 -d 0.0005 -a - $FMUS <<< "$CONNS"|sha1sum

  URIS=
  for j in $(seq 1 $N)
  do
    PORT=$(python <<< "print(1023 + $j)")
    fmigo-server -p $PORT $FMU &
    URIS="$URIS tcp://localhost:$PORT"
  done

  echo ---------------------
  echo "ZMQ, method=$method"
  time fmigo-master -m $method -t 10 -d 0.0005 -a - $URIS <<< "$CONNS"|sha1sum
done

