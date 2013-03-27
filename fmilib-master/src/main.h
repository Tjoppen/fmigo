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

#define VERSION "0.1.2"
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
#define MAX_STEP_ORDER 1000


/**
 * @struct __connection
 * @brief Storage struct for a parsed connection
 * @var connection::fromFMU
 * @brief Index of the FMU that the connection goes from.
 * @var connection::toFMU
 * @brief Index of the FMU that the connection goes to.
 * @var connection::fromOutputVR
 * @brief Value reference of the variable to connect from.
 * @var connection::toInputVR
 * @brief Value reference of the variable to connect to.
 */
typedef struct __connection{
    int fromFMU;                // Index of FMU
    int fromOutputVR;           // Value reference
    int toFMU;                  // FMU index
    int toInputVR;              // Value reference
} connection;

/**
 * @struct __param
 * @brief Storage struct for a parsed parameter
 * @var param::fmuIndex
 * @brief The index of the FMU to apply the parameter on. The index refers to the position in the fmus array
 * @var param::valueReference
 * @brief Value reference of the parameter
 * @var param::stringValue
 * @brief String version of the parsed parameter.
 * @var param::intValue
 * @brief The integer version of it, if it could be parsed as an int. Otherwise zero.
 * @var param::realValue
 * @brief The real version of it, if it could be parsed as a real. Otherwise zero.
 * @var param::boolValue
 * @brief The boolean version of it, if it could be parsed as a boolean. Otherwise zero.
 */
typedef struct __param{
    int fmuIndex;
    int valueReference;                 // Value reference to apply to
    char stringValue[MAX_PARAM_LENGTH]; // String version, always set to what the user wrote
    int intValue;                       // Integer
    double realValue;                   // Real
    int boolValue;                      // Boolean
} param;

enum FILEFORMAT {
    csv
} fileformat;

enum METHOD {
    jacobi,
    gs
} method;

/**
 * @brief Stepping function signature, for global Master stepping functions such as Jacobi and Gauss-Seidel
 * @return int Zero if successful, otherwise error code.
 */
typedef int (*fmi1stepfunction)(double time,                // System stepping function
                                double communicationTimeStep,
                                int numFMUs,
                                fmi1_import_t ** fmus,
                                int numConnections,
                                connection connections[MAX_CONNECTIONS],
                                int numStepOrder,
                                int stepOrder[MAX_STEP_ORDER]);
/**
 * @todo Implement me!
 * @return int Zero if successful, otherwise error code.
 */
typedef int (*fmi2stepfunction)(double time,                
                                double communicationTimeStep,
                                int numFMUs,
                                fmi2_import_t ** fmus,
                                int numConnections,
                                connection connections[MAX_CONNECTIONS],
                                int numStepOrder,
                                int stepOrder[MAX_STEP_ORDER]);

void importlogger(jm_callbacks* c, jm_string module, jm_log_level_enu_t log_level, jm_string message);
void fmi1Logger(fmi1_component_t c, fmi1_string_t instanceName, fmi1_status_t status, fmi1_string_t category, fmi1_string_t message, ...);
void fmi1StepFinished(fmi1_component_t c, fmi1_status_t status);
int main( int argc, char *argv[] );

#endif /* MAIN_H */
