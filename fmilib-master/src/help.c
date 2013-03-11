#include <stdio.h>

#include "main.h"
#include "help.h"

void printHeader(){
    printf("  FMU CO-SIMULATION MASTER CLI %s\n",VERSION);
}

void printHelp(const char* command) {

    printf("\n");
    printHeader();
    printf("\n");
    printf("  USAGE\n\n");
    printf("    %s [FLAGS]... [OPTIONS]... FMUPATHS...\n\n",command);
    printf("  FLAGS\n\n");
    printf("    -h  Show help and quit.\n");
    printf("    -l  Show FMILibrary logs.\n");
    printf("    -q  Quiet mode.\n");
    printf("    -v  Show version and quit.\n");
    printf("\n");
    printf("  OPTIONS\n\n");
    printf("    -c [CONNECTIONS] Connection specification. No connections by default.\n");
    printf("    -d [TIMESTEP]    Timestep size. Default: %g.\n",DEFAULT_TIMESTEP);
    printf("    -o [OUTFILE]     Result output file. Default: %s\n",DEFAULT_OUTFILE);
    printf("    -p [PARAMS]      Parameter specification. No params by default.\n");
    printf("    -s [SEPARATOR]   CSV separator character. Default: %c\n",DEFAULT_CSVSEP);
    printf("    -t [ENDTIME]     End simulation time in seconds. Default: %g.\n",DEFAULT_ENDTIME);
    printf("\n");
    printf("  FMUPATHS\n\n");
    printf("    A space separated list of FMU file paths, relative or absolute. The FMU\n");
    printf("    paths you specity are referred to with integers; 0 for the first one, 1 for\n    the 2nd etc.\n");
    printf("\n");
    printf("  CONNECTIONS\n\n");
    printf("    Quadruples of positive integers, representing which FMU and value reference\n");
    printf("    to connect from and what to connect to. Syntax is CONN1:CONN2:CONN3... where\n");
    printf("    CONNX is four comma-separated integers; FMUFROM,VRFROM,FMUTO,VRTO.\n");
    printf("\n");
    printf("    An example connection string is\n\n      0,0,1,0:0,1,1,1\n\n");
    printf("    which means: connect FMU0 (value reference 0) to FMU1 (vr 0) and FMU0 (vr 1)\n");
    printf("    to FMU1 (vr 1).\n");
    printf("\n");
    printf("  PARAMS\n\n");
    printf("    Triplets separated by :, specifying FMU index, value reference index and\n");
    printf("    parameter value. Example setting the three first value references of FMU 0:\n");
    printf("\n");
    printf("      0,0,12.3:0,1,true:0,2,9\n");
    printf("\n");
    printf("  EXAMPLES\n\n");
    printf("    To run an FMU simulation from time 0 to 5 with timestep 0.01:\n\n");
    printf("      %s -t 5 -d 0.01 ../myFMU.fmu\n",command);
    printf("\n");
    printf("    To simulate two FMUs connected from the first output of the first FMU\n");
    printf("    to the first input of the second:\n\n");
    printf("      %s -c 0,0,1,0 a.fmu b.fmu\n",command);
    printf("\n");
    printf("    To simulate quietly (without output to STDOUT) and save the results\n    to %s:\n\n",DEFAULT_OUTFILE);
    printf("      %s -q -o %s a.fmu\n",command,DEFAULT_OUTFILE);
    printf("\n");
    printf("    To show the help page:\n\n");
    printf("      %s -h\n",command);
    printf("\n");
    printf("  CREDITS\n\n");
    printf("    The app was built by Stefan Hedman at UMIT Research Lab 2013.\n\n");

    exit(EXIT_FAILURE);
}

