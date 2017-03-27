SRC = ioh5.cpp ioh5.h 
OBJS =  ioh5.o   
LIBE = libh5io.a
DEPS = ioh5.h ioh5.c
LDLIBES = -L. -lh5io -lhdf5_cpp -lhdf5  -lboost_regex -lm 
CXXFLAGS = -fPIC -std=c++0x  -g  



#%: %.cpp    libh5io.a  $(DEPS)
#	g++ -o $@  $(CXXFLAGS)  $<   $(LDFLAGS)

%:%.cpp    $(LIBE) $(DEPS) Makefile
	$(CXX) $(CXXFLAGS) $< -o $@    $(LDLIBES)

.o:.cpp
	g++ -c -o $@ $(CXXFLAGS)  $<  


%:%.c
	gcc -g -o $@ $<  -lhdf5 -lm
$(LIBE): $(OBJS) 
	ar -cr $@  $?

h5: h5.cpp ioh5c.h
	g++ -o $@ $(CXXFLAGS)  $<  $(LDLIBES)

clean:  
	rm -f *.o *.a

