#include <stdio.h>

#include "main.h"
#include "help.h"

void printHeader(){
    printf("  FMU CO-SIMULATION MASTER CLI %s\n",VERSION);
}

void printHelp() {
    system("man fmu-master");
}

