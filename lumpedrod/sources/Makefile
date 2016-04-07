DEPS = Makefile lumped_rod.c lumped_rod.h tridiag_ldlt.c tridiag_ldlt.h sim.c

LOADLIBES = -lm
CXXFLAGS = -g -std=c++11 
CFLAGS = -g

OBJS  = lumped_rod.o  tridiag_ldlt.o 

%.oct: %.cpp  $(DEPS)
	export CXXFLAGS="-std=c++11 "; mkoctfile  -g -Wcpp $<


%:%.cpp $(DEPS) $(OBJS)
	$(CXX) $(CXXFLAGS)  -g   $< -o $@

sim:sim.c $(DEPS) $(OBJS)
	$(CC) $(CFLAGS)     $<  $(OBJS)  -o $@

all:  

clean:
	rm -f *.{o,oct}
