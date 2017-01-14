SERVERPATH=~/work/umit/build/install/bin/
MASTERPATH=~/work/umit/build/install/bin/
FMUPATH= ~/work/umit/data/

all: build cosimulation #run
make: FORCE
	(cd ~/work/umit/buildmake && make > tempbuild && cat tempbuild | grep -v "make\[" | grep -v "\[ ")

build: FORCE
	(cd ~/work/umit/build && ninja install > tempbuild && cat tempbuild | grep -v "Up-to-date")


run:
	(cd ~/work/umit/build && ninja && ninja install  | grep -v Up-to-date | grep -v Building | grep -v Install | grep -v "Set runtime") && mpiexec -np 1 $(MASTERPATH)fmi-mpi-master -t 12 -m me : -np 1 $(SERVERPATH)fmi-mpi-server $(FMUPATH)bouncingBall.fmu

rundebug:
	(( cd ~/work/umit/build && ninja && ninja install | grep -v Up-to-date| grep -v Linking) && mpiexec -np 1 xterm -hold -e gdb --args $(MASTERPATH)fmi-mpi-master -m me -t 10 : -np 1 xterm -hold -e gdb --args $(SERVERPATH)fmi-mpi-server $(FMUPATH)bouncingBall.fmu)

runmpidebug:
	./DEBUG.sh $(MASTERPATH)fmi-mpi-master $(SERVERPATH)fmi-mpi-server $(FMUPATH)vanDerPol.fmu
#	( mpiexec -np 1 $( ./mpidebug.sh $(MASTERPATH)fmi-mpi-master -m me -t 12 ) : -np 1 $( ./mpidebug.sh $(SERVERPATH)fmi-mpi-server $(FMUPATH)vanDerPol.fmu ) )
FORCE:

run_me_test:
	(cd ~/work/umit/build && ninja && ninja install  | grep -v Up-to-date | grep -v Linking | grep -v Install | grep -v "Set runtime")
	pwd
	mpiexec -np 1 $(MASTERPATH)fmi-mpi-master -t 12 -m me : -np 1 $(SERVERPATH)fmi-mpi-server $(FMUPATH)vanDerPol.fmu
	mpiexec -np 1 $(MASTERPATH)fmi-mpi-master -t 12 -m me : -np 1 $(SERVERPATH)fmi-mpi-server $(FMUPATH)bouncingBall.fm
