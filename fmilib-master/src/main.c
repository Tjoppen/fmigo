#include <fmilib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <regex.h>
#include <sys/stat.h>
#include <limits.h>

#include "main.h"
#include "utils.h"
#include "help.h"
#include "parseargs.h"
#include "jacobi.h"
#include "simulate.h"

int doLog = 0;

void importlogger(jm_callbacks* c, jm_string module, jm_log_level_enu_t log_level, jm_string message){
    if(doLog)
        printf("%10s,\tlog level %2d:\t%s\n", module, log_level, message);
}

void fmi1Logger(fmi1_component_t c, fmi1_string_t instanceName, fmi1_status_t status, fmi1_string_t category, fmi1_string_t message, ...){
    if(doLog){
        char msg[MAX_LOG_LENGTH];
        va_list argp;
        va_start(argp, message);
        vsprintf(msg, message, argp);
        printf("fmiStatus = %d;  %s (%s): %s\n", status, instanceName, category, msg);
    }
}
void fmi1StepFinished(fmi1_component_t c, fmi1_status_t status){
    
}

int main( int argc, char *argv[] ) {

    if(argc == 1){
        // No args given, print help
        printHelp();
        exit(EXIT_SUCCESS);
    }

    int i;

    char fmuPaths[MAX_FMUS][PATH_MAX];
    char outFilePath[PATH_MAX];
    param params[MAX_PARAMS];
    connection connections[MAX_CONNECTIONS];

    int numFMUs=0, K=0, M=0;
    double tEnd=1.0, h=0.1;
    char csv_separator = ',';
    int outFileGiven=0, quiet=0, loggingOn=0, version=0;
    enum FILEFORMAT outfileFormat;
    enum METHOD method;
    int status =parseArguments(argc,
                               argv,
                               &numFMUs,
                               fmuPaths,
                               &M,
                               connections,
                               &K,
                               params,
                               &tEnd,
                               &h,
                               &loggingOn,
                               &csv_separator,
                               outFilePath,
                               &outFileGiven,
                               &quiet,
                               &version,
                               &outfileFormat,
                               &method);

    if(version){
        // version flag given
        printf(VERSION);
        printf("\n");
        exit(EXIT_SUCCESS);
    }

    if(status == 1){
        // Exit
        exit(EXIT_FAILURE);
    }

    if(numFMUs == 0){
        // No fmus...
        printHelp();
        exit(EXIT_FAILURE);
    }

    if(!quiet){

        printf("\n");
        printHeader();
        printf("\n");

        printf("  FMUS (%d)\n",numFMUs);
        for(i=0; i<numFMUs; i++){
            printf("    %d: %s\n",i,fmuPaths[i]);
        }

        if(M>0){
            printf("\n  CONNECTIONS (%d)\n",M);
            for(i=0; i<M; i++){
                printf("    FMU %d, value reference %d ---> FMU %d, value reference %d\n",connections[i].fromFMU,
                                                        connections[i].fromOutputVR,
                                                        connections[i].toFMU,
                                                        connections[i].toInputVR);
            }
        }

        if(outFileGiven){
            printf("\n  OUTPUT CSV FILE\n");
            printf("    %s\n",outFilePath);
        }

        printf("\n");
    }

    // Callbacks
    jm_callbacks callbacks;
    callbacks.malloc = malloc;
    callbacks.calloc = calloc;
    callbacks.realloc = realloc;
    callbacks.free = free;
    callbacks.logger = importlogger;
    callbacks.log_level = loggingOn ? jm_log_level_all : jm_log_level_fatal;
    callbacks.context = 0;

    doLog = loggingOn;

    fmi1_import_t** fmus =              calloc(sizeof(fmi1_import_t*),numFMUs);
    fmi_import_context_t** contexts =   calloc(sizeof(fmi_import_context_t*),numFMUs);
    fmi_version_enu_t* versions =       calloc(sizeof(fmi_version_enu_t),numFMUs);
    const char** tmpPaths =             calloc(sizeof(const char*),numFMUs);

    // Load all FMUs
    for(i=0; i<numFMUs; i++){
        
        char buf[PATH_MAX+1]; /* not sure about the "+ 1" */
        char *res = realpath(fmuPaths[i], buf);
        if (res) {
            // OK
            strcpy(fmuPaths[i],buf);
        } else {
            // Error opening...
            perror(fmuPaths[i]);
            exit(EXIT_FAILURE);
        }

        const char* tmpPath = fmi_import_mk_temp_dir (&callbacks, "/tmp", "FMUMaster");

        tmpPaths[i] = tmpPath;
        contexts[i] = fmi_import_allocate_context(&callbacks);
        versions[i] = fmi_import_get_fmi_version(contexts[i], fmuPaths[i], tmpPath);

        if(versions[i] == fmi_version_1_enu) {

            fmus[i] = fmi1_import_parse_xml ( contexts[i], tmpPath );

            if(!fmus[i]){
                fprintf(stderr,"Could not load XML\n");
                exit(EXIT_FAILURE);
            }

            int registerGlobally = 0;
            fmi1_callback_functions_t callBackFunctions;
            callBackFunctions.logger = fmi1Logger;
            callBackFunctions.allocateMemory = calloc;
            callBackFunctions.freeMemory = free;
            callBackFunctions.stepFinished = fmi1StepFinished;
            jm_status_enu_t status = fmi1_import_create_dllfmu  ( fmus[i], callBackFunctions, registerGlobally );
            if(status == jm_status_success){
                // Successfully loaded DLL!

            } else {
                fmi_import_free_context(contexts[i]);
                fprintf(stderr,"There was an error loading the FMU dll.\n");
                exit(EXIT_FAILURE);
            }
        } else if(versions[i] == fmi_version_2_0_enu) {
            fprintf(stderr,"FMI v2.0 not supported yet.\n");
            exit(EXIT_FAILURE);

        } else {
            fprintf(stderr,"FMI version not recognized.\n");
            fmi_import_free_context(contexts[i]);
            exit(EXIT_FAILURE);
        }
    }

    if(!quiet){
        printf("  RUNNING SIMULATION...\n\n");
    }

    // Pick stepfunction
    stepfunctionType stepfunction;
    switch(method){
    case jacobi:
        stepfunction = &jacobiStep;
        break;
    default:
        fprintf(stderr, "Method enum not correct!\n");
        exit(EXIT_FAILURE);
        break;
    }

    // All loaded. Simulate.
    int res = simulate( fmus,
                        fmuPaths,
                        numFMUs,
                        connections,
                        K,
                        params,
                        M,
                        tEnd,
                        h,
                        loggingOn,
                        csv_separator,
                        callbacks,
                        quiet,
                        stepfunction,
                        outfileFormat);

    if(!quiet){
        if(res==0){
            printf("  SIMULATION TERMINATED SUCCESSFULLY\n\n");

            // print simulation summary 
            printf("  START ............ %g\n", 0.0);
            printf("  END .............. %g\n", tEnd);
            //printf("  STEPS ............ %d\n", nSteps);
            printf("  TIMESTEP ......... %g\n", h);
            printf("\n");
        } else {
            printf("  SIMULATION FAILED\n\n");
        }
    }

    // Clean up
    for(i=0; i<numFMUs; i++){
        fmi1_import_destroy_dllfmu(fmus[i]);
        fmi_import_free_context(contexts[i]);

        // Remove the temp dir
        fmi_import_rmdir(&callbacks, tmpPaths[i]);
    }
    free(fmus);
    free(contexts);
    free(versions);

    return EXIT_SUCCESS;
}
