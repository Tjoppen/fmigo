SERVER=~/work/umit/build/install/bin/fmi-mpi-server
MASTER=~/work/umit/build/install/bin/fmi-mpi-master
FMU=~/work/umit/umit-fmus/gsl2/
DATA=~/work/umit/data/
SHELL := /bin/bash

BOUNCINGBALL= -np 1 $(SERVER) $(FMU)bouncingBall/bouncingBall.fmu
BOUNCINGBALLOLD= -np 1 $(SERVER) $(DATA)bouncingBall.fmu
BOUNCINGBALLWITHSPRING= -np 1 $(SERVER) $(FMU)bouncingBallWithSpring/bouncingBallWithSpring.fmu
FIXEDPOINT= -np 1 $(SERVER) $(FMU)fixedPoint/fixedPoint.fmu
VANDERPOL= -np 1 $(SERVER) $(FMU)vanDerPol/vanDerPol.fmu
SPRINGS= -np 1 $(SERVER) $(FMU)springs/springs.fmu
SUBME= -np 1 $(SERVER) $(FMU)subME/subME.fmu

DEBUGG= $(FMU)fixedPoint/fixedPoint.fmu
DEBUGG= $(FMU)vanDerPol/vanDerPol.fmu
DEBUGG= $(DATA)bouncingBall.fmu
DEBUGG= $(FMU)bouncingBall/bouncingBall.fmu

makeplot: run_me_test plot

generate:
	pushd ~/work/umit/umit-fmus/ && ./generate_everything.sh && popd

plot:
	pkill python || echo ""
	python $(DATA)plot1.py $(DATA)bouncingball.mat
	python $(DATA)plot1.py $(DATA)vanderpol.mat der

FORCE:
build:FORCE
	(cd ~/work/umit/build && ninja && ninja install  | 		\
					    grep -v Up-to-date | 	\
					    grep -v Linking | 		\
					    grep -v Install | 		\
					    grep -v "Set runtime")

rundebug: build
	mpiexec -np 1 xterm -hold -e gdb --args $(MASTER) -m me -t 10 : -np 1 xterm -hold -e gdb --args $(SERVER) $(DEBUGG)

debug: build
	./mpiemacsgdb.sh $(MASTER) $(SERVER) $(DEBUGG)

run_me_two: generate build
	mpiexec -np 1 $(MASTER) -t 12 -m me : $(FIXEDPOINT) # : $(BOUNCINGBALL)
	#mpiexec -np 1 $(MASTER) -t 12 -m me -p r,0,301,1 -c 0,1,1,1 : $(FIXEDPOINT) :  $(BOUNINGBALLWITHSPRING)
	#(cd $(FMU) && mv resultFile.mat ballspringfixed.mat)
	#mpiexec -np 1 $(MASTER) -t 12 -m me -p r,1,11,1 -c 0,2,1,1 : $(SPRINGS) :  $(SPRINGS)
	#(cd $(FMU) && mv resultFile.mat springs.mat)

run_me_test: build
	mpiexec -np 1 $(MASTER) -t 3 -m me : $(BOUNCINGBALLOLD)
	(cd $(DATA) && mv resultFile.mat bouncingballOld.mat)
	mpiexec -np 1 $(MASTER) -t 3 -m me : $(BOUNCINGBALL)
	(cd $(DATA) && mv resultFile.mat bouncingball.mat)
	mpiexec -np 1 $(MASTER) -t 12 -m me : $(VANDERPOL)
	(cd $(DATA) && mv resultFile.mat vanderpol.mat)
	mpiexec -np 1 $(MASTER) -t 12 -m me -p r,1,11,1 -c 0,2,1,1 : $(SPRINGS) :  $(SPRINGS)
	(cd $(FMU) && mv resultFile.mat springs.mat)

valgrind: build
	mpiexec -np 1 valgrind -v --leak-check=full --show-leak-kinds=all $(MASTER) -t 0.1 -m me : -np 1 $(SERVER) $(DEBUGG)

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
