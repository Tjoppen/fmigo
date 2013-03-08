#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "parseargs.h"

int parseArguments2(int argc,
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
                    int* quiet,
                    int* version){
    int index, c;
    opterr = 0;
    *outFileGiven = 0;

    while ((c = getopt (argc, argv, "lvqht:c:d:s:o:p:")) != -1){

        int n, skip, l, cont, i;
        connection * conn;

        switch (c) {

        case 'c':
            n=0;
            skip=0;
            l=strlen(optarg);
            cont=1;
            i=0;
            conn = &connections[0];
            while((n=sscanf(&optarg[skip],"%d,%d,%d,%d",&conn->fromFMU,&conn->fromOutputVR,&conn->toFMU,&conn->toInputVR))!=-1 && skip<l && cont){
                // Now skip everything before the n'th colon
                char* pos = strchr(&optarg[skip],':');
                if(pos==NULL){
                    cont=0;
                } else {
                    skip += pos-&optarg[skip]+1; // Dunno why this works... See http://www.cplusplus.com/reference/cstring/strchr/
                    conn = &connections[i+1];
                }
                i++;
            }
            *numConnections = i;
            break;
            
        case 'l':
            *loggingOn = 1;
            break;
            
        case 't':
            sscanf(optarg, "%lf", tEnd);
            break;
            
        case 'd':
            sscanf(optarg,"%lf", timeStepSize);
            break;

        case 'h':
            printHelp(argv[0]);
            exit(EXIT_SUCCESS);
            break;

        case 's':
            *csv_separator = optarg[0];
            break;

        case 'o':
            strcpy(outFilePath,optarg);
            *outFileGiven = 1;
            break;

        case 'q':
            *quiet = 1;
            break;

        case 'v':
            *version = 1;
            break;

        case 'p':
            // Real if number and contains .
            // Int if number and only digits
            // Bool if "true" or "false"
            // Else: string

            n=0;
            skip=0;
            l=strlen(optarg);
            cont=1;
            i=0;
            char s[MAX_PARAM_LENGTH];
            param * p = &params[0];
            while((n=sscanf(&optarg[skip],"%d,%d,%s", &p->fmuIndex, &p->valueReference, s))!=-1 && skip<l && cont){
                // Now skip everything before the n'th colon
                char* pos = strchr(&optarg[skip],':');
                if(pos==NULL){
                    cont=0;
                } else {
                    skip += pos-&optarg[skip]+1; // Dunno why this works... See http://www.cplusplus.com/reference/cstring/strchr/

                    // Check type of the parameter
                    double realVal;
                    int intVal;
                    if( sscanf(s,"%lf",&realVal) != -1 ){ // Real
                        p->realValue = realVal;
                    }
                    if( sscanf(s,"%d",&intVal) != -1 ){ // Integer
                        p->intValue = intVal;
                    }
                    // String
                    strcpy(p->stringValue,s);

                    if(strcmp(s,"true")==0){
                        p->boolValue = 1;
                    }
                    if(strcmp(s,"false")==0){
                        p->boolValue = 0;
                    }

                    p = &params[i+1];
                }
                i++;
            }
            *numParameters = i;
            break;

        case '?':

            if (optopt == 'c'){
                fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            } else if (isprint (optopt)){
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            } else {
                fprintf (stderr,
                        "Unknown option character `\\x%x'.\n",
                        optopt);
            }
            return 1;

        default:
            printf("abort %c...\n",c);
            abort ();
        }
    }

    // Parse FMU paths
    int i=0;
    for (index = optind; index < argc; index++) {
        strcpy( fmuFilePaths[i] , argv[index] );
        i++;
    }
    *numFMUs = i;

    // Check if connections refer to nonexistant FMU index
    for(i=0; i<*numConnections; i++){
        int from = connections[i].fromFMU;
        int to = connections[i].toFMU;
        if(from < 0 || from >= *numFMUs){
            fprintf(stderr,"Connection %d connects from FMU %d, which does not exist.\n", i, from);
            exit(EXIT_FAILURE);
        }
        if(to < 0 || to >= *numFMUs){
            fprintf(stderr,"Connection %d connects to FMU %d, which does not exist.\n", i, to);
            exit(EXIT_FAILURE);
        }
    }

    // Check if parameters refer to nonexistant FMU index
    for(i=0; i<*numParameters; i++){
        int idx = params[i].fmuIndex;
        if(idx < 0 || idx > *numFMUs){
            fprintf(stderr,"Parameter %d refers to FMU %d, which does not exist.\n", i, idx);
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}

/*
  Parse arguments on the form e.g.
  ./executable 2 my/fmu/path1.fmu my/fmu/path2.fmu 1 0 0 1 0 5 0.1 0 c
   exec  numFMUs  fmuPaths[]  numConnections  connections[]  numParams <params> endTime  timeStep  loggingOn  csvSep
*/ 
void parseArguments(int argc, char *argv[], int* N, fmi1_string_t** fmuFileNames, int* M, int** connections,
                    int* K, fmi1_string_t** params, double* tEnd, double* h, int* loggingOn, char* csv_separator) {

    int pos = 1;
    
    // Get number of FMUs, N
    if(argc > pos){
        if (sscanf(argv[pos],"%d", N) != 1 || *N<0) {
            printf("Error: The given number of FMUs N (%s) is not an integer larger than 0\n", argv[pos]);
            exit(EXIT_FAILURE);
        }
        pos++;
    } else {
        printf("Error: no number of FMUs (N) given\n");
        printHelp(argv[0]);
        exit(EXIT_FAILURE);
    }

    // Get the filenames
    if(argc > pos + *N){
        int i;
        *fmuFileNames = calloc(sizeof(fmi1_string_t),*N);
        for(i=0; i < *N; i++){
            (*fmuFileNames)[i] = argv[pos];
            pos++;
        }
    } else {
        printf("Error: The number of FMUs (N) does not match the given number of filenames...\n");
        printHelp(argv[0]);
        exit(EXIT_FAILURE);
    }

    // Get number of connections, M
    if(argc > pos){
        if (sscanf(argv[pos],"%d", M) != 1 || *M<0) {
            printf("Error: The given number of connections M (%s) is not an integer larger than 0\n", argv[pos]);
            exit(EXIT_FAILURE);
        }
        pos++;
    } else {
        printf("Error: no number of connections (M) given\n");
        printHelp(argv[0]);
        exit(EXIT_FAILURE);
    }

    // Get connections
    if(argc > pos + (*M) * 4){
        int i;
        int* conn = (int*)calloc(sizeof(int),(*M) * 4);
        for(i=0; i < (*M) * 4; i++){
            if (sscanf(argv[pos],"%d", &conn[i]) != 1 || conn[i]<0) {
                printf("Error parsing connections...\n");
                exit(EXIT_FAILURE);                
            }
            pos++;
        }
        *connections = conn;
    } else {
        printf("Error: The number of connections (M=%d) does not match the given number of connections...\n",*M);
        printHelp(argv[0]);
        exit(EXIT_FAILURE);
    }

    // Get number of params, K
    if(argc > pos){
        if (sscanf(argv[pos],"%d", K) != 1 || *K<0) {
            printf("Error: The given number of parameters K (%s) is not an integer larger than 0\n", argv[pos]);
            exit(EXIT_FAILURE);
        }
        pos++;
    } else {
        printf("Error: no number of parameters (K) given\n");
        printHelp(argv[0]);
        exit(EXIT_FAILURE);
    }



    // Get parameters as triplets: ( int int string ) * K
    if(argc > pos + (*K) * 3){
        int i;
        *params = calloc(sizeof(fmi1_string_t),(*K)*3);
        //params = (fmi1_string_t**)calloc(sizeof(fmi1_string_t*),(*K) * 3);
        for(i=0; i < (*K) * 3; i++){
            (*params)[i] = argv[pos];
            pos++;
        }
    } else {
        printf("Error: The number of parameters (K=%d) does not match the given number of parameters...\n",*K);
        printHelp(argv[0]);
        exit(EXIT_FAILURE);
    }

    // Get simulation end time
    if (argc > pos) {
        if (sscanf(argv[pos],"%lf", tEnd) != 1 || *tEnd<0) {
            printf("error: The given end time (%s) is not a number larger than 0\n", argv[pos]);
            exit(EXIT_FAILURE);
        }
        pos++;
    }

    // Get step size
    if (argc > pos) {
        if (sscanf(argv[pos],"%lf", h) != 1) {
            printf("error: The given stepsize (%s) is not a number\n", argv[pos]);
            exit(EXIT_FAILURE);
        }
        pos++;
    }

    // Get logging flag
    if (argc > pos) {
        if (sscanf(argv[pos],"%d", loggingOn) != 1 || *loggingOn<0 || *loggingOn>1) {
            printf("error: The given logging flag (%s) is not boolean\n", argv[pos]);
            exit(EXIT_FAILURE);
        }
        pos++;
    }
    if (argc > pos) {
        if (strlen(argv[pos]) != 1) {
            printf("error: The given CSV separator char (%s) is not valid\n", argv[pos]);
            exit(EXIT_FAILURE);
        }
        switch (argv[pos][0]) {
            case 'c': *csv_separator = ','; break; // comma
            case 's': *csv_separator = ';'; break; // semicolon
            default:  *csv_separator = argv[pos][0]; break; // any other char
        }
        pos++;
    }
    if (argc > pos) {
        printf("warning: Ignoring %d additional arguments: %s ...\n", argc-pos, argv[pos]);
        printHelp(argv[0]);
        pos++;
    }
}
