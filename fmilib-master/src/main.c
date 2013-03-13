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

void setInitialValues(fmi1_import_t* fmu){
    int k;

    // Set initial values
    fmi1_import_variable_list_t* vl = fmi1_import_get_variable_list(fmu);
    int num = fmi1_import_get_variable_list_size(vl);
    for (k=0; num; k++) {
        fmi1_import_variable_t * v = fmi1_import_get_variable(vl,k);
        if(!v) break;

        fmi1_value_reference_t vr[1];
        vr[0] = fmi1_import_get_variable_vr(v);

        fmi1_import_variable_typedef_t * vt = fmi1_import_get_variable_declared_type(v);

        fmi1_base_type_enu_t bt;
        bt = fmi1_import_get_variable_base_type(v);

        fmi1_real_t lol[1];
        fmi1_integer_t innt[1];
        fmi1_boolean_t boool[1];
        fmi1_string_t striing[1];

        // Set initial values from the XML file
        if(fmi1_import_get_variable_has_start(v)){
            switch (bt){
            case fmi1_base_type_real:
                lol[0] = fmi1_import_get_real_variable_start((fmi1_import_real_variable_t*) v);
                fmi1_import_set_real(fmu,   vr,   1, lol);
                break;
            case fmi1_base_type_int:
            case fmi1_base_type_enum:
                innt[0] = fmi1_import_get_integer_variable_start((fmi1_import_integer_variable_t*) v);
                fmi1_import_set_integer(fmu,   vr,   1, innt);
                break;
            case fmi1_base_type_bool:
                boool[0] = fmi1_import_get_boolean_variable_start((fmi1_import_bool_variable_t*) v);
                fmi1_import_set_boolean(fmu,   vr,   1, boool);
                break;
            case fmi1_base_type_str:
                striing[0] = fmi1_import_get_string_variable_start((fmi1_import_string_variable_t*) v);
                fmi1_import_set_string(fmu,   vr,   1, striing);
                break;
            default: 
                fprintf(stderr,"Could not determine type of value reference %d in FMU. Continuing without setting initial value...\n", vr[0]);
                break;
            }
        }
    }
}

// Set initial values from the command line, overrides the XML init values
void setParams(int numFMUs, int numParams, fmi1_import_t ** fmus, param params[MAX_PARAMS]){
    int i,j,k;
    for(i=0; i<numFMUs; i++){

        fmi1_import_variable_list_t* vl = fmi1_import_get_variable_list(fmus[i]);
        int num = fmi1_import_get_variable_list_size(vl);
        for (k=0; num; k++) {
            fmi1_import_variable_t* v = fmi1_import_get_variable(vl,k);
            if(!v) break;

            fmi1_value_reference_t vr[1];
            vr[0] = fmi1_import_get_variable_vr(v);
            fmi1_base_type_enu_t type = fmi1_import_get_base_type((fmi1_import_variable_typedef_t*)v);
            fmi1_real_t lol[1];
            fmi1_integer_t innt[1];
            fmi1_boolean_t boool[1];
            fmi1_string_t striing[1];

            int j;
            for(j=0; j < numParams; j++){ // Loop over params

                int fmuIndex = params[j].fmuIndex;
                int valueReference = params[j].valueReference;

                if( i == fmuIndex && // Correct FMU
                    vr[0] == valueReference // Correct valuereference
                    ) {

                    float tmpFloat;
                    int tmpInt;

                    switch (type){
                    
                    case fmi1_base_type_real: // Real
                        lol[0] = params[j].realValue;
                        fmi1_import_set_real(fmus[i],   vr,   1, lol);
                        break;

                    case fmi1_base_type_int: // Integer
                    case fmi1_base_type_enum:
                        innt[0] = params[j].intValue;
                        fmi1_import_set_integer(fmus[i],   vr,   1, innt);
                        break;

                    // Boolean
                    case fmi1_base_type_bool:
                        boool[0] = params[j].boolValue;
                        fmi1_import_set_boolean(fmus[i],   vr,   1, boool);
                        break;

                    // String
                    case fmi1_base_type_str:
                        striing[0] = params[j].stringValue;
                        fmi1_import_set_string(fmus[i],   vr,   1, striing);
                        break;

                    default: 
                        printf("Could not determine type of value reference %d in FMU %d. Continuing without connection value transfer...\n", vr[0],i);
                        break;
                    }
                }
            }
        }
    }
}

static int simulate( fmi1_import_t** fmus,
                     char fmuFileNames[MAX_FMUS][PATH_MAX],
                     int N,
                     connection connections[MAX_CONNECTIONS],
                     int K,
                     param params[MAX_PARAMS],
                     int M,
                     double tEnd,
                     double h,
                     int loggingOn,
                     char separator,
                     jm_callbacks callbacks,
                     int quiet,
                     stepfunctionType stepfunc,
                     enum FILEFORMAT outFileFormat){
    int i;
    int k;
    double time;                                                // Current time
    double tStart = 0;                                          // Start time
    fmi1_string_t** guids;
    jm_status_enu_t status;                                     // return code of the fmu functions
    fmi1_string_t fmuLocation = "";                             // path to the fmu as URL, "file://C:\QTronic\sales"
    fmi1_string_t mimeType = "application/x-fmu-sharedlibrary"; // denotes tool in case of tool coupling
    fmi1_real_t timeout = 1000;                                 // wait period in milliseconds, 0 for unlimited wait period
    fmi1_boolean_t visible = 0;                                 // no simulator user interface
    fmi1_boolean_t interactive = 0;                             // simulation run without user interaction
    int nSteps = 0;                                             // Number of steps taken
    FILE** files;                                               // result files
    char** fmuNames;                                            // Result file names

    // Allocate
    guids =        (fmi1_string_t**)calloc(sizeof(fmi1_string_t*),N);
    for(i=0; i<N; i++){
        guids[i] = calloc(sizeof(char),MAX_GUID_LENGTH);
    }
    files =       (FILE**)calloc(sizeof(FILE*),N);
    fmuNames =    (char**)calloc(sizeof(char*),N);

    // Open result file
    FILE * f;
    if (!(f = fopen("result.csv", "w"))) {
        fprintf(stderr,"Could not write to %s\n", "result.csv");
        return 1;
    }

    // Init FMU names
    // Todo: should be able to specify via command line
    for(i=0; i<N; i++){ 
        fmuNames[i] = calloc(sizeof(char),100);
        sprintf(fmuNames[i],"fmu%d",i);
    }

    // Write CSV header
    if(outFileFormat == csv){
        writeCsvHeader(f, fmuNames, fmus, N, separator);
    }

    // Init all the FMUs
    for(i=0; i<N; i++){ 

        //strcpy((char*)guids[i], fmi1_import_get_GUID ( fmus[i] ) );
        //char * a = fmi_import_create_URL_from_abs_path(&callbacks, fmuFileNames[i]);
        fmuLocation = fmi_import_create_URL_from_abs_path(&callbacks, (const char*)fmuFileNames[i]);

        // Instantiate the slave
        status = fmi1_import_instantiate_slave (fmus[i], fmuNames[i], fmuLocation, mimeType, timeout, visible, interactive);
        if (status == jm_status_error){
            fprintf(stderr,"Could not instantiate model %s\n",fmuFileNames[i]);
            return 1;
        }
        
        // StopTimeDefined=fmiFalse means: ignore value of tEnd
        fmi1_status_t status = fmi1_import_initialize_slave(fmus[i], tStart, (fmi1_boolean_t)0, tEnd);
        if (status != fmi1_status_ok){
            fprintf(stderr,"Could not initialize model %s\n",fmuFileNames[i]);
            return 1;
        }
    }

    // Set initial values from the XML file
    for(i=0; i<N; i++){
        setInitialValues(fmus[i]);
    }

    // Set user-given parameters
    setParams(N, K, fmus, params);

    // Write CSV row for time=0
    if(outFileFormat == csv){
        writeCsvRow(f, fmus, N, time, separator);
    }

    // enter the simulation loop
    time = tStart;
    int l;
    fmi1_base_type_enu_t type;
    fmi1_value_reference_t vrFrom[1];
    fmi1_value_reference_t vrTo[1];
    fmi1_real_t rr[2];
    fmi1_boolean_t bb[2];
    fmi1_integer_t ii[2];
    fmi1_string_t ss[2];
    int found = 0;
    status = fmi1_status_ok;

    int simulationStatus = 0; // Success

    while (time < tEnd && status==fmi1_status_ok) {

        // Step the system of FMUs
        int result = (*stepfunc)(time, h, N, fmus, M, connections);

        if(result != 0){
            simulationStatus = 1; // Error
            break;
        }

        // Advance time
        time += h;

        if(outFileFormat == csv){
            writeCsvRow(f, fmus, N, time, separator);
        }

        nSteps++;
    }
    
    // end simulation
    for(i=0; i<N; i++){
        fmi1_status_t s = fmi1_import_terminate_slave(fmus[i]);
        fmi1_import_free_slave_instance(fmus[i]);
        if(s != fmi1_status_ok){
            fprintf(stderr,"Error terminating slave instance %d. Continuing...\n",i);
            simulationStatus = 1; // error
        }
    }

    fclose(f);

    return simulationStatus;
}


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
        printHelp(argv[0]);
        exit(EXIT_SUCCESS);
    }

    int i;

    char fmuPaths[MAX_FMUS][PATH_MAX];
    char outFilePath[PATH_MAX];
    //char fmuNames[MAX_FMUS][PATH_MAX];
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
        printf(VERSION);
        printf("\n");
        exit(EXIT_SUCCESS);
    }

    if(status == 1){
        exit(EXIT_FAILURE);
    }

    if(numFMUs == 0){
        printHelp(argv[0]);
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
