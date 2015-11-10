##ODESRC = /home/claude/src/ode/ode-0.12
##LDFLAGS = -lhdf5_cpp -lhdf5 -L/usr/local/lib -lode -lm -lpthread
LDFLAGS = -L. -lh5io -lhdf5_cpp -lhdf5  -lboost_regex -lm 
CXXFLAGS = -fPIC -std=c++0x -DMBSFCLIB_HDF -g  
##CXXFLAGS = -g -I$(ODESRC)/ode  -I$(ODESRC)/include  -DdSINGLE 

DEPS = types_abstract.h 

dummy: dummy.cpp
	g++ -o $@  $(CXXFLAGS)  $<   $(LDFLAGS)

converter:  agx_journal_convert.cpp ioh5.o libh5io.a
	g++ $(CXXFLAGS) -o converter agx_journal_convert.cpp ioh5.o $(LDFLAGS)

%: %.cpp    libh5io.a  $(DEPS)
	g++ -o $@  $(CXXFLAGS)  $<   $(LDFLAGS)

ioh5.o: ioh5.cpp ioh5.h  #h5dump.h
#io.o: io.cpp io.h

SRC = ioh5.cpp ioh5.h # io.h io.cpp

OBJS =  ioh5.o  #    io.o
LIBE = libh5io.a

$(LIBE) : $(OBJS) 
	ar -cr $@  $?

%: %.cpp  $(LIBE)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

GAZEBO_SRC = /home/claude/src/gazebo/sandbox/gazebo/deps/opende
install: clean $(LIBE)
	#cp $(SRC) $(ODESRC)/ode
	#cp h5dump.h $(ODESRC)/include/ode/h5dump.h

clean:  
	rm -f *.o *.a

