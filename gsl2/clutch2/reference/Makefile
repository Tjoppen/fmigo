DEPS = Makefile  

LOADLIBES = -lm
CXXFLAGS = -g -std=c++11  
FLAGS = -g

clutchcurve.oct:clutchcurve.cpp  $(DEPS)
	export CXXFLAGS="$(CXXFLAGS)" ; mkoctfile  -g -O -Wcpp $(LDFLAGS) $<

%.oct: %.cpp  $(DEPS)
	export CXXFLAGS="$(CXXFLAGS)" ; mkoctfile  -g -O -Wcpp $(LDFLAGS) $<

clean:
	rm -f *.{o,oct}
