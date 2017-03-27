DEPS = Makefile  ../diag4.h ../banded.h ../qp_solver.h ../Makefile ../nonsmooth_clutch.h ../nonsmooth_clutch.cpp

LOADLIBES = -lm
CXXFLAGS = -g -std=c++11  -I/home/claude/vtb/umit/nla/
FLAGS = -g

diagwrap.oct:diagwrap.cpp  $(DEPS)
	export CXXFLAGS="$(CXXFLAGS)" ; mkoctfile  -g -O -Wcpp $(LDFLAGS) $<

%.oct: %.cpp  $(DEPS)
	export CXXFLAGS="$(CXXFLAGS)" ; mkoctfile  -g -O -Wcpp $(LDFLAGS) $<

%:%.cpp $(DEPS)
	$(CXX) $(CXXFLAGS)  -g   $< -o $@

all:  diagwrap.oct
	./linkdll.sh

clean:
	rm -f *.{o,oct}
