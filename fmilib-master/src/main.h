#ifndef MAIN_H
#define MAIN_H

#include <fmilib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <regex.h>
#include <sys/stat.h>
#include <limits.h>

#define VERSION "0.1.0"
#define MAX_FMUS 1000
#define MAX_PARAMS 1000
#define MAX_PARAM_LENGTH 1000
#define MAX_CONNECTIONS 1000
#define DEFAULT_ENDTIME 1.0
#define DEFAULT_TIMESTEP 0.1
#define DEFAULT_OUTFILE "result.csv"
#define DEFAULT_CSVSEP ','

typedef struct __connection{
    int fromFMU;
    int fromOutputVR;
    int toFMU;
    int toInputVR;
} connection;

typedef struct __param{
    int fmuIndex;
    int valueReference;
    int valueType; // 0:real, 1:int, 2:bool, 3:string. TODO: make enum

    char stringValue[1000];
    int intValue;
    double realValue;
    int boolValue;
} param;

static int simulate(fmi1_import_t** fmus,
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
                    int quiet);
void importlogger(jm_callbacks* c, jm_string module, jm_log_level_enu_t log_level, jm_string message);
void fmi1Logger(fmi1_component_t c, fmi1_string_t instanceName, fmi1_status_t status, fmi1_string_t category, fmi1_string_t message, ...);
void fmi1StepFinished(fmi1_component_t c, fmi1_status_t status);
int main( int argc, char *argv[] );

#endif /* MAIN_H */
