#ifndef PARSEARGS_H
#define PARSEARGS_H

#include <fmilib.h>
#include <stdio.h>

#include "main.h"

int parseArguments2(int argc,
                    char *argv[],
                    int* numFMUs,
                    char fmuFilePaths[MAX_FMUS][PATH_MAX],
                    int* numConnections,
                    connection connections[MAX_CONNECTIONS],
                    int* numParameters,
                    param params[MAX_PARAMS],
                    double* tEnd,
                    double* timeStepSize,
                    int* loggingOn,
                    char* csv_separator,
                    char outFilePath[PATH_MAX],
                    int* outFileGiven,
                    int* quiet,
                    int* version);

void parseArguments(int argc, char *argv[], int* N, fmi1_string_t** fmuFileNames, int* M, int** connections,
                    int* K, fmi1_string_t** params, double* tEnd, double* h, int* loggingOn, char* csv_separator);

#endif /* PARSEARGS_H */