# Build the .fmu file for co-simulation

LOADLIBES = -lgsl -lgslcblas -lm 
CFLAGS = -g -std=c99

OBJS = gsl-interface.o  
DEPS = gsl-interface.h 

PIC = -fPIC

objs: $(OBJS)

%.o:%.c $(DEPS)
	$(CC) $(CFLAGS) -c  $(PIC) -Wall $(INCLUDE)  $< -o $@

%:%.c  $(DEPS) $(OBJS)
	$(CC) $(CFLAGS)  $(PIC) -Wall $(INCLUDE)  $< $(OBJS)  -o $@ $(LOADLIBES)
%.so: %.o
	$(CC) $(CBITSFLAGS) -shared -Wl,-soname,$@ -o $@ $<

clean:
	rm -rf *.so *.dylib *.o *~
