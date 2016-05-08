DEPS = Makefile ../diag.cpp ../diag4.h ../diag4qp.h ../Makefile

LOADLIBES = -lm
CXXFLAGS = -g -std=c++11  -I/home/claude/vtb/scania-vtb/new-git/umit/nla/
CFLAGS = -g

%.oct: %.cpp  $(DEPS)
	export CXXFLAGS="${CXXFLAGS}"; mkoctfile  -g -Wcpp $<




%:%.cpp $(DEPS)
	$(CXX) $(CXXFLAGS)  -g   $< -o $@

all:  diagwrap.oct
	./linkdll.sh

clean:
	rm -f *.{o,oct}
