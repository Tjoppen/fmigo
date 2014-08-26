#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <getopt.h>
#include <deque>
#include "common/common.h"

#include "master/parseargs.h"

using namespace fmitcp_master;

static void printHelp(){
    printf("Usage\n\
\n\
master [OPTIONS] [FMU_URLS...]\n\
\n\
OPTIONS\n\
\n\
    --timeStep [NUMBER]\n\
            Timestep size. Default is 0.1.\n\
\n\
    --stopAfter [NUMBER]\n\
        End simulation time in seconds. Default is 1.0.\n\
\n\
    --weakMethod [STRING]\n\
        Stepping  method for weak coupling connections. Must be \"parallel\" or \"serial\". Default is \"parallel\".\n\
\n\
    --weakConnections [STRING]\n\
        Connection  specification. No connections by default. Quadruples of\n\
        positive integers, representing which FMU and value reference to connect\n\
        from and what to connect to. Syntax is\n\
\n\
            CONN1:CONN2:CONN3...\n\
\n\
        where CONNX is four comma-separated integers FMUFROM,VRFROM,FMUTO,VRTO.\n\
        An example connection string is\n\
\n\
            0,0,1,0:0,1,1,1\n\
\n\
        which means: connect FMU0 (value reference 0) to FMU1 (vr 0) and FMU0\n\
        (vr 1) to FMU1 (vr 1).  Default is no  connections.\n\
\n\
    --strongConnections [STRING]\n\
        TODO\n\
\n\
    --help\n\
        You're looking at it.\n\
\n\
FMU_URLS\n\
\n\
    Urls to FMU servers, separated by spaces. For example \"tcp://fmiserver.com:1234\".\n\
\n\
EXAMPLES\n\
\n\
    master --weakConnections 0,0,0,0:0,0,0,0 tcp://localhost:3000\n\n");
}

static void printInvalidArg(char option){
    fprintf(stderr, "Invalid argument of -%c. Use -h for help.\n",option);
}

static fmi2_base_type_enu_t type_from_char(char type) {
    switch (type) {
    default:
    case 'r': return fmi2_base_type_real;
    case 'i': return fmi2_base_type_int;
    case 'b': return fmi2_base_type_bool;
    case 's': return fmi2_base_type_str;
    }
}

template<typename T> int checkFMUIndex(T it, int i, std::vector<std::string> *fmuFilePaths) {
    if(it->fromFMU < 0 || it->fromFMU >= fmuFilePaths->size()){
        fprintf(stderr,"Connection %d connects from FMU %d, which does not exist.\n", i, it->fromFMU);
        return 1;
    }
    if(it->toFMU < 0 || it->toFMU >= fmuFilePaths->size()){
        fprintf(stderr,"Connection %d connects to FMU %d, which does not exist.\n", i, it->toFMU);
        return 1;
    }

    return 0;
}

int fmitcp_master::parseArguments( int argc,
                    char *argv[],
                    std::vector<std::string> *fmuFilePaths,
                    std::vector<connection> *connections,
                    map<pair<int,fmi2_base_type_enu_t>, vector<param> > *params,
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
                    vector<strongconnection> *strongConnections) {
    int index, c;
    opterr = 0;

    while ((c = getopt (argc, argv, "xrlvqht:c:d:s:o:p:f:m:g:w:C:")) != -1){
        int n, skip, l, cont, i, numScanned, stop, vis;
        deque<string> parts;
        if (optarg) parts = split(optarg, ':');

        switch (c) {

        case 'c':
            for (auto it = parts.begin(); it != parts.end(); it++) {
                connection conn;
                deque<string> values = split(*it, ',');

                if (values.size() == 5) {
                    if (values[0].size() != 1) {
                        fprintf(stderr, "Bad type: %s\n", values[0].c_str());
                        return 1;
                    }

                    conn.type     = type_from_char(values[0][0]);
                    values.pop_front();
                } else if (values.size() == 4) {
                    conn.type     = type_from_char('r');
                } else {
                    fprintf(stderr, "Bad param: %s\n", it->c_str());
                    return 1;
                }

                conn.fromFMU      = atoi(values[0].c_str());
                conn.fromOutputVR = atoi(values[1].c_str());
                conn.toFMU        = atoi(values[2].c_str());
                conn.toInputVR    = atoi(values[3].c_str());

                connections->push_back(conn);
            }
            break;

        case 'C':
            //strong connections
            for (auto it = parts.begin(); it != parts.end(); it++) {
                deque<string> values = split(*it, ',');
                strongconnection sc;

                if (values.size() < 3) {
                    fprintf(stderr, "Bad strong connection specification: %s\n", it->c_str());
                    exit(1);
                }

                sc.type    = values[0];
                sc.fromFMU = atoi(values[1].c_str());
                sc.toFMU   = atoi(values[2].c_str());

                for (auto it2 = values.begin() + 3; it2 != values.end(); it2++) {
                    sc.vrs.push_back(atoi(it2->c_str()));
                }

                strongConnections->push_back(sc);
            }
            break;

        case 'd':
            numScanned = sscanf(optarg,"%lf", timeStepSize);
            if(numScanned <= 0){
                printInvalidArg(c);
                return 1;
            }
            break;

        case 'f':
            if(strcmp(optarg,"csv") == 0){
                *fileFormat = csv;
            } else {
                fprintf(stderr,"File format \"%s\" not recognized.\n",optarg);
                return 1;
            }
            break;

        case 'l':
            *loggingOn = 1;
            break;

        case 'm':
            if(strcmp(optarg,"jacobi") == 0){
                *method = jacobi;
            } else if(strcmp(optarg,"gs") == 0){
                *method = gs;
            } else {
                fprintf(stderr,"Method \"%s\" not recognized. Use \"jacobi\" or \"gs\".\n",optarg);
                return 1;
            }
            break;
            
        case 't':
            numScanned = sscanf(optarg, "%lf", tEnd);
            if(numScanned <= 0){
                printInvalidArg(c);
                return 1;
            }
            break;

        case 'g':
            // Step order spec
            n=0;
            skip=0;
            l=strlen(optarg);
            cont=1;
            i=0;
            int scannedInt;
            stop = 2;
            while(cont && (n=sscanf(&optarg[skip],"%d", &scannedInt))!=-1 && skip<l){
                // Now skip everything before the n'th comma
                char* pos = strchr(&optarg[skip],',');
                if(pos==NULL){
                    stop--;
                    if(stop == 0){
                        cont=0;
                        break;
                    }
                    stepOrder->push_back(scannedInt);
                    break;
                }
                stepOrder->push_back(scannedInt);
                skip += pos-&optarg[skip]+1; // Dunno why this works... See http://www.cplusplus.com/reference/cstring/strchr/
                i++;
            }
            break;

        case 'h':
            printHelp();
            return 1;

        case 'r':
            *realtimeMode = 1;
            break;

        case 's':
            if(strlen(optarg)==1 && isprint(optarg[0])){
                *csv_separator = optarg[0];
            } else {
                printInvalidArg('s');
                return 1;
            }
            break;

        case 'o':
            *outFilePath = optarg;
            break;

        case 'q':
            *quietMode = 1;
            break;

        case 'v':
            printf("%s\n",FMITCPMASTER_VERSION);
            return 1;

        case 'p':
            for (auto it = parts.begin(); it != parts.end(); it++) {
                param p;
                deque<string> values = split(*it, ',');

                //expect [type,]FMU,VR,value
                if (values.size() == 4) {
                    if (values[0].size() != 1) {
                        fprintf(stderr, "Bad type: %s\n", values[0].c_str());
                        return 1;
                    }

                    p.type       = type_from_char(values[0][0]);
                    values.pop_front();
                } else if (values.size() == 3) {
                    p.type       = type_from_char('r');
                } else {
                    fprintf(stderr, "Bad param: %s\n", it->c_str());
                    return 1;
                }

                p.fmuIndex       = atoi(values[0].c_str());
                p.valueReference = atoi(values[1].c_str());

                switch (p.type) {
                case fmi2_base_type_real: p.realValue = atof(values[2].c_str()); break;
                case fmi2_base_type_int:  p.intValue = atoi(values[2].c_str()); break;
                case fmi2_base_type_bool: p.boolValue = (values[2] == "true"); break;
                case fmi2_base_type_str:  p.stringValue = values[2]; break;
                }

                (*params)[make_pair(p.fmuIndex,p.type)].push_back(p);
            }
            break;

        case 'x':
            *printXML = 1;
            break;

        case 'w':
            //visibilities
            for (auto it = parts.begin(); it != parts.end(); it++) {
                fmuVisibilities->push_back(atoi(it->c_str()));
            }
            break;

        case '?':

            if(isprint(optopt)){
                if(strchr("cdsopfm", optopt)){
                    fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                } else {
                    fprintf (stderr, "Unknown option: -%c\n", optopt);
                }
            } else {
                fprintf (stderr, "Unknown option character: \\x%x\n", optopt);
            }
            return 1;

        default:
            printf("abort %c...\n",c);
            return 1;
        }
    }

    // Parse FMU paths in the end of the command line
    for (index = optind; index < argc; index++) {
        fmuFilePaths->push_back(argv[index]);
    }

    if (fmuFilePaths->size() == 0){
        fprintf(stderr, "No FMUs given. Aborting... (see -h for help)\n");
        return 1;
    }

    // Check if connections refer to nonexistant FMU index
    int i = 0;
    for (auto it = connections->begin(); it != connections->end(); it++, i++) {
        if (checkFMUIndex(it, i, fmuFilePaths))
            return 1;
    }

    i = 0;
    for (auto it = strongConnections->begin(); it != strongConnections->end(); it++, i++) {
        if (checkFMUIndex(it, i, fmuFilePaths))
            return 1;
    }

    // Check if parameters refer to nonexistant FMU index
    i = 0;
    for (auto it = params->begin(); it != params->end(); it++, i++) {
        if(it->first.first < 0 || it->first.first >= fmuFilePaths->size()){
            fprintf(stderr,"Parameter %d refers to FMU %d, which does not exist.\n", i, it->first.first);
            return 1;
        }
    }

    // Default step order is all FMUs in their current order
    if (stepOrder->size() == 0){
        for(c=0; c<fmuFilePaths->size(); c++){
            stepOrder->push_back(c);
        }
    } else if (stepOrder->size() != fmuFilePaths->size()) {
        fprintf(stderr, "Step order/FMU count mismatch: %zu vs %zu\n", stepOrder->size(), fmuFilePaths->size());
        return 1;
    }

    return 0; // OK
}
