#include <fmilib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>

#include "main.h"
#include "utils.h"
#include "help.h"
#include "parseargs.h"
#include "jacobi.h"
#include "gs.h"
#include "simulate.h"
#include "log.h"

int main( int argc, char *argv[] ) {

    if(argc == 1){
        // No args given, print help
        printHelp();
        exit(EXIT_SUCCESS);
    }

    int exitCode = EXIT_SUCCESS,
        i,
        doSimulate = 1;

    char fmuPaths[MAX_FMUS][PATH_MAX];
    int fmuVisibilities[MAX_FMUS] = {0};
    char outFilePath[PATH_MAX] = "result.csv";
    param params[MAX_PARAMS];
    connection connections[MAX_CONNECTIONS];

    int stepOrder[MAX_STEP_ORDER],
        numFMUs=0,
        numParameters=0,
        numConnections=0,
        outFileGiven=0,
        quiet=0,
        loggingOn=0,
        version=0,
        realtime=0,
        printXML=0,
        numStepOrder=0;
    double tEnd=1.0,
           timeStep=0.1;
    char csv_separator = ',';
    enum FILEFORMAT outfileFormat;
    enum METHOD method;
    int status =parseArguments(argc,
                               argv,
                               &numFMUs,
                               fmuPaths,
                               &numConnections,
                               connections,
                               &numParameters,
                               params,
                               &tEnd,
                               &timeStep,
                               &loggingOn,
                               &csv_separator,
                               outFilePath,
                               &outFileGiven,
                               &quiet,
                               &version,
                               &outfileFormat,
                               &method,
                               &realtime,
                               &printXML,
                               stepOrder,
                               &numStepOrder,
                               fmuVisibilities);

    if(printXML){
        // Should print XML and quit
        quiet = 1;
        doSimulate = 0;
    }

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

    fmi1_import_t** fmus1 =             calloc(sizeof(fmi1_import_t*),numFMUs);
    fmi2_import_t** fmus2 =             calloc(sizeof(fmi2_import_t*),numFMUs);
    fmi_import_context_t** contexts =   calloc(sizeof(fmi_import_context_t*),numFMUs);
    fmi_version_enu_t* versions =       calloc(sizeof(fmi_version_enu_t),numFMUs);
    const char** tmpPaths =             calloc(sizeof(const char*),numFMUs);

    int numFMU1 = 0, numFMU2 = 0; // Count the number of each version

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

        tmpPaths[i] = fmi_import_mk_temp_dir (&callbacks, "/tmp", "FMUMaster");
        contexts[i] = fmi_import_allocate_context(&callbacks);
        versions[i] = fmi_import_get_fmi_version(contexts[i], fmuPaths[i], tmpPaths[i]);

        if(printXML){
            char xmlPath[PATH_MAX];
            char ch;
            strcpy(xmlPath,tmpPaths[i]);
            strcat(xmlPath,"/modelDescription.xml");
            FILE * xmlFile = fopen(xmlPath,"r");
            if(xmlFile){
                while( ( ch = fgetc(xmlFile) ) != EOF )
                    printf("%c",ch);
                fclose(xmlFile);
            } else {
                printf("Could not open XML file %s...\n",xmlPath);
            }
        }

        if(versions[i] == fmi_version_1_enu) { // FMI 1.0

            numFMU1++;

            fmus1[i] = fmi1_import_parse_xml ( contexts[i], tmpPaths[i] );

            if(!fmus1[i]){
                fprintf(stderr,"Could not load XML\n");
                exit(EXIT_FAILURE);
            }

            int registerGlobally = 0;
            fmi1_callback_functions_t callBackFunctions;
            callBackFunctions.logger = loggingOn ? fmi1_log_forwarding : fmi1_null_logger;
            callBackFunctions.allocateMemory = calloc;
            callBackFunctions.freeMemory = free;
            jm_status_enu_t status = fmi1_import_create_dllfmu  ( fmus1[i], callBackFunctions, registerGlobally );
            if(status == jm_status_success){
                // Successfully loaded DLL!

            } else {
                fmi_import_free_context(contexts[i]);
                fprintf(stderr,"There was an error loading the FMU dll.\n");
                exit(EXIT_FAILURE);
            }
        } else if(versions[i] == fmi_version_2_0_enu) { // FMI 2.0
    
            numFMU2++;

            // Test some fmu2 stuff
            fmus2[i] = fmi2_import_parse_xml(contexts[i], tmpPaths[i], 0);
            if(!fmus2[i]){
                fprintf(stderr,"Could not load XML\n");
                exit(EXIT_FAILURE);
            }

            if(fmi2_import_get_fmu_kind(fmus2[i]) != fmi2_fmu_kind_cs) {
                fprintf(stderr,"Only CS 2.0 is supported by this code\n");
                exit(EXIT_FAILURE);
            }

            fmi2_callback_functions_t callBackFunctions;

            callBackFunctions.logger = loggingOn ? fmi2_log_forwarding : fmi2_null_logger;
            callBackFunctions.allocateMemory = calloc;
            callBackFunctions.freeMemory = free;
            callBackFunctions.componentEnvironment = fmus2[i];

            jm_status_enu_t status = fmi2_import_create_dllfmu(fmus2[i], fmi2_fmu_kind_cs, &callBackFunctions);
            if(status == jm_status_error) {
                printf("Could not create the DLL loading mechanism(C-API) (error: %s).\n", fmi2_import_get_last_error(fmus2[i]));
                exit(EXIT_FAILURE);
            }

            fprintf(stderr,"FMI v2.0 not supported yet.\n");
            doSimulate = 0;
            exitCode = EXIT_FAILURE;

        } else {
            fprintf(stderr,"FMI version not recognized.\n");
            fmi_import_free_context(contexts[i]);
            exit(EXIT_FAILURE);
        }
    }

    if(numFMU1 && numFMU2){
        fprintf(stderr, "Sorry, you can only simulate FMUs of the same version together.\n");
        doSimulate = 0;
        exitCode = EXIT_FAILURE;
    }

    if(doSimulate){

        if(!quiet){

            printf("  FMUS (%d)\n",numFMUs);
            for(i=0; i<numFMUs; i++){
                printf("    %d: %s",i,fmuPaths[i]);
                switch(versions[i]){
                    case fmi_version_1_enu:   printf(" (v1.0)\n"); break;
                    case fmi_version_2_0_enu: printf(" (v2.0)\n"); break;
                }
            }

            if(numConnections > 0){
                printf("\n  CONNECTIONS (%d)\n",numConnections);
                for(i=0; i<numConnections; i++){
                    printf("    FMU %d, value reference %d ---> FMU %d, value reference %d\n",  connections[i].fromFMU,
                                                                                                connections[i].fromOutputVR,
                                                                                                connections[i].toFMU,
                                                                                                connections[i].toInputVR);
                }
            }

            printf("\n  OUTPUT CSV FILE\n");
            printf("    %s\n",outFilePath);

            printf("\n");

            printf("  RUNNING SIMULATION...\n\n");
        }

        int res, numSteps;

        if(numFMU1){

            // Pick stepfunction
            fmi1stepfunction stepfunction;
            switch(method){
            case jacobi:
                stepfunction = &fmi1JacobiStep;
                break;
            case gs:
                stepfunction = &fmi1GaussSeidelStep;
                break;
            default:
                fprintf(stderr, "Method enum not correct!\n");
                exit(EXIT_FAILURE);
                break;
            }

            // All loaded. Simulate.
            res = fmi1simulate( fmus1,
                                tmpPaths, //pass unzipped paths
                                numFMUs,
                                connections,
                                numParameters,
                                params,
                                numConnections,
                                tEnd,
                                timeStep,
                                loggingOn,
                                csv_separator,
                                callbacks,
                                quiet,
                                stepfunction,
                                outfileFormat,
                                outFilePath,
                                realtime,
                                &numSteps,
                                numStepOrder,
                                stepOrder,
                                fmuVisibilities);
        } else if(numFMU2){
            
            // Pick stepfunction
            fmi2stepfunction stepfunction;
            switch(method){
            case jacobi:
                stepfunction = &fmi2JacobiStep;
                break;
            default:
                fprintf(stderr, "Method enum not correct!\n");
                exit(EXIT_FAILURE);
                break;
            }

            // All loaded. Simulate.
            res = fmi2simulate( fmus2,
                                tmpPaths, //pass unzipped paths
                                numFMUs,
                                connections,
                                numParameters,
                                params,
                                numConnections,
                                tEnd,
                                timeStep,
                                loggingOn,
                                csv_separator,
                                callbacks,
                                quiet,
                                stepfunction,
                                outfileFormat,
                                outFilePath,
                                realtime,
                                &numSteps,
                                fmuVisibilities);

        } else {
            fprintf(stderr, "Something went wrong...\n");
        }

        if(!quiet){
            if(res==0){
                printf("  SIMULATION TERMINATED SUCCESSFULLY\n\n");

                // print simulation summary 
                printf("  START ............ %g\n", 0.0);
                printf("  END .............. %g\n", tEnd);
                printf("  STEPS ............ %d\n", numSteps);
                printf("  TIMESTEP ......... %g\n", timeStep);
                printf("\n");
            } else {
                printf("  SIMULATION FAILED\n\n");
            }
        }
    }

    // Clean up
    for(i=0; i<numFMUs; i++){

        // Free the FMU
        switch(versions[i]){
            case fmi_version_1_enu:
                fmi1_import_destroy_dllfmu(fmus1[i]);
                fmi1_import_free(fmus1[i]);
                break;
            case fmi_version_2_0_enu:
                fmi2_import_destroy_dllfmu(fmus2[i]);
                fmi2_import_free(fmus2[i]);
                break;
        }

        // Free context
        fmi_import_free_context(contexts[i]);

        // Remove the temp dir
        fmi_import_rmdir(&callbacks, tmpPaths[i]);
        callbacks.free(tmpPaths[i]);
    }

    // Free arrays
    free(fmus1);
    free(fmus2);
    free(contexts);
    free(versions);
    free(tmpPaths);

    return exitCode;
}
