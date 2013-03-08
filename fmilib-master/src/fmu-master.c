#include <fmilib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

#define MAX_FMUS 1000
#define MAX_PATH_LENGTH 1000
#define MAX_PARAMS 1000
#define MAX_CONNECTIONS 1000

void printHelp(const char* fmusim) {
    printf("\n  Syntax: %s <N> <fmuPaths> <M> <connections> <K> <params> <tEnd> <h> <logOn> <csvSep>\n", fmusim);
    printf("\n");
    printf("    <N> ............ Number of FMUs included.\n");
    printf("    <fmuPaths> ..... Paths to FMUs, space-separated, relative or absolute.\n");
    printf("    <M> ............ Number of connections.\n");
    printf("    <connections> .. Connections given as 4*M space-separated integers. E.g.\n");
    printf("                     <fromFMU> <fromValueRef> <toFMU> <toValueRef>\n");
    printf("    <K> ............ Number of parameters.\n");
    printf("    <params> ....... Parameters given as 3*K space-separated values e.g.\n");
    printf("                     <FMU:int> <valueRef:int> <value:any>\n");
    printf("    <tEnd> ......... end  time of simulation, optional, defaults to 1.0 sec\n");
    printf("    <h> ............ step size of simulation, optional, defaults to 0.1 sec\n");
    printf("    <logOn> ........ 1 to activate logging,   optional, defaults to 0\n");
    printf("    <csvSep>........ separator in csv file, optional, c for , s for ; default=c\n");
    printf("\n");
    printf("  Note that each connection is specified with four space-separated integers.\n  If you have M connections, there need to be 4*M of them.\n");
    printf("\n");
    printf("  Example with 2 FMUs, 1 connection from FMU0 (value reference 0) to FMU1 (value\n  reference 0), tEnd=5, h=0.1, log=off, separator=commas:\n\n");
    printf("    %s 2 fmu/cs/bouncingBall.fmu fmu/cs/bouncingBall.fmu 1 0 0 1 0 0 5 0.1 0 c\n",fmusim);
    printf("\n");
    printf("  Example with one FMU, 2 connections from valueref 0 to 1 and 2 to 3,\n  tEnd=10, h=0.001, log=on, separator=semicolon:\n\n");
    printf("    %s 1 fmu/cs/bouncingBall.fmu 2 0 0 0 1 0 2 0 3 0 10 0.001 1 s\n",fmusim);
    printf("\n");
}

// output time and all non-alias variables in CSV format
// if separator is ',', columns are separated by ',' and '.' is used for floating-point numbers.
// otherwise, the given separator (e.g. ';' or '\t') is to separate columns, and ',' is used 
// as decimal dot in floating-point numbers.
void outputCSVRow(fmi1_import_t * fmu, fmi1_real_t time, FILE* file, char separator, fmi1_boolean_t header) {
    int k;
    fmi1_real_t r;
    fmi1_integer_t i;
    fmi1_boolean_t b;
    fmi1_string_t s;
    fmi1_value_reference_t vr;
    char buffer[32];
    
    // print first column
    if (header){
        fprintf(file, "time"); 
    } else {
        if (separator==',') 
            fprintf(file, "%.16g", time);
        else {
            // separator is e.g. ';' or '\t'
            //doubleToCommaString(buffer, time);
            fprintf(file, "%f", time);
        }
    }
    
    // print all other columns
    fmi1_import_variable_list_t* vl = fmi1_import_get_variable_list(fmu);
    int n = fmi1_import_get_variable_list_size(vl);
    for (k=0; n; k++) {
        fmi1_import_variable_t* v = fmi1_import_get_variable(vl,k);
        if(!v) break;
        //fmi1_import_variable_typedef_t* vt = fmi1_import_get_variable_declared_type(v);
        vr = fmi1_import_get_variable_vr(v);
        if (header) {
            const char* s = fmi1_import_get_variable_name(v);
            // output names only
            if (separator==',') {
                // treat array element, e.g. print a[1, 2] as a[1.2]
                fprintf(file, "%c", separator);
                while (*s) {
                   if (*s!=' ') fprintf(file, "%c", *s==',' ? '.' : *s);
                   s++;
                }
            } else
                fprintf(file, "%c%s", separator, s);
        } else {
            // output values
            fmi1_base_type_enu_t type = fmi1_import_get_base_type((fmi1_import_variable_typedef_t*)v);
            fmi1_value_reference_t vr[1];
            vr[0] = fmi1_import_get_variable_vr(v);
            fmi1_real_t rr[1];
            fmi1_boolean_t bb[1];
            fmi1_integer_t ii[1];
            fmi1_string_t ss[1];
            switch (type){
                case fmi1_base_type_real :
                    fmi1_import_get_real(fmu, vr, 1, rr);
                    if (separator==',') 
                        fprintf(file, ",%.16g", rr[0]);
                    else {
                        // separator is e.g. ';' or '\t'
                        //doubleToCommaString(buffer, r);
                        fprintf(file, "%c%f", separator, rr[0]);       
                    }
                    break;
                case fmi1_base_type_int:
                case fmi1_base_type_enum:
                    fmi1_import_get_integer(fmu, vr, 1, ii);
                    fprintf(file, "%c%d", separator, ii[0]);
                    break;
                case fmi1_base_type_bool:
                    fmi1_import_get_boolean(fmu, vr, 1, bb);
                    fprintf(file, "%c%d", separator, bb[0]);
                    break;
                case fmi1_base_type_str:
                    fmi1_import_get_string(fmu, vr, 1, &s);
                    fprintf(file, "%c%s", separator, ss[0]);
                    break;
                default: 
                    fprintf(file, "NoValueForType");
            }
        }
    } // for
    
    // terminate this row
    fprintf(file, "\n"); 
}


// simulate the given FMUs
static int simulate(fmi1_import_t** fmus, fmi1_string_t fmuFileNames[], int N, int* connections, int K, fmi1_string_t params[], int M,
                    double tEnd, double h, int loggingOn, char separator, jm_callbacks callbacks){

    int i;
    int k;
    double time;                        // Current time
    double tStart = 0;                  // Start time
    fmi1_string_t** guids;
    jm_status_enu_t status;           // return code of the fmu functions
    fmi1_string_t fmuLocation = "";           // path to the fmu as URL, "file://C:\QTronic\sales"
    fmi1_string_t mimeType = "application/x-fmu-sharedlibrary"; // denotes tool in case of tool coupling
    fmi1_real_t timeout = 1000;             // wait period in milliseconds, 0 for unlimited wait period
    fmi1_boolean_t visible = 0;      // no simulator user interface
    fmi1_boolean_t interactive = 0;  // simulation run without user interaction
    int nSteps = 0;                     // Number of steps taken
    FILE** files;                       // result files
    char** fileNames;                   // Result file names

    // Allocate
    guids =        (fmi1_string_t**)calloc(sizeof(fmi1_string_t*),N);
    for(i=0; i<N; i++){
        guids[i] = calloc(sizeof(char),100);
    }
    files =       (FILE**)calloc(sizeof(FILE*),N);
    fileNames =   (char**)calloc(sizeof(char*),N);

    // Init all the FMUs
    for(i=0; i<N; i++){ 

        strcpy((char*)guids[i], fmi1_import_get_GUID ( fmus[i] ) );
        char * a = fmi_import_create_URL_from_abs_path(&callbacks, fmuFileNames[i]);
        fmuLocation = fmi_import_create_URL_from_abs_path(&callbacks, (const char*)fmuFileNames[i]);

        status = fmi1_import_instantiate_slave (fmus[i], "lol", fmuLocation, mimeType, timeout, visible, interactive);
        if (status == jm_status_error){
            printf("could not instantiate model\n");
            return 1;
        }

        // Generate out file name like this: "resultN.csv" where N is the FMU index
        fileNames[i] = calloc(sizeof(char),100);
        sprintf(fileNames[i],"result%d.csv",i);

        // open result file
        if (!(files[i] = fopen(fileNames[i], "w"))) {
            printf("could not write %s\n", fileNames[i]);
            return 1; // failure
        }
        
        // StopTimeDefined=fmiFalse means: ignore value of tEnd
        fmi1_status_t status = fmi1_import_initialize_slave(fmus[i], tStart, (fmi1_boolean_t)0, tEnd);
        if (status != fmi1_status_ok){
            printf("Could not initialize model %s",fmuFileNames[i]);
            return 0;
        }


        // Set initial values
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

            // Set initial values from the XML file
            if(fmi1_import_get_variable_has_start(v)){
                switch (type){
                    case fmi1_base_type_real:
                        lol[0] = fmi1_import_get_real_variable_start((fmi1_import_real_variable_t*) v);
                        fmi1_import_set_real(fmus[i],   vr,   1, lol);
                        break;
                    case fmi1_base_type_int:
                    case fmi1_base_type_enum:
                        innt[0] = fmi1_import_get_integer_variable_start((fmi1_import_integer_variable_t*) v);
                        fmi1_import_set_integer(fmus[i],   vr,   1, innt);
                        break;
                    case fmi1_base_type_bool:
                        boool[0] = fmi1_import_get_boolean_variable_start((fmi1_import_bool_variable_t*) v);
                        fmi1_import_set_boolean(fmus[i],   vr,   1, boool);
                        break;
                    case fmi1_base_type_str:
                        striing[0] = fmi1_import_get_string_variable_start((fmi1_import_string_variable_t*) v);
                        fmi1_import_set_string(fmus[i],   vr,   1, striing);
                        break;
                    default: 
                        printf("Could not determine type of value reference %d in FMU %d. Continuing without connection value transfer...\n", vr[0],i);
                        return 1;
                        break;
                }
            }

            // Set initial values from the command line, overrides the XML init values
            int j;
            for(j=0; j<K*3; j+=3){
                // Get FMU index and value reference to set
                int fmuIndex = -1;
                int valueReference = -1;
                if (sscanf(params[j],"%d", &fmuIndex) != 1){
                    printf("Could not scan parameter\n");
                    return 1;
                }
                if (sscanf(params[j+1],"%d", &valueReference) != 1){
                    printf("Could not scan parameter value reference\n");
                    return 1;
                }

                if( i == fmuIndex && // Correct FMU
                    vr[0] == valueReference // Correct valuereference
                    ) {

                    printf("Setting parameter %d of FMU%d, vr=%d to ",k,fmuIndex,valueReference);
                    float tmpFloat;
                    int tmpInt;

                    switch (type){
                        // Real
                        case fmi1_base_type_real:
                            if (sscanf(params[j+2],"%f", &tmpFloat) != 1){
                                printf("Could not scan parameter %d real value\n",k);
                                return 1;
                            }
                            lol[0] = tmpFloat;
                            printf("%f\n",tmpFloat);
                            fmi1_import_set_real(fmus[i],   vr,   1, lol);
                            break;

                        // Integer
                        case fmi1_base_type_int:
                        case fmi1_base_type_enum:
                            if (sscanf(params[j+2],"%d", &tmpInt) != 1){
                                printf("Could not scan parameter %d integer value\n",k);
                                return 1;
                            }
                            innt[0] = tmpInt;
                            printf("%d\n",tmpInt);
                            fmi1_import_set_integer(fmus[i],   vr,   1, innt);
                            break;

                        // Boolean
                        case fmi1_base_type_bool:
                            // Use integer input
                            if (sscanf(params[j+2],"%d", &tmpInt) != 1 || (tmpInt!=0 && tmpInt!=1)){
                                printf("Could not scan parameter %d boolean (integer) value\n",k);
                                return 1;
                            }
                            boool[0] = tmpInt;
                            printf("%d\n",tmpInt);
                            //boool[0] = fmi1_import_get_boolean_variable_start((fmi1_import_bool_variable_t*) v);
                            fmi1_import_set_boolean(fmus[i],   vr,   1, boool);
                            break;

                        // String
                        case fmi1_base_type_str:
                            // Use the raw string
                            striing[0] = params[j+2];
                            printf("%s\n",params[j+2]);
                            //striing[0] = fmi1_import_get_string_variable_start((fmi1_import_string_variable_t*) v);
                            fmi1_import_set_string(fmus[i],   vr,   1, striing);
                            break;

                        default: 
                            printf("Could not determine type of value reference %d in FMU %d. Continuing without connection value transfer...\n", vr[0],i);
                            return 1;
                            break;
                    }
                }
            }
        }



        // output solution for time t0
        outputCSVRow(fmus[i], tStart, files[i], separator, 1);  // output column names
        outputCSVRow(fmus[i], tStart, files[i], separator, 0); // output values
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
    while (time < tEnd && status==fmi1_status_ok) {

        int ci;
        for(ci=0; ci<M; ci++){
            found = 0;
            int fmuFrom = connections[ci*4 + 0];
            vrFrom[0] =   connections[ci*4 + 1];
            int fmuTo =   connections[ci*4 + 2];
            vrTo[0] =     connections[ci*4 + 3];

            //printf("%d %d %d %d\n",connections[ci*4 + 0],connections[ci*4 + 1],connections[ci*4 + 2],connections[ci*4 + 3]);

            // Get variable list of both FMU participating in the connection
            fmi1_import_variable_list_t* varsFrom = fmi1_import_get_variable_list(fmus[fmuFrom]);
            fmi1_import_variable_list_t* varsTo =   fmi1_import_get_variable_list(fmus[fmuTo]);
            int numFrom = fmi1_import_get_variable_list_size(varsFrom);
            int numTo   = fmi1_import_get_variable_list_size(varsTo);
            for (k=0; numFrom; k++) {
                fmi1_import_variable_t* v = fmi1_import_get_variable(varsFrom,k);
                if(!v) break;
                //vrFrom[0] = fmi1_import_get_variable_vr(v);
                fmi1_base_type_enu_t typeFrom = fmi1_import_get_base_type((fmi1_import_variable_typedef_t*)v);

                // Now find the input variable
                for (l=0; !found && l<numTo; l++) {

                    fmi1_import_variable_t* vTo = fmi1_import_get_variable(varsTo,l);
                    if(!vTo) break;
                    //vrTo[0] = fmi1_import_get_variable_vr(vTo);
                    fmi1_base_type_enu_t typeTo = fmi1_import_get_base_type((fmi1_import_variable_typedef_t*)vTo);

                    // Found the input and output. Check if they have equal types
                    if(typeFrom == typeTo){

                        //printf("Connection %d at T=%g: Transferring value from FMU%d (vr=%d) to FMU%d (vr=%d)\n",ci,time,fmuFrom,vrFrom[0],fmuTo,vrTo[0]);

                        switch (typeFrom){
                            case fmi1_base_type_real :
                                fmi1_import_get_real(fmus[fmuFrom], vrFrom, 1, rr);
                                fmi1_import_set_real(fmus[fmuTo],   vrTo,   1, rr);
                                break;
                            case fmi1_base_type_int:
                            case fmi1_base_type_enum:
                                fmi1_import_get_integer(fmus[fmuFrom], vrFrom, 1, ii);
                                fmi1_import_set_integer(fmus[fmuTo],   vrTo,   1, ii);
                                break;
                            case fmi1_base_type_bool:
                                fmi1_import_get_boolean(fmus[fmuFrom], vrFrom, 1, bb);
                                fmi1_import_set_boolean(fmus[fmuTo],   vrTo,   1, bb);
                                break;
                            case fmi1_base_type_str:
                                fmi1_import_get_string(fmus[fmuFrom], vrFrom, 1, ss);
                                fmi1_import_set_string(fmus[fmuTo],   vrTo,   1, ss);
                                break;
                            default: 
                                printf("Could not determine type of value reference %d in FMU %d. Continuing without connection value transfer...\n", vrFrom[0],fmuFrom);
                                break;
                        }

                        found = 1;
                    } else {
                        printf("Connection between FMU %d (value ref %d) and %d (value ref %d) had incompatible data types!\n",fmuFrom,vrFrom[0],fmuTo,vrTo[0]);
                    }
                }
            }
        }

        // Step all the FMUs
        for(i=0; i<N; i++){
            fmi1_status_t s = fmi1_import_do_step (fmus[i], time, h, 1);
            //status = fmus[i]->doStep(c[i], time, h, fmiTrue);
            if(s != fmi1_status_ok){
                status = s;
                printf("doStep() of model %s didn't return fmiOK! Exiting...",fmuFileNames[i]);
                return 0;
            }
        }

        // Advance time
        time += h;

        // Write to files
        for(i=0; i<N; i++){
            outputCSVRow(fmus[i], time, files[i], separator, 0); // output values for this step
        }
        nSteps++;
    }
    
    // end simulation
    for(i=0; i<N; i++){
        fmi1_status_t s = fmi1_import_terminate_slave(fmus[i]);
        fmi1_import_free_slave_instance (fmus[i]);
        if(s != fmi1_status_ok) printf("Error terminating slave instance %d\n",i);
    }
  
    // print simulation summary 
    printf("Simulation from %g to %g terminated successful\n", tStart, tEnd);
    printf("  steps ............ %d\n", nSteps);
    printf("  fixed step size .. %g\n", h);
    for(i=0; i<N; i++){
        fclose(files[i]);
        printf("CSV file '%s' written\n", fileNames[i]);
    }

    return 1; // success
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

int doLog = 0;

void importlogger(jm_callbacks* c, jm_string module, jm_log_level_enu_t log_level, jm_string message){
    if(doLog)
        printf("%10s,\tlog level %2d:\t%s\n", module, log_level, message);
}

void fmi1Logger(fmi1_component_t c, fmi1_string_t instanceName, fmi1_status_t status, fmi1_string_t category, fmi1_string_t message, ...){
    if(doLog)
        printf("%s\n",message);
}
void fmi1StepFinished(fmi1_component_t c, fmi1_status_t status){
    
}

typedef struct __connection{
    int fromFMU;
    int fromOutputVR;
    int toFMU;
    int toInputVR;
 
} connection;

typedef struct __param{
    int fmuIndex;
    int valueReference;
    int valueType; // 0:real, 1:int, 2:bool, 3:string

    char stringValue[1000];
    int intValue;
    double realValue;
    int boolValue;
} param;

/**
 * Parses command line using getopt
 * 
 * master [OPTIONS] [FMUs...]
 * 
 * Flags:
 *   -t      Simulation end time in seconds e.g. 123.4. Defaults to 1.0.
 *   -c      Connections specification e.g. "0,0,1,0:0,0,1,0" (fromFMU fromOutput toFMU toInput). Defaults to no connections.
 *   -h      Timestep size in seconds. Defaults to 0.01.
 *   -s      CSV separator. Defaults to comma (,)
 *   -o      Output CSV file. If not given, stdout is output.
 *   -p      Parameters given as "0 0 param1:1 1 param2"
 */
int parseArguments2(int argc,
                    char *argv[],
                    int* numFMUs,
                    char fmuFilePaths[MAX_FMUS][MAX_PATH_LENGTH],
                    int* numConnections,
                    connection connections[MAX_CONNECTIONS],
                    int* numParameters,
                    param params[MAX_PARAMS],
                    double* tEnd,
                    double* timeStepSize,
                    int* loggingOn,
                    char* csv_separator,
                    char outFilePath[MAX_PATH_LENGTH],
                    int* outFileGiven){
    int index;
    int c;

    opterr = 0;

    *outFileGiven = 0;

    while ((c = getopt (argc, argv, "t:c:h:s:o:p:")) != -1){

        int n=0;
        int skip=0;
        int l=strlen(optarg);
        int cont=1;
        int i=0;
        connection * conn;

        switch (c) {
        case 'c':
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
            
        case 't':
            sscanf(optarg, "%lf", tEnd);
            break;
            
        case 'h':
            sscanf(optarg,"%lf", timeStepSize);
            break;
        case 's':
            *csv_separator = optarg[0];
            break;
        case 'o':
            strcpy(outFilePath,optarg);
            *outFileGiven = 1;
            break;
        case 'p':
            // Real if number and contains .
            // Int if number and only digits
            // Bool if "true" or "false"
            // Else: string
            printf("p=%s\n",optarg);
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
        printf ("FMU path: %s\n", argv[index]);
        strcpy( fmuFilePaths[i] , argv[index] );
        i++;
    }
    *numFMUs = i;

    return 0;
}

int main( int argc, char *argv[] ) {

    int i;

    if(0){

        char fmuPaths[MAX_FMUS][MAX_PATH_LENGTH];
        char outFilePath[MAX_PATH_LENGTH];
        param params2[MAX_PARAMS];
        connection connections2[MAX_CONNECTIONS];

        int N2=0, K2=0;
        int M2 = 0;
        double tEnd2 = 1.0;
        double h2 = 0.1;
        int loggingOn2 = 1;
        char csv_separator2 = ',';
        int outFileGiven;
        parseArguments2(argc, argv, &N2, fmuPaths, &M2, connections2, &K2, params2, &tEnd2, &h2, &loggingOn2, &csv_separator2, outFilePath, &outFileGiven);

        printf("FMUS (%d)\n",N2);
        for(i=0; i<N2; i++){
            printf("    FMU %d: %s\n",i,fmuPaths[i]);
        }

        printf("\nCONNECTIONS (%d)\n",M2);
        for(i=0; i<M2; i++){
            printf("    FMU%d[%d] ---> FMU%d[%d]\n",connections2[i].fromFMU,
                                                    connections2[i].fromOutputVR,
                                                    connections2[i].toFMU,
                                                    connections2[i].toInputVR);
        }

        if(outFileGiven==1){
            printf("\nOUTPUT CSV FILE\n");
            printf("    %s\n",outFilePath);
        }
    }


    int N=1, K=0;
    int *connections;
    fmi1_string_t *fileNames;
    fmi1_string_t *params;
    int M = 0;
    double tEnd = 1.0;
    double h = 0.1;
    int loggingOn = 1;
    char csv_separator = ',';
    parseArguments(argc, argv, &N, &fileNames, &M, &connections, &K, &params, &tEnd, &h, &loggingOn, &csv_separator);


    /*
    printf("num params %d\n",K);
    for(i=0; i<K; i+=3){
        printf("FMU%s vr=%s set to %s\n",params[i],params[i+1],params[i+2]);
    }
    */

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

    fmi1_import_t** fmus =            calloc(sizeof(fmi1_import_t*),N);
    fmi_import_context_t** contexts = calloc(sizeof(fmi_import_context_t*),N);
    fmi_version_enu_t* versions =     calloc(sizeof(fmi_version_enu_t),N);

    // Load all FMUs
    for(i=0; i<N; i++){
        
        const char* tmpPath = fmi_import_mk_temp_dir (&callbacks, "/tmp", "FMUMaster");

        contexts[i] = fmi_import_allocate_context(&callbacks);
        versions[i] = fmi_import_get_fmi_version(contexts[i], fileNames[i], tmpPath);

        if(versions[i] == fmi_version_1_enu) {

            // Check file contents
            //FILE* f = fopen(tmpPath);
            //fclose(f);

            fmus[i] = fmi1_import_parse_xml ( contexts[i], tmpPath );
            int registerGlobally = 0;
            fmi1_callback_functions_t callBackFunctions;
            callBackFunctions.logger = fmi1Logger;
            callBackFunctions.allocateMemory = calloc;
            callBackFunctions.freeMemory = free;
            callBackFunctions.stepFinished = fmi1StepFinished;
            jm_status_enu_t status = fmi1_import_create_dllfmu  ( fmus[i], callBackFunctions, registerGlobally );
            if(status == jm_status_success){
                // Successfully loaded DLL

            } else {
                fmi_import_free_context(contexts[i]);
                printf("There was an error loading the FMU dll.\n");
                return 1;
            }
        } else if(versions[i] == fmi_version_2_0_enu) {
        } else {
            fmi_import_free_context(contexts[i]);
            printf("Only versions 1.0 and 2.0 are supported so far\n");
            return 1;
        }
    }

    // All loaded. Simulate.
    simulate(fmus, fileNames, N, connections, K, params, M, tEnd, h, loggingOn, csv_separator,callbacks);

    // Clean up
    for(i=0; i<N; i++){
        fmi1_import_destroy_dllfmu(fmus[i]);
        fmi_import_free_context(contexts[i]);
        free(fmus);
        free(contexts);
        free(versions);
    }
    free(fileNames);
    free(connections);

    return( 0 );
}