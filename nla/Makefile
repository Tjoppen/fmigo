CXXFLAGS = -g  -std=c++11  -I.
CFLAGS   = -g -std=c99 -I.
DEPS = qp_solver.h nonsmooth_clutch.h diag4.h banded.h
OBJ = nonsmooth_clutch.o
diag: $(DEPS)
	g++  $(CXXFLAGS) diag.cpp -o diag

.o:.cpp $(DEPS)
	$(CXX) $(CXXFLAGS)  -c  $<
clutch:  $(DEPS) nonsmooth_clutch.o clutch.c 
	$(CXX) $(CXXFLAGS) clutch.c  nonsmooth_clutch.o -o clutch 
