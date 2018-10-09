ml purge
ml load gimpi/2017b itac/2017.3.030

export PREFIX=$(pwd)/../build/local
alias python=$PREFIX/bin/python3
export PATH=$PREFIX/bin:$PATH
export MANPATH=$PREFIX/man:$MANPATH:
export LD_LIBRARY_PATH=$PREFIX/lib
export PYTHONHOME=$PREFIX
#export PYTHONPATH=$PYTHONPATH:$PREFIX/lib/python3.7/site-packages
export PYTHONPATH=$PREFIX/lib/python3.7/site-packages

