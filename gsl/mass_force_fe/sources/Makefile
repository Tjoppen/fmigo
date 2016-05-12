CFLAGS = -I ../../../../build/install/include/
mass_force_fe: mass_force_fe.c
	gcc -g $(CFLAGS) mass_force_fe.c ../../../templates/gsl/*.c -DCONSOLE	-I../../../templates/gsl -I../../../templates/fmi2 -lgsl -lgslcblas -lm -Wall -o mass_force_fe
