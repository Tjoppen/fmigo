# Set BUILD_DIR if not set
if [ -z "${BUILD_DIR+x}" ]
then
  export BUILD_DIR="$(pwd)/build"
fi

export FMUS_DIR="${BUILD_DIR}/tests/umit-fmus"
export PATH=$BUILD_DIR/install/bin:$PATH
export SERVER=fmigo-server
export MASTER=fmigo-master
export MPI_MASTER=fmigo-mpi
export COMPARE_CSV=$(pwd)/tests/compare_csv.py
export WRAPPER=$(pwd)/tools/wrapper/wrapper.py

if [[ "`uname`" = "Windows_NT" || "`uname`" = "MINGW64"* ]]
then
    export WIN=1
else
    export WIN=0
    #Produce core dumps
    ulimit -c unlimited
    ulimit -n 2048  #should be some value greater than 2*max(Nseq), probably a power of two too
fi
