#ifndef MAIN_H
#define MAIN_H

#include <fmilib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
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
#define MAX_LOG_LENGTH 1000
#define MAX_GUID_LENGTH 100

// Command line parsed connection
typedef struct __connection{
    int fromFMU;                // Index of FMU
    int fromOutputVR;           // Value reference
    int toFMU;                  // FMU index
    int toInputVR;              // Value reference
} connection;

// Parsed command line parameter
typedef struct __param{
    int fmuIndex;                       // FMU to apply to
    int valueReference;                 // Value reference to apply to

    // different versions of the parameter, set depending on how the parsing goes
    char stringValue[MAX_PARAM_LENGTH]; // String version, always set to what the user wrote
    int intValue;                       // Integer
    double realValue;                   // Real
    int boolValue;                      // Boolean
} param;

enum FILEFORMAT {
    csv
} fileformat;

enum METHOD {
    jacobi
} method;

typedef int (*stepfunctionType)(double time,                // System stepping function
                                double communicationTimeStep,
                                int numFMUs,
                                fmi1_import_t ** fmus,
                                int numConnections,
                                connection connections[MAX_CONNECTIONS]);

// Runs all FMUs
static int simulate(fmi1_import_t** fmus,                       // FMUs to be simulated
                    char fmuFileNames[MAX_FMUS][PATH_MAX],      // File names of all FMUs
                    int N,                                      // Number of FMUs
                    connection connections[MAX_CONNECTIONS],    // Connections
                    int K,                                      // Number of connections
                    param params[MAX_PARAMS],                   // Parameters
                    int M,                                      // Number of parameters
                    double tEnd,                                // Simulation end time
                    double h,                                   // Time step
                    int loggingOn,                              // FMILibrary logging on
                    char separator,                             // CSV separator
                    jm_callbacks callbacks,                     // FMILibrary callbacks
                    int quiet,                                  // Quiet mode
                    stepfunctionType stepfunc,
                    enum FILEFORMAT outFileFormat               // Out file format
                    );
void importlogger(jm_callbacks* c, jm_string module, jm_log_level_enu_t log_level, jm_string message);
void fmi1Logger(fmi1_component_t c, fmi1_string_t instanceName, fmi1_status_t status, fmi1_string_t category, fmi1_string_t message, ...);
void fmi1StepFinished(fmi1_component_t c, fmi1_status_t status);
int main( int argc, char *argv[] );

#endif /* MAIN_H */
