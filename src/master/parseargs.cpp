#include <iostream>
#include <stdio.h>
#ifdef WIN32
#include "master/getopt.h"
#else
#include <getopt.h>
#endif
#include <deque>
#include <fstream>
#include "common/common.h"
#ifdef USE_MPI
#include <mpi.h>
#endif
#include <sstream>
#include <iomanip>  //for std::quoted, requires C++14
#include "common/CSV-parser.h"

#include "master/parseargs.h"
#include "master/globals.h"
#include <expat.h>

using namespace fmitcp_master;
using namespace common;
using namespace std;

static void printHelp(){
  info("Check manpage for help, \"man fmigo-master\"\n");
}

static void printInvalidArg(char option){
  error("Invalid argument of -%c\n",option);
  printHelp();
}

fmi2_base_type_enu_t fmitcp_master::type_from_char(string type) {
    if (type.size() != 1) {
        fatal("Bad type: %s\n", type.c_str());
    }

    switch (type[0]) {
    default:
    case 'r': return fmi2_base_type_real;
    case 'i': return fmi2_base_type_int;
    case 'b': return fmi2_base_type_bool;
    case 's': return fmi2_base_type_str;
    }
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
    while (ifs >> std::quoted(token)) {
        if (token == "-a") {
            fatal("Found -a token in argument file, which might lead to recursive list of arguments. Stopping.\n");
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
            fatal("Couldn't open %s to parse more arguments!\n", filename.c_str());
        }
        add_args_internal(ifs, argvstore, argv2, position);
    }
}

//split a delim separated string, with backslash escaping
//delim=':' example: "s,0,0,C\:\\foo\\bar:s,0,1,D\:\\bluh\\" -> "s,0,0,C:\foo\bar", "s,0,1,D:\bluh\"
//trailing single backslashes will be pruned ("...\" -> "...")
static deque<string> escapeSplit(string str, char delim) {
  deque<string> ret;
  ostringstream oss;
  bool escaped = false;

  for (char c : str) {
    if (escaped) {
      if (c != ',' && c != ':' && c != '\\') {
        fatal("Only comma, colon and backslash (\",:\\\") may be escaped in program options (\"%s\")\n", str.c_str());
      }
      oss << c;
      escaped = false;
    } else if (c == '\\') {
      escaped = true;
    } else if (c == delim) {
      ret.push_back(oss.str());
      oss.str("");
      oss.clear();
    } else {
      oss << c;
    }
  }

  if (escaped) {
    fatal("Trailing backslash in program option (\"%s\")\n", str.c_str());
  }

  //push remaining string
  ret.push_back(oss.str());
  return ret;
}


/**
 * ExecutionOrder stuff starts here.
 * There's two stages: first the ExecutionOrder XML is parsed.
 * The parser makes use of expat and can take ExecutionOrder XML with or without namespaces.
 * In other words both of the following strings parse to the same thing:
 *
 *  <fmigo:p xmlns:fmigo="http://umit.math.umu.se/FmiGo.xsd"><fmigo:f>0</fmigo:f><fmigo:f>1</fmigo:f></fmigo:p>
 *
 * and
 *
 *  <p><f>0</f><f>1</f></p>
 *
 * After the parsing stage the resulting ExecutionOrder tree in memory is transformed to the rendezvous structure needed by StrongMaster.
 */

struct Serial;

/**
 * Since order doesn't matter for parallel execution groups we can collect FMU indices and serials neatly in this struct.
 */
struct Parallel {
    fmitcp::int_set fmus;
    std::vector<Serial> serials;
};

/**
 * Order is significant for serial execution groups.
 * Since something like <f/><s/><f/> must be kept track of in that order we have SerialElement as kind of a union between int and Parallel.
 * We can't use an actual union however since Parallel has a non-trivial destructor.
 */
struct SerialElement {
    Parallel p;
    int f;
    bool is_p; //If true, use p. If false, use f.
};

struct Serial {
    std::vector<SerialElement> elements;
};

/**
 * Expat parser state
 */
struct ParserStruct {
    ParserStruct() {
        depth = 0;
        text = 0;
        size = 0;
        pointers[0] = &p;
    }

    int depth;

    //element text content. used for parsing <f>
    char *text;
    size_t size;

    //the structure that we're parsing
    Parallel p;

    //This is a stack of pointers into ParserStruct::p.
    //We don't need to keep more pointers than this since parsing is done depth-first.
    //Additionally, a push_back on Parallel::serials or Serial::elements would invalidate the associated pointer,
    //if were doing breadth-first parsing.
    //Finally, since we know the nesting is alternating <p> and <s>, we let even indices mean Parallel* and odd indices mean Serial*
#define MAX_DEPTH 256
    void *pointers[MAX_DEPTH];
};

static void freeMemory(ParserStruct *state) {
    free(state->text);
    state->text = NULL;
    state->size = 0;
}

//checks if tag is <expected> or <fmigo:expected> aka <http://umit.math.umu.se/FmiGo.xsd:expected>
#define NS_SEPARATOR ':'
static bool tagMatches(const char *tag, const char *expected) {
    if (!strcmp(tag, expected)) {
        return true;
    } else {
        ostringstream oss;
        oss << "http://umit.math.umu.se/FmiGo.xsd" << NS_SEPARATOR << expected;
        return oss.str() == tag;
    }
}

static void startElement(void *opaque, const XML_Char *name, const XML_Char **atts)
{
    ParserStruct *state = (ParserStruct*)opaque;

    if (tagMatches(name, "f")) {
        //<f>
        //not much to do except NULL the pointer at this level in case we want to print it
        //this makes valgrind happier
        state->pointers[state->depth] = NULL;
    } else {
        if (state->depth % 2 == 0) {
            //<p>, might be root element
            if (!tagMatches(name, "p")) {
                fatal("Expected <p> tag at depth %i, got <%s>\n", state->depth, name);
            }
            if (state->depth > 0) {
                //definitely <s><p>
                //allocate new <p>-type SerialElement in Serial one level up in the stack
                Serial *s = (Serial*)state->pointers[state->depth-1];
                SerialElement se;
                se.is_p = true;
                s->elements.push_back(se);
                state->pointers[state->depth] = &s->elements[s->elements.size()-1].p;
            } //else root element, no need to do anything since it is already allocated
        } else {
            //<p><s>
            if (!tagMatches(name, "s")) {
                fatal("Expected <s> tag at depth %i, got <%s>\n", state->depth, name);
            }
            //opposite here, allocate a new Serial in the Parallel one level up (possibly root <p>)
            Parallel *p = (Parallel*)state->pointers[state->depth-1];
            Serial s;
            p->serials.push_back(s);
            state->pointers[state->depth] = &p->serials[p->serials.size()-1];
        }
    }

    state->depth++;
    if (state->depth >= MAX_DEPTH) {
        fatal("Stepping order spec too deep. Maximum depth is %i\n", MAX_DEPTH);
    }
    freeMemory(state);
}

static void characterDataHandler(void *opaque, const XML_Char *s, int len)
{
    ParserStruct *state = (ParserStruct*)opaque;

    //may be multiple times with non-null terminated string data, so we need to keep accumulating + terminating
    if ((state->text = (char*)realloc(state->text, state->size + len + 1)) == NULL) {
        fatal("Failed to realloc() in characterDataHandler\n");
    }

    memcpy(&state->text[state->size], s, len);
    state->size += len;
    state->text[state->size] = 0;
}

static void endElement(void *opaque, const XML_Char *name)
{
    ParserStruct *state = (ParserStruct*)opaque;
    state->depth--;

    if (tagMatches(name, "f")) {
        //</f>
        if (state->size == 0) {
            fatal("<f> tag missing content\n");
        }

        int f = atoi(state->text);

        //put parsed value in the right place
        if (state->depth % 2 == 1) {
            //%s =  0  1
            //     <p><f></f>
            Parallel *p = (Parallel*)state->pointers[state->depth-1];
            p->fmus.insert(f);
        } else {
            //%s =  0  1
            //     <s><f></f>
            Serial *s = (Serial*)state->pointers[state->depth-1];
            SerialElement se;
            se.f = f;
            se.is_p = false;
            s->elements.push_back(se);
        }
    } else {
        if (state->depth % 2 == 0) {
            //</p>
        } else {
            //</s>
        }
    }

    freeMemory(state);
}

struct TraverseRet {
    fmitcp::int_set head;
    fmitcp::int_set tail;
};

TraverseRet traverse(Serial s, std::vector<Rend> *rends, int parent_rend);

TraverseRet traverse(Parallel p, std::vector<Rend> *rends, int parent_rend) {
    TraverseRet ret;
    ret.head = p.fmus;
    ret.tail = p.fmus;

    for (Serial s : p.serials) {
        TraverseRet ret2 = traverse(s, rends, parent_rend);
        for (int i : ret2.head) {
            ret.head.insert(i);
        }
        for (int i : ret2.tail) {
            ret.tail.insert(i);
        }
    }

    return ret;
}

TraverseRet traverse(Serial s, std::vector<Rend> *rends, int parent_rend) {
    TraverseRet ret;
    if (s.elements.size() <= 1) {
        fatal("serial elements in execution order must have two or more elements, got %zu\n", s.elements.size());
    }
    size_t x;
    for (x = 0; x < s.elements.size() - 1; x++) {
        TraverseRet ret2;
        if (s.elements[x].is_p) {
            ret2 = traverse(s.elements[x].p, rends, parent_rend);
        } else {
            ret2.head.insert(s.elements[x].f);
            ret2.tail.insert(s.elements[x].f);
        }

        //populate & emit rend with black set
        Rend r;
        r.parents = ret2.tail;
        for (int i : ret2.head) {
            (*rends)[parent_rend].children.insert(i);
        }
        rends->push_back(r);
        parent_rend = rends->size() - 1;
    }

    //handle last element
    if (s.elements[x].is_p) {
        TraverseRet ret2 = traverse(s.elements[x].p, rends, parent_rend);
        ret.tail = ret2.tail;
        (*rends)[parent_rend].children = ret2.tail;
    } else {
        ret.tail.insert(s.elements[x].f);
        (*rends)[parent_rend].children.insert(s.elements[x].f);
    }

    return ret;
}

std::vector<Rend> executionOrderFromXML(std::string xml) {
    //first parse the XML into tree form
    //deal with namespaces
    XML_Parser parser = XML_ParserCreateNS(NULL, NS_SEPARATOR);
    ParserStruct state;

    XML_SetUserData(parser, &state);
    XML_SetElementHandler(parser, startElement, endElement);
    XML_SetCharacterDataHandler(parser, characterDataHandler);

    if (XML_Parse(parser, xml.c_str(), xml.length(), 1) == XML_STATUS_ERROR) {
      fatal("%s at line %lu\n",
            XML_ErrorString(XML_GetErrorCode(parser)),
            XML_GetCurrentLineNumber(parser));
    }
    XML_ParserFree(parser);

    //traverse the tree
    std::vector<Rend> rends;
    rends.push_back(Rend());
    TraverseRet ret = traverse(state.p, &rends, 0);
    for (int i : ret.head) {
        rends[0].children.insert(i);
    }

    Rend r;
    r.parents = ret.tail;
    rends.push_back(r);

    return rends;
}

std::string fmitcp_master::executionOrderToString(const std::vector<Rend>& rends) {
    ostringstream oss;
    int x = 0;
    for (Rend r : rends) {
        oss << "rend " << x << endl;
        for (int i : r.parents) {
            oss << " parent " << i << endl;
        }
        for (int i : r.children) {
            oss << " child " << i << endl;
        }
        x++;
    }
    return oss.str();
}

enum METHOD {
    method_none,   // not specified
    jacobi, // backward compatibility: -g 0|1|2|...
    gs,     // backward compatibility: -g 0,1,2,...
};

void fmitcp_master::parseArguments( int argc,
                    char *argv[],
                    std::vector<std::string> *fmuFilePaths,
                    std::vector<connection> *connections,
                    std::vector<std::deque<std::string> > *params,
                    double* tEnd,
                    double* timeStepSize,
                    jm_log_level_enu_t *loglevel,
                    std::string *outFilePath,
                    enum FILEFORMAT * fileFormat,
                    int * realtimeMode,
                    std::vector<Rend> *executionOrder,
                    std::vector<int> *fmuVisibilities,
                    vector<strongconnection> *strongConnections,
                    string *hdf5Filename,
                    string *fieldnameFilename,
                    bool *holonomic,
                    double *compliance,
                    int *command_port,
                    int *results_port,
                    bool *paused,
                    bool *solveLoops,
                    bool *useHeadersInCSV,
                    fmigo_csv_fmu *csv_fmu,
                    int* maxSamples,
                    double *relaxation,
                    bool *writeSolverFields
 ) {
    int index, c;
    opterr = 0;
    METHOD method = method_none;
    vector<int> g;  //-g

#ifdef USE_MPI
    //world = master at 0, FMUs at 1..N
    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
#endif

    //repack argv into argvstore, to which the entries in argv2 point
    vector<string> argvstore;

    for (int x = 0; x < argc; x++) {
        argvstore.push_back(string(argv[x]));
    }

    vector<char*> argv2 = make_char_vector(argvstore);

    while ((c = getopt (argv2.size(), argv2.data(), "rl:ht:c:d:o:p:f:m:g:w:C:5:F:NM:a:z:ZLHV:DeS:G:RE")) != -1){
        int n, skip, l, cont, i, numScanned, stop, vis;
        deque<string> parts;
        if (optarg) parts = escapeSplit(optarg, ':');

        switch (c) {

        case 'c':
            for (auto it = parts.begin(); it != parts.end(); it++) {
                connection conn;
                deque<string> values = escapeSplit(*it, ',');
                conn.slope = 1;
                conn.intercept = 0;
                conn.needs_type = true;
                int a = 0, b = 1, c = 2, d = 3; //positions of FMUFROM,VRFROM,FMUTO,VRTO in values

                if (values.size() == 8) {
                    //TYPEFROM,FMUFROM,VRFROM,TYPETO,FMUTO,VRTO,k,m
                    if (!isVR(values[2]) || !isVR(values[5])) {
                        fatal("TYPEFROM,FMUFROM,NAMEFROM,TYPETO,FMUTO,NAMETO,k,m syntax not allowed\n");
                    }
                    conn.needs_type = false;
                    conn.fromType = type_from_char(values[0]);
                    conn.toType   = type_from_char(values[3]);
                    conn.slope    = atof(values[6].c_str());
                    conn.intercept= atof(values[7].c_str());
                    a = 1; b = 2;  c = 4; d = 5;
                } else if (values.size() == 6) {
                    //TYPEFROM,FMUFROM,VRFROM,TYPETO,FMUTO,VRTO
                    //FMUFROM,NAMEFROM,FMUTO,NAMETO,k,m
                    if (isVR(values[1])) {
                        conn.needs_type = false;
                        conn.fromType = type_from_char(values[0]);
                        conn.toType   = type_from_char(values[3]);
                        a = 1; b = 2;  c = 4; d = 5;
                    } else {
                        conn.slope    = atof(values[4].c_str());
                        conn.intercept= atof(values[5].c_str());
                    }
                } else  if (values.size() == 5) {
                    //TYPE,FMUFROM,VRFROM,FMUTO,VRTO
                    //TYPE,FMUFROM,NAMEFROM,FMUTO,NAMETO (undocumented, not recommended)
                    if (!isVR(values[1]) || !isVR(values[4])) {
                        warning("TYPE,FMUFROM,NAMEFROM,FMUTO,NAMETO syntax not recommended\n");
                    }
                    conn.needs_type = false;
                    conn.fromType = conn.toType = type_from_char(values[0]);
                    values.pop_front();
                } else if (values.size() == 4) {
                    //FMUFROM,VRFROM,FMUTO,VRTO
                    //FMUFROM,NAMEFROM,FMUTO,NAMETO
                    if (isVR(values[1]) != isVR(values[3])) {
                        fatal("Must specify VRs or names, not both (-c %s,%s,%s,%s)\n",
                            values[0].c_str(), values[1].c_str(), values[2].c_str(), values[3].c_str()
                        );
                    }

                    //assume real if VRs, request type if names
                    conn.fromType = conn.toType = type_from_char("r");
                    conn.needs_type = !isVR(values[1]);
                } else {
                    fatal("Bad param: %s\n", it->c_str());
                }

                conn.fromFMU      = atoi(values[a].c_str());
                conn.toFMU        = atoi(values[c].c_str());
                conn.fromOutputVRorNAME = values[b];
                conn.toInputVRorNAME    = values[d];

                connections->push_back(conn);
            }
            break;

        case 'C':
            //strong connections
            for (auto it = parts.begin(); it != parts.end(); it++) {
                deque<string> values = escapeSplit(*it, ',');
                strongconnection sc;

                if (values.size() < 3) {
                    fatal("Bad strong connection specification: %s\n", it->c_str());
                }

                sc.type    = values[0];
                int N = 2, ofs = 1;
                if (sc.type == "multiway") {
                    N = atoi(values[ofs++].c_str());
                    if (N < 2) {
                        fatal("Multiway constraints must have N>=2, got %i\n", N);
                    }
                }
                for (int x = 0; x < N; x++, ofs++) {
                    sc.fmus.push_back(atoi(values[ofs].c_str()));
                }

                for (auto it2 = values.begin() + ofs; it2 != values.end(); it2++) {
                    sc.vrORname.push_back(it2->c_str());
                }

                strongConnections->push_back(sc);
            }
            break;

        case 'd':
            numScanned = sscanf(optarg,"%lf", timeStepSize);
            if(numScanned <= 0){
                printInvalidArg(c);
                exit(1);
            }
            break;

        case 'S':
            numScanned = sscanf(optarg,"%i", maxSamples);
            if(numScanned <= 0){
                printInvalidArg(c);
                exit(1);
            }
            break;
        case 'f':
            if(strcmp(optarg,"csv") == 0){
                *fileFormat = csv;
            } else if( strcmp(optarg,"tikz") == 0){
                *fileFormat = tikz;
            } else if( strcmp(optarg,"mat5") == 0){
                *fileFormat = mat5;
            } else if( strcmp(optarg,"mat5_zlib") == 0){
                *fileFormat = mat5_zlib;
            } else if (!strcmp(optarg, "none")) {
                *fileFormat = none;
            } else {
                fatal("File format \"%s\" not recognized.\n",optarg);
            }
            break;

        case 'l':
            *loglevel = logOptionToJMLogLevel(optarg);
            break;

        case 'm':
            warning("-m is deprecated (method = %s)\n", optarg);
            if(strcmp(optarg,"jacobi") == 0){
                method = jacobi;
            } else if(strcmp(optarg,"gs") == 0){
                method = gs;
            } else {
                fatal("Method \"%s\" not recognized. Use \"jacobi\" or \"gs\".\n",optarg);
            }
            break;

        case 't':
            numScanned = sscanf(optarg, "%lf", tEnd);
            if(numScanned <= 0){
                printInvalidArg(c);
                exit(1);
            }
            break;

        case 'g':
            // Old step order spec
            if (executionOrder->size() != 0) {
                fatal("Do not use -g and -G together\n");
            }
            for (string s : escapeSplit(optarg, ',')) {
                g.push_back(atoi(s.c_str()));
            }
            break;

        case 'G':
            // XML based step order
            if (g.size() != 0) {
                fatal("Do not use -g and -G together\n");
            }
            *executionOrder = executionOrderFromXML(optarg);
            break;

        case 'h':
            printHelp();
            exit(1);

        case 'r':
            *realtimeMode = 1;
            break;

        case 'o':
            *outFilePath = optarg;
            break;

        case 'p':
            for (auto it = parts.begin(); it != parts.end(); it++) {
                deque<string> values = escapeSplit(*it, ',');

                //expect [type,]FMU,VR,value
                if (values.size() < 3 || values.size() > 4) {
                    fatal("Parameters must have exactly 3 or 4 parts: %s\n", it->c_str());
                }

                params->push_back(values);
            }
            break;

        case 'w':
            //visibilities
            for (auto it = parts.begin(); it != parts.end(); it++) {
                fmuVisibilities->push_back(atoi(it->c_str()));
            }
            break;

        case '5':
            *hdf5Filename = optarg;
            break;

        case 'H':
            *useHeadersInCSV = true;
            break;

        case 'F':
            warning("-F option is deprecated and will be removed soon\n");
            *fieldnameFilename = optarg;
            break;

        case 'N':
            *holonomic = false;
            break;

        case 'M':
            *compliance = atof(optarg);
            break;
        case 'R':
            *relaxation = atof(optarg);
            break;

        case 'a':
#ifdef USE_MPI
            if (world_rank == 0) {
                //only chomp stdin on the master node
                add_args(argvstore, argv2, optarg, optind);
            }
#else
            add_args(argvstore, argv2, optarg, optind);
#endif
            break;

        case 'z':
            if (parts.size() < 1 || parts.size() > 2) {
              fatal("-z must have exactly one or two parts (got %s which has %li parts)\n", optarg, parts.size());
            } else {
              *command_port = atoi(parts[0].c_str());
              if (parts.size() == 2) {
                //using ZMQ output disables printing, unless the user follows -z with -f
                *fileFormat = none;
                *results_port = atoi(parts[1].c_str());
              }
            }
            break;

        case 'Z':
            info("Starting master in paused state\n");
            *paused = true;
            break;

        case 'L':
            *solveLoops = true;
            break;

        case '?':

            if(isprint(optopt)){
                if(strchr("cdsopfm", optopt)){
                    fatal("Option -%c requires an argument.\n", optopt);
                } else {
                    fatal("Unknown option: -%c\n", optopt);
                }
            } else {
                fatal("Unknown option character: \\x%x\n", optopt);
            }

        case 'V':{
          //for (auto it = parts.begin(); it != parts.end(); it++)
        fprintf(stderr,"string %s",optarg);
            {
              fprintf(stderr," have CSV \n");
              // fprintf(stderr," %s\n",parts.at(0).c_str());
             deque<string> values = escapeSplit(optarg, ',');
              if(values.size() != 2){
                fatal("Error: Option \"-V\" requires two argument: \"-V fmuid,path/to/input.csv\"\n");
              }

              //fmigo_csv_fmu csv_matrix;

              (*csv_fmu)[atoi(values.at(0).c_str())] = fmigo_CSV_matrix(values.at(1),',');
              //printCSVmatrix(csv_matrix[values.at(0)]);
            }
            break;
        }

        case 'D':
            info("Always computing numerical directional derivatives, regardless of providesDirectionalDerivatives\n");
            alwaysComputeNumericalDirectionalDerivatives = true;
            break;

        case 'e':
#ifdef USE_MPI
            printf("USE_MPI=1\n");
#else
            printf("USE_MPI=0\n");
#endif
#ifdef USE_GPL
            printf("USE_GPL=1\n");
#else
            printf("USE_GPL=0\n");
#endif
            exit(0);

        case 'E':
            *writeSolverFields = true;
            break;

        default:
            fatal("abort %c...\n",c);
        }
    }

    // Parse FMU paths in the end of the command line
    for (index = optind; index < (int)argv2.size(); index++) {
        fmuFilePaths->push_back(argv2[index]);
    }

#ifndef USE_MPI
    //ZMQ
    size_t numFMUs = fmuFilePaths->size();

    if (numFMUs == 0){
        error("No FMU URIs given. Aborting...\n");
        printHelp();
        exit(1);
    }

#else //USE_MPI
    // Support two types of command lines:
    //
    //  mpiexec -np 3 fmigo-mpi [master arguments] fmu1.fmu fmu2.fmu
    //
    // Where the argument to -np is the number of FMUs + 1.
    // Second form:
    //
    //  mpiexec -np 1 fmigo-mpi [master arguments] : -np 1 fmigo-mpi fmu1.fmu : -np 1 fmigo-mpi fmu2.fmu
    //
    if (world_size <= 1) {
        //world too small to make sense
        fatal("No FMUs given and MPI world too small. Aborting...\n");
    }

    if (fmuFilePaths->size() == 0) {
        //zero FMU filename is only OK for the master node
        if (world_rank != 0) {
            fatal("MPI non-master node given no FMU filename(s)\n");
        }
    } else if (fmuFilePaths->size() == 1) {
        //a single filename is OK most of the time, unless we're the master node and world_size > 2
        if (world_rank == 0 && world_size > 2) {
            fatal("MPI master node given too few FMU filenames for given MPI world size (%i)\n", world_size);
        }
    } else {
      //multiple filenames only OK if all nodes given full list of FMU filenames
      size_t numFMUs = fmuFilePaths->size();
      if ((size_t)world_size != numFMUs + 1) {
        //only complain for the first node
        if (world_rank == 0) {
            error("Need exactly n+1 processes, where n is the number of FMUs (%zu)\n", numFMUs);
            info("Try re-running with mpiexec -np %zu fmigo-mpi [rest of command line]\n", numFMUs+1 );
        }
        exit(1);
      }
    }

    size_t numFMUs = world_size - 1;
#endif

    // Check if connections refer to nonexistant FMU index
    int i = 0;
    for (auto it = connections->begin(); it != connections->end(); it++, i++) {
        if(it->fromFMU < 0 || (size_t)it->fromFMU >= numFMUs){
            fatal("Connection %d connects from FMU %d, which does not exist.\n", i, it->fromFMU);
        }
        if(it->toFMU < 0 || (size_t)it->toFMU >= numFMUs){
            fatal("Connection %d connects to FMU %d, which does not exist.\n", i, it->toFMU);
        }
    }

    i = 0;
    for (auto it = strongConnections->begin(); it != strongConnections->end(); it++, i++) {
        for (int fmu : it->fmus) {
            if(fmu < 0 || (size_t)fmu >= numFMUs){
                fatal("Strong connection %d connects from FMU %d, which does not exist.\n", i, fmu);
            }
        }
    }

    if (method == jacobi || (method == method_none && executionOrder->size() == 0)) {
        if (g.size() != 0) {
            fatal("You may not use -g and -m jacobi together\n");
        }
        debug("generate jacobi\n");
        *executionOrder = vector<Rend>(2, Rend());
        for (size_t x = 0; x < numFMUs; x++) {
            (*executionOrder)[0].children.insert(x);
            (*executionOrder)[1].parents.insert(x);
        }
    } else if (method == gs) {
        if (g.size() == 0) {
            //default stepOrders
            for (size_t x = 0; x < numFMUs; x++) {
                g.push_back(x);
            }
        }
        if (g.size() != numFMUs) {
            fatal("The number of entries in -g (%i) does not match the number of FMUs (%i)\n", (int)g.size(), (int)numFMUs);
        }

        //construct serial execution order
        //g[0] -> g[1] -> ... -> g[N-1]
        debug("generate gs\n");
        *executionOrder = vector<Rend>(numFMUs+1, Rend());
        for (size_t x = 0; x < numFMUs; x++) {
            (*executionOrder)[x].children.insert(g[x]);
            (*executionOrder)[x+1].parents.insert(g[x]);
        }

    }
}
