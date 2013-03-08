#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "parseargs.h"

int parseArguments(int argc,
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

    strcpy(outFilePath,DEFAULT_OUTFILE);

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

                // Now skip everything before the n'th colon
                char* pos = strchr(&optarg[skip],':');
                if(pos==NULL){
                    cont=0;
                } else {
                    skip += pos-&optarg[skip]+1; // Dunno why this works... See http://www.cplusplus.com/reference/cstring/strchr/
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

    // Parse FMU paths in the end of the command line
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
