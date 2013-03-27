#ifndef PARSEARGS_H
#define PARSEARGS_H

#include <fmilib.h>
#include <stdio.h>

#include "main.h"

/**
 * @brief Parses the command line arguments and stores in the given variable pointer targets.
 * @param argc Given by system
 * @param argv Given by system
 * @param numFMUs
 * @param fmuFilePaths
 * @param numConnections
 * @param connections
 * @param numParameters
 * @param params
 * @param tEnd
 * @param timeStepSize
 * @param loggingOn
 * @param csv_separator
 * @param outFilePath
 * @param outFileGiven
 * @param quietMode
 * @param versionMode
 * @param fileFormat
 * @param method
 * @param realtimeMode
 * @param printXML
 * @param stepOrder
 * @param numStepOrder
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
                    int* quietMode,
                    int* versionMode,
                    enum FILEFORMAT * fileFormat,
                    enum METHOD * method,
                    int * realtimeMode,
                    int * printXML,
                    int stepOrder[MAX_STEP_ORDER],
                    int * numStepOrder);

#endif /* PARSEARGS_H */