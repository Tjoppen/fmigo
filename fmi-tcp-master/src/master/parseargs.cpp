#include <iostream>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#ifdef WIN32
#include "master/getopt.h"
#else
#include <getopt.h>
#endif
#include <json/json.h>
#include <deque>
#include <fstream>
#include "common/common.h"
#ifdef USE_MPI
#include <mpi.h>
#endif

#include "master/parseargs.h"

using namespace fmitcp_master;
using namespace common;
using namespace std;

static void printHelp(){
    system("man fmi-tcp-master");
}

static void printInvalidArg(char option){
    fprintf(stderr, "Invalid argument of -%c. Use -h for help.\n",option);
}

static fmi2_base_type_enu_t type_from_char(string type) {
    if (type.size() != 1) {
        fprintf(stderr, "Bad type: %s\n", type.c_str());
        exit(1);
    }

    switch (type[0]) {
    default:
    case 'r': return fmi2_base_type_real;
    case 'i': return fmi2_base_type_int;
    case 'b': return fmi2_base_type_bool;
    case 's': return fmi2_base_type_str;
    }
}

template<typename T> int checkFMUIndex(T it, int i, size_t numFMUs) {
    if(it->fromFMU < 0 || it->fromFMU >= numFMUs){
        fprintf(stderr,"Connection %d connects from FMU %d, which does not exist.\n", i, it->fromFMU);
        return 1;
    }
    if(it->toFMU < 0 || it->toFMU >= numFMUs){
        fprintf(stderr,"Connection %d connects to FMU %d, which does not exist.\n", i, it->toFMU);
        return 1;
    }

    return 0;
}

static vector<char*> make_char_vector(vector<string>& vec) {
    vector<char*> ret;
    for (size_t x = 0; x < vec.size(); x++) {
        ret.push_back((char*)vec[x].c_str());
    }
    return ret;
}

//template to handle not being able to use ifstream and istream at the same time
//maybe there's a better way, but this works
template<typename T> void add_args_internal(T& ifs, vector<string>& argvstore, vector<char*>& argv2, int position) {
    string token;

    //read tokens, insert into argvstore/argv2
    while (ifs >> token) {
        if (token == "-a") {
            fprintf(stderr, "Found -a token in argument file, which might lead to recursive list of arguments. Stopping.\n");
            exit(1);
        }

        argvstore.insert(argvstore.begin() + position, token);
        position++;
    }

    argv2 = make_char_vector(argvstore);
}

static void add_args(vector<string>& argvstore, vector<char*>& argv2, string filename, int position) {
    if (filename == "-") {
        add_args_internal(std::cin, argvstore, argv2, position);
    } else {
        ifstream ifs(filename.c_str());
        if (!ifs) {
            fprintf(stderr, "Couldn't open %s to parse more arguments!\n", filename.c_str());
            exit(1);
        }
        add_args_internal(ifs, argvstore, argv2, position);
    }
}

int fmitcp_master::parseArguments( int argc,
                    char *argv[],
                    std::vector<std::string> *fmuFilePaths,
                    std::vector<connection> *connections,
                    map<pair<int,fmi2_base_type_enu_t>, vector<param> > *params,
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
                    vector<strongconnection> *strongConnections,
                    vector<connectionconfig> *connconf,
                    string *hdf5Filename,
                    string *fieldnameFilename,
                    bool *holonomic,
                    double *compliance,
                    int *command_port,
                    int *results_port,
                    bool *paused,
                    bool *solveLoops,
                    enum INTEGRATORTYPE *integratorType,
                    double *tolerance
        ) {
    int index, c;
    opterr = 0;

    //repack argv into argvstore, to which the entries in argv2 point
    vector<string> argvstore;

    for (int x = 0; x < argc; x++) {
        argvstore.push_back(string(argv[x]));
    }

    vector<char*> argv2 = make_char_vector(argvstore);

    while ((c = getopt (argv2.size(), argv2.data(), "xrl:vqh:t:T:c:d:s:o:p:f:m:g:w:C:j:5:F:NM:a:z:ZLi:")) != -1){
        int n, skip, l, cont, i, numScanned, stop, vis;
        deque<string> parts;
        if (optarg) parts = split(optarg, ':');

        switch (c) {

        case 'c':
            for (auto it = parts.begin(); it != parts.end(); it++) {
                connection conn;
                deque<string> values = split(*it, ',');
                conn.slope = 1;
                conn.intercept = 0;
                int a = 0, b = 1, c = 2, d = 3; //positions of FMUFROM,VRFROM,FMUTO,VRTO in values

                if (values.size() == 8) {
                    //TYPEFROM,FMUFROM,VRFROM,TYPETO,FMUTO,VRTO,k,m
                    conn.fromType = type_from_char(values[0]);
                    conn.toType   = type_from_char(values[3]);
                    conn.slope    = atof(values[6].c_str());
                    conn.intercept= atof(values[7].c_str());
                    a = 1; b = 2;  c = 4; d = 5;
                } else if (values.size() == 6) {
                    //TYPEFROM,FMUFROM,VRFROM,TYPETO,FMUTO,VRTO
                    conn.fromType = type_from_char(values[0]);
                    conn.toType   = type_from_char(values[3]);
                    a = 1; b = 2;  c = 4; d = 5;
                } else  if (values.size() == 5) {
                    //TYPE,FMUFROM,VRFROM,FMUTO,VRTO
                    conn.fromType = conn.toType = type_from_char(values[0]);
                    values.pop_front();
                } else if (values.size() == 4) {
                    //FMUFROM,VRFROM,FMUTO,VRTO
                    conn.fromType = conn.toType = type_from_char("r");
                } else {
                    fprintf(stderr, "Bad param: %s\n", it->c_str());
                    return 1;
                }

                conn.fromFMU      = atoi(values[a].c_str());
                conn.fromOutputVR = atoi(values[b].c_str());
                conn.toFMU        = atoi(values[c].c_str());
                conn.toInputVR    = atoi(values[d].c_str());

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
            *loglevel = logOptionToJMLogLevel(optarg);
            break;

        case 'm':
            if(strcmp(optarg,"jacobi") == 0){
                *method = jacobi;
            } else if(strcmp(optarg,"gs") == 0){
                *method = gs;
            } else if(strcmp(optarg,"me") == 0){
                *method = me;
            } else {
                fprintf(stderr,"Method \"%s\" not recognized. Use \"jacobi\" or \"gs\" or \"me\".\n",optarg);
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
                    p.type       = type_from_char(values[0]);
                    values.pop_front();
                } else if (values.size() == 3) {
                    p.type       = type_from_char("r");
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

        case 'j': {
            Json::Value root;
            std::ifstream ifs(optarg);
            if (!ifs) {
                fprintf(stderr, "%s does not exist\n", optarg);
                exit(1);
            }
            ifs >> root;

            for (auto conn : root["connections"]) {
                //fprintf(stderr, "node %s, signal %s\n", conn["input"]["node"].asString().c_str(), conn["input"]["signal"].asString().c_str());
                connectionconfig conf;
                conf.input.node   = conn["input"]["node"].asString();
                conf.input.signal = conn["input"]["signal"].asString();

                for (auto output : conn["outputs"]) {
                    //fprintf(stderr, "-> %s, %s\n", output["node"].asString().c_str(), output["signal"].asString().c_str());
                    nodesignal ns;
                    ns.node   = output["node"].asString();
                    ns.signal = output["signal"].asString();
                    conf.outputs.push_back(ns);
                }

                if (conn.isMember("constant")) {
                    auto c = conn["constant"];

                    if (c.isBool()) {
                        //fprintf(stderr, "bool: %s\n", c.asBool() ? "true" : "false");
                        conf.defaultValue.type = fmi2_base_type_bool;
                        conf.defaultValue.boolValue = c.asBool();
                    } else if (c.isDouble()) {
                        //fprintf(stderr, "double: %lf\n", c.asDouble());
                        conf.defaultValue.type = fmi2_base_type_real;
                        conf.defaultValue.realValue = c.asDouble();
                    } else if (c.isInt()) {
                        //fprintf(stderr, "int: %i\n", c.asInt());
                        conf.defaultValue.type = fmi2_base_type_int;
                        conf.defaultValue.intValue = c.asInt();
                    } else if (c.isString()) {
                        //fprintf(stderr, "string: %s\n", c.asString().c_str());
                        conf.defaultValue.type = fmi2_base_type_str;
                        conf.defaultValue.stringValue = c.asString();
                    } else {
                        fprintf(stderr, "unknown constant value type for node %s, signal %s\n",
                                conn["input"]["node"].asString().c_str(), conn["input"]["signal"].asString().c_str());
                        exit(1);
                    }

                    conf.hasDefault = true;
                } else {
                    conf.hasDefault = false;
                }
                connconf->push_back(conf);
            }
            fprintf(stderr, "Parsed %li connection configuration(s) from %s\n",
                    connconf->size(), optarg);
            break;
        }

        case '5':
            *hdf5Filename = optarg;
            break;

        case 'F':
            *fieldnameFilename = optarg;
            break;

        case 'N':
            *holonomic = false;
            break;

        case 'M':
            *compliance = atof(optarg);
            break;

        case 'a':
            add_args(argvstore, argv2, optarg, optind);
            break;

        case 'z':
            if (parts.size() != 2) {
                fprintf(stderr, "-z must have exactly two parts (got %s which has %li parts)\n", optarg, parts.size());
                return 1;
            }
            *command_port = atoi(parts[0].c_str());
            *results_port = atoi(parts[1].c_str());
            break;

        case 'Z':
            fprintf(stderr, "Starting master in paused state\n");
            *paused = true;
            break;
        case 'i':
            if(strcmp(optarg,"cgsl") == 0){
                *integratorType = cgsl;
            } else if(strcmp(optarg,"bsd") == 0){
                *integratorType = bsd;
            }break;
        case 'T':
            *tolerance = atof(optarg);
            break;
        case 'L':
            *solveLoops = true;
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

#ifdef USE_MPI
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    size_t numFMUs = world_size - 1;
#else
    // Parse FMU paths in the end of the command line
    for (index = optind; index < argv2.size(); index++) {
        fmuFilePaths->push_back(argv2[index]);
    }

    size_t numFMUs = fmuFilePaths->size();

    if (numFMUs == 0){
        fprintf(stderr, "No FMUs given. Aborting... (see -h for help)\n");
        return 1;
    }
#endif

    // Check if connections refer to nonexistant FMU index
    int i = 0;
    for (auto it = connections->begin(); it != connections->end(); it++, i++) {
        if (checkFMUIndex(it, i, numFMUs))
            return 1;
    }

    i = 0;
    for (auto it = strongConnections->begin(); it != strongConnections->end(); it++, i++) {
        if (checkFMUIndex(it, i, numFMUs))
            return 1;
    }

    // Check if parameters refer to nonexistant FMU index
    i = 0;
    for (auto it = params->begin(); it != params->end(); it++, i++) {
        if(it->first.first < 0 || it->first.first >= numFMUs){
            fprintf(stderr,"Parameter %d refers to FMU %d, which does not exist.\n", i, it->first.first);
            return 1;
        }
    }

    // Default step order is all FMUs in their current order
    if (stepOrder->size() == 0){
        for(c=0; c<numFMUs; c++){
            stepOrder->push_back(c);
        }
    } else if (stepOrder->size() != numFMUs) {
        fprintf(stderr, "Step order/FMU count mismatch: %zu vs %zu\n", stepOrder->size(), numFMUs);
        return 1;
    }

    return 0; // OK
}
