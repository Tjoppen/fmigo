LUMPED_ROD_DIR = ../lumpedrod/sources
LUMPED_ROD_LIB = liblumpedrod.a

H5IO_DIR = ../../hdf5
H5IO_LIB    = libh5io.a

SRCS = \
$(LUMPED_ROD_DIR)/lumped_rod.h \
$(LUMPED_ROD_DIR)/lumped_rod.c \
$(LUMPED_ROD_DIR)/tridiag_ldlt.h \
$(LUMPED_ROD_DIR)/tridiag_ldlt.c

DEPS = Makefile $(LUMPED_ROD_DIR)/$(LUMPED_ROD_LIB) $(H5IO_DIR)/$(H5IO_LIB)   $(SRCS)

CFLAGS = -g -I$(H5IO_DIR) -I$(LUMPED_ROD_DIR)

LDFLAGS = -L$(LUMPED_ROD_DIR) -llumpedrod -L$(H5IO_DIR) -lh5io -lhdf5_cpp -lhdf5 -lboost_regex -lm

CXXFLAGS = -g -std=c++11  -I$(H5IO_DIR)  -I$(LUMPED_ROD_DIR)

$(H5IO_DIR)/$(H5IO_LIB):
	cd $(H5IO_DIR) && make $(H5IO_LIB)

$(LUMPED_ROD_DIR)/$(LUMPED_ROD_LIB):
	cd $(LUMPED_ROD_DIR) && make $(LUMPED_ROD_LIB)

%.oct: %.cpp  $(DEPS)
	export CXXFLAGS="$(CXXFLAGS)" ; mkoctfile  -g -O -Wcpp $(LDFLAGS) $<


%:%.cpp $(DEPS) 
	$(CXX) $(CXXFLAGS)  -g   $< -o $@  $(LDFLAGS)

%:%.c $(DEPS) 
	$(CC) $(CFLAGS)  -g   $< -o $@  $(LDFLAGS)
all:  

clean:
	rm -f *.{o,oct}
