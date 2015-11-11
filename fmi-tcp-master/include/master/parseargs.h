#ifndef PARSEARGS_H
#define PARSEARGS_H

#include <fmilib.h>
#include <stdio.h>
#include "master/WeakConnection.h"

namespace fmitcp_master {

#define DEFAULT_OUTFILE "result.csv"

struct connection {
    int fromFMU;                // Index of FMU
    int fromOutputVR;           // Value reference
    int toFMU;                  // FMU index
    int toInputVR;              // Value reference
    fmi2_base_type_enu_t type;
};

struct strongconnection {
    string type;
    int fromFMU;
    int toFMU;
    vector<int> vrs;
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

struct nodesignal {
    std::string node, signal;
};

struct connectionconfig {
    nodesignal input;
    std::vector<nodesignal> outputs;
    bool hasDefault;
    param defaultValue;
};

enum FILEFORMAT {
    csv
};

enum METHOD {
    jacobi,
    gs
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
                    int* loggingOn,
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
                    std::vector<connectionconfig> *connconf,
                    std::string *hdf5Filename
                    );
}

#endif /* PARSEARGS_H */
