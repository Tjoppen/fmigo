set -e

#for N in 6 12 24 48 96
#for N in 192 384 768
#for N in 200 220 240 255 256 260
#for N in 300 340 380
#for N in 310 320 330
#for N in 340
for N in 768
#for N in 1536
do
  D=fmigo-kinematic-N$N
  mkdir -p $D
  cp abisko-perftest3.sh perftest3.sh $D
  (
  cd $D
  sed -i -e "s/-n 6/-n $N/;s|../../../../../|../../../../../../|" abisko-perftest3.sh
  sed -i -e "s/^n=6/n=$N/;s|pushd ../../../..|pushd ../../../../..|;s|../../../../buildi/fmigo-mpi|../../../../../buildi/fmigo-mpi|" perftest3.sh
  sbatch abisko-perftest3.sh
  )
done

