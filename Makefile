SERVERPATH=~/work/umit/build/install/bin/
MASTERPATH=~/work/umit/build/install/bin/
FMUPATH=~/work/umit/data/

BOUNCINGBALL= -np 1 $(SERVERPATH)fmi-mpi-server $(FMUPATH)bouncingBall.fmu
VANDERPOL= -np 1 $(SERVERPATH)fmi-mpi-server $(FMUPATH)vanDerPol.fmu
VANDERPOL2= -np 1 $(SERVERPATH)fmi-mpi-server $(FMUPATH)vanDerPol2.fmu
all: build cosimulation #run
make: FORCE
	(cd ~/work/umit/buildmake && make > tempbuild && cat tempbuild | grep -v "make\[" | grep -v "\[ ")

build: FORCE
	(cd ~/work/umit/build && ninja install > tempbuild && cat tempbuild | grep -v "Up-to-date")

makeplot: run_me_test plot

plot:
	pkill python || echo ""
	python $(FMUPATH)plot1.py $(FMUPATH)bouncingball.mat
	python $(FMUPATH)plot1.py $(FMUPATH)vanderpol.mat der

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
	mpiexec -np 1 $(MASTERPATH)fmi-mpi-master -t 3 -m me : $(BOUNCINGBALL)
	(cd $(FMUPATH) && mv resultFile.mat bouncingball.mat)
	mpiexec -np 1 $(MASTERPATH)fmi-mpi-master -t 12 -m me : $(VANDERPOL)
	(cd $(FMUPATH) && mv resultFile.mat vanderpol.mat)
	#mpiexec -np 1 $(MASTERPATH)fmi-mpi-master -t 12 -m me : $(VANDERPOL) : $(VANDERPOL2)
	#mpiexec -np 1 $(MASTERPATH)fmi-mpi-master -t 1.5 -m me : $(BOUNCINGBALL) : $(BOUNCINGBALL)
	#mpiexec -np 1 $(MASTERPATH)fmi-mpi-master -t 1.6 -m me : $(VANDERPOL) : $(BOUNCINGBALL)

valgrind:
	(cd ~/work/umit/build && ninja && ninja install  | grep -v Up-to-date | grep -v Building | grep -v Install | grep -v "Set runtime") && mpiexec -np 1 $(MASTERPATH)fmi-mpi-master -t 12 -m me : -np 1 valgrind -v --leak-check=full $(SERVERPATH)fmi-mpi-server $(FMUPATH)bouncingBall.fmu

FLAGS=-D DEBUG
FMUGOHPP=~/work/umit/fmi-tcp-master/src/common/fmu_go_storage.hpp
FMUGOH=~/work/umit/fmi-tcp-master/include/common/fmu_go_storage.h
TESTFILE=~/work/umit/fmi-tcp-master/src/common/testStorage.cpp
LINKFILES=~/work/umit/fmi-tcp-master/src/common/fmu_go_storage.cpp $(FLAGS)
LOOP=1 1 1 1 1 1 1 1 1
fmugostorageLOOP: merge_hpp_with_h
	$(foreach var,$(LOOP),g++ -std=c++11 $(TESTFILE) $(LINKFILES) -o foo && ./foo && rm foo;)

fmugostorage: merge_hpp_with_h
	g++ -std=c++11 $(TESTFILE) $(LINKFILES) -o foo && ./foo && rm foo

fmugostorageval: merge_hpp_with_h
	g++ -g -std=c++11 $(TESTFILE) $(LINKFILES) -o foo && valgrind --leak-check=full --show-leak-kinds=all ./foo && rm foo

fmugostoragegdb: merge_hpp_with_h
	g++ -g -std=c++11 $(TESTFILE) $(LINKFILES) -o foo && gdb -ex run ./foo && rm foo

merge_hpp_with_h:
	cp $(FMUGOH) $(FMUGOHPP)
