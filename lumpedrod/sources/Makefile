DEPS = Makefile lumped_rod.c lumped_rod.h tridiag_ldlt.c tridiag_ldlt.h sim.c

IOH5LIB = ../../../hdf5
LDFLAGS = -L$(IOH5LIB) -lh5io -lhdf5_cpp -lhdf5 -lm
CXXFLAGS = -g -std=c++11 
CFLAGS = -g -I$(IOH5LIB)  -I/usr/local/include


OBJS  = lumped_rod.o  tridiag_ldlt.o 

%.oct: %.cpp  $(DEPS)
	export CXXFLAGS="-std=c++11 "; mkoctfile  -g -Wcpp $<


%:%.cpp $(DEPS) $(OBJS)
	$(CXX) $(CXXFLAGS)  -g   $< -o $@

sim:sim.c $(DEPS) $(OBJS)
	$(CC) $(CFLAGS)     $<  $(OBJS)  -o $@   $(LDFLAGS)

all:  

clean:
	rm -f *.{o,oct}
