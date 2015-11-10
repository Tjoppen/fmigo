LDFLAGS = -L. -lh5io -lhdf5_cpp -lhdf5  -lboost_regex -lm 
CXXFLAGS = -fPIC -std=c++0x  -g  


%: %.cpp    libh5io.a  $(DEPS)
	g++ -o $@  $(CXXFLAGS)  $<   $(LDFLAGS)

SRC = ioh5.cpp ioh5.h 

OBJS =  ioh5.o  
LIBE = libh5io.a

$(LIBE) : $(OBJS) 
	ar -cr $@  $?

%: %.cpp  $(LIBE)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

clean:  
	rm -f *.o *.a

