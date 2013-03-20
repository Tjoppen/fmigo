#ifndef PARSEARGS_H
#define PARSEARGS_H

#include <fmilib.h>
#include <stdio.h>

#include "main.h"

/**
 * @brief Parses the command line arguments.
 * @return int Returns 0 if the program should proceed, 1 if the program should end.
 */
int parseArguments( int argc,
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
                    int* version,
                    enum FILEFORMAT * format,
                    enum METHOD * method,
                    int * realtime,
                    int * printXML);

#endif /* PARSEARGS_H */