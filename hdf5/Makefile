LDLIBES = -L. -lh5io -lhdf5_cpp -lhdf5  -lboost_regex -lm 
CXXFLAGS = -fPIC -std=c++0x  -g  



#%: %.cpp    libh5io.a  $(DEPS)
#	g++ -o $@  $(CXXFLAGS)  $<   $(LDFLAGS)

%:%.cpp    $(LIBE) $(DEPS) Makefile
	$(CXX) $(CXXFLAGS) $< -o $@    $(LDLIBES)

.o:.cpp
	g++ -c -o $@ $(CXXFLAGS)  $<  
SRC = ioh5.cpp ioh5.h 

OBJS =  ioh5.o   
LIBE = libh5io.a
DEPS = ioh5.h ioh5.c

$(LIBE): $(OBJS) 
	ar -cr $@  $?


clean:  
	rm -f *.o *.a

