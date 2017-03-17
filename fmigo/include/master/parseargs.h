#ifndef PARSEARGS_H
#define PARSEARGS_H

#include <fmilib.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <map>

namespace fmitcp_master {

#define DEFAULT_OUTFILE "result.csv"

struct connection {
    connection() {
        slope = 1;
        intercept = 0;
    }
    fmi2_base_type_enu_t fromType;
    int fromFMU;                // Index of FMU
    int fromOutputVR;           // Value reference
    fmi2_base_type_enu_t toType;
    int toFMU;                  // FMU index
    int toInputVR;              // Value reference
    double slope, intercept;    // for unit conversion. y = slope*x + intercept
};

struct strongconnection {
    std::string type;
    int fromFMU;
    int toFMU;
    std::vector<int> vrs;
};

struct param {
    int fmuIndex;
    int valueReference;                 // Value reference to apply to
    fmi2_base_type_enu_t type;
    std::string stringValue;            // String version, always set to what the user wrote
    int intValue;                       // Integer
    double realValue;                   // Real
    int boolValue;                      // Boolean
};

enum FILEFORMAT {
    csv,
    tikz
};

enum METHOD {
    jacobi,
    gs,
    me
};

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
                    std::vector<std::string> *fmuFilePaths,
                    std::vector<connection> *connections,
                    std::map<std::pair<int,fmi2_base_type_enu_t>, std::vector<param> > *params,
                    double* tEnd,
                    double* timeStepSize,
                    jm_log_level_enu_t *loglevel,
                    char* csv_separator,
                    std::string *outFilePath,
                    int* quietMode,
                    enum FILEFORMAT * fileFormat,
                    enum METHOD * method,
                    int * realtimeMode,
                    int * printXML,
                    std::vector<int> *stepOrder,
                    std::vector<int> *fmuVisibilities,
                    std::vector<strongconnection> *strongConnections,
                    std::string *hdf5Filename,
                    std::string *fieldnameFilename,
                    bool *holonomic,
                    double *compliance,
                    int *command_port,
                    int *results_port,
                    bool *paused,
                    bool *solveLoops,
                    bool *useHeadersInCSV
                    );
}

#endif /* PARSEARGS_H */
