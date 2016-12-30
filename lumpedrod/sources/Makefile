DEPS = Makefile  lumped_rod.h  tridiag_ldlt.h 

IOH5LIB = ../../../hdf5
LDFLAGS = -L$(IOH5LIB) -lh5io -lhdf5_cpp -lhdf5 -lm
#CXXFLAGS = -g -std=c++11 
CFLAGS = -fPIC -g -I$(IOH5LIB)  -I/usr/local/include
#CFLAGS =  -g -I$(IOH5LIB)  -I/usr/local/include
#CFLAGS =  -g  -I/usr/local/include


OBJS  =  tridiag_ldlt.o 
LIBE = libtri.a

%:%.cpp $(DEPS) $(OBJS)
	$(CXX) $(CXXFLAGS)  -g   $< -o $@

$(LIBE) : $(OBJS)  
	ar -cr $@  $?

%.o:%.c $(DEPS) 
	$(CC) $(CFLAGS)     $<  -c

%:%.c  $(LIBE)  $(DEPS)  
	$(CC) -DCONSOLE $(CFLAGS) $< $(OBJS)  -o $@  -L. -ltri -lm

all:  

clean:
	rm -f *.{o,oct,a}
