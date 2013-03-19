#ifndef SIMULATE_H
#define SIMULATE_H

#include <fmilib.h>
#include <stdio.h>

#include "main.h"

void setInitialValues(fmi1_import_t* fmu);

// Set initial values from the command line, overrides the XML init values
void setParams(int numFMUs, int numParams, fmi1_import_t ** fmus, param params[MAX_PARAMS]);

/**
 * Simulate the given FMUs.
 * Returns 0 on success, else an error code.
 */
int simulate(fmi1_import_t** fmus,
             char fmuFileNames[MAX_FMUS][PATH_MAX],
             int N,
             connection connections[MAX_CONNECTIONS],
             int K,
             param params[MAX_PARAMS],
             int M,
             double tEnd,
             double h,
             int loggingOn,
             char separator,
             jm_callbacks callbacks,
             int quiet,
             stepfunctionType stepfunc,
             enum FILEFORMAT outFileFormat,
             char outFile[PATH_MAX],
             int realTimeMode,
             int * numSteps);

#endif /* SIMULATE_H */