ml purge
# GCC, Intel MPI and traceanalyzer
#ml load gimpi/2018b itac/2018.1.017
# cmake won't work if compiled with gimpi/2018b, with this error:
# /usr/lib/x86_64-linux-gnu/libstdc++.so.6: version `GLIBCXX_3.4.22' not found (required by ./cmake)
ml load gimpi/2017b itac/2018.1.017

export PREFIX=/pfs/nobackup/home/t/thardin/local
alias python=$PREFIX/bin/python3
export PATH=$PREFIX/bin:$PATH
export MANPATH=$PREFIX/man:$MANPATH:
export LD_LIBRARY_PATH=$PREFIX/lib
export PYTHONHOME=$PREFIX
#export PYTHONPATH=$PYTHONPATH:$PREFIX/lib/python3.7/site-packages
export PYTHONPATH=$PREFIX/lib/python3.7/site-packages

