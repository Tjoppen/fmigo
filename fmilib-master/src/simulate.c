#include <fmilib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include "simulate.h"

/**
 * Computes time difference between start and end timespecs, and stores the result in diffTime.
 */
void diff(struct timespec start, struct timespec end, struct timespec * diffTime){
    if ((end.tv_nsec-start.tv_nsec)<0) {
        diffTime->tv_sec = end.tv_sec-start.tv_sec-1;
        diffTime->tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        diffTime->tv_sec = end.tv_sec-start.tv_sec;
        diffTime->tv_nsec = end.tv_nsec-start.tv_nsec;
    }
}

int simulate(fmi1_import_t** fmus,
             char fmuPaths[MAX_FMUS][PATH_MAX],
             int numFMUs,
             connection connections[MAX_CONNECTIONS],
             int numParameters,
             param params[MAX_PARAMS],
             int numConnections,
             double tEnd,
             double timeStep,
             int loggingOn,
             char separator,
             jm_callbacks callbacks,
             int quiet,
             stepfunctionType stepfunc,
             enum FILEFORMAT outFileFormat,
             char outFilePath[PATH_MAX],
             int realTimeMode,
             int * numSteps){
    int i;
    int k;
    double time;                                                // Current time
    double tStart = 0;                                          // Start time
    jm_status_enu_t status;                                     // return code of the fmu functions
    fmi1_string_t fmuLocation = "";                             // path to the fmu as URL, "file://C:\QTronic\sales"
    fmi1_string_t mimeType = "application/x-fmu-sharedlibrary"; // denotes tool in case of tool coupling
    fmi1_real_t timeout = 1000;                                 // wait period in milliseconds, 0 for unlimited wait period
    fmi1_boolean_t visible = 0;                                 // no simulator user interface
    fmi1_boolean_t interactive = 0;                             // simulation run without user interaction
    int nSteps = 0;                                             // Number of steps taken
    char** fmuNames;                                            // Result file names

    // Allocate
    fmuNames =    (char**)calloc(sizeof(char*),numFMUs);

    // Open result file
    FILE * f;
    if(strcmp(outFilePath,"stdout")==0){
        f = stdout;
    } else {
        if (!(f = fopen(outFilePath, "w"))) {
            fprintf(stderr,"Could not write to %s\n", outFilePath);
            return 1;
        }
    }

    // Init FMU names
    for(i=0; i<numFMUs; i++){ 
        fmuNames[i] = calloc(sizeof(char),100);
        sprintf(fmuNames[i],"%s%d",fmi1_import_get_model_name(fmus[i]),i);
    }

    // Write CSV header
    if(outFileFormat == csv){
        writeCsvHeader(f, fmuNames, fmus, numFMUs, separator);
    }

    // Init all the FMUs
    for(i=0; i<numFMUs; i++){ 

        //char * a = fmi_import_create_URL_from_abs_path(&callbacks, fmuPaths[i]);
        fmuLocation = fmi_import_create_URL_from_abs_path(&callbacks, (const char*)fmuPaths[i]);

        // Instantiate the slave
        status = fmi1_import_instantiate_slave (fmus[i], fmuNames[i], fmuLocation, mimeType, timeout, visible, interactive);
        if (status == jm_status_error){
            fprintf(stderr,"Could not instantiate model %s\n",fmuPaths[i]);
            return 1;
        }
        
        // StopTimeDefined=fmiFalse means: ignore value of tEnd
        fmi1_status_t status = fmi1_import_initialize_slave(fmus[i], tStart, (fmi1_boolean_t)0, tEnd);
        if (status != fmi1_status_ok){
            fprintf(stderr,"Could not initialize model %s\n",fmuPaths[i]);
            return 1;
        }
    }

    // Set initial values from the XML file
    for(i=0; i<numFMUs; i++){
        setInitialValues(fmus[i]);
    }

    // Set user-given parameters
    setParams(numFMUs, numParameters, fmus, params);

    time = tStart;

    // Write CSV row for time=0
    if(outFileFormat == csv){
        writeCsvRow(f, fmus, numFMUs, time, separator);
    }

    // enter the simulation loop
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

    struct timespec time1, time2, diffTime;

    while (time < tEnd && status==fmi1_status_ok) {

        if(realTimeMode){
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time1);
        }

        // Step the system of FMUs
        int result = (*stepfunc)(time, timeStep, numFMUs, fmus, numConnections, connections);

        if(result != 0){
            simulationStatus = 1; // Error
            break;
        }

        // Advance time
        time += timeStep;

        if(outFileFormat == csv){
            writeCsvRow(f, fmus, numFMUs, time, separator);
        }

        nSteps++;

        if(realTimeMode){
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time2);

            diff(time1,time2, &diffTime);            
            long int s = timeStep*1e6 - diffTime.tv_sec * 1e6 + diffTime.tv_nsec / 1e3;
            if(s < 0) s = 0;
            usleep(s);
        }
    }
    
    // end simulation
    for(i=0; i<numFMUs; i++){
        fmi1_status_t s = fmi1_import_terminate_slave(fmus[i]);
        fmi1_import_free_slave_instance(fmus[i]);
        if(s != fmi1_status_ok){
            fprintf(stderr,"Error terminating slave instance %d. Continuing...\n",i);
            simulationStatus = 1; // error
        }
    }

    fclose(f);

    *numSteps = nSteps;

    return simulationStatus;
}



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
            fmi1_base_type_enu_t type = fmi1_import_get_variable_base_type(v);

            // Temp things to pass to fmi1_import_set_xxx()
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
