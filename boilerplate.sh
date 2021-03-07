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
export MPIEXEC=mpiexec
export OMPI_MCA_rmaps_base_oversubscribe=1
export OMPI_MCA_btl_base_warn_component_unused=0

if [[ "`uname`" = "Windows_NT" || "`uname`" = "MINGW64"* ]]
then
    export WIN=1
else
    export WIN=0
    #Produce core dumps
    ulimit -c unlimited
    ulimit -n 2048  #should be some value greater than 2*max(Nseq), probably a power of two too
fi

# Grab configuration, for figuring if we have GPL enabled or not
for e in $(fmigo-master -e); do export "$e"; done
