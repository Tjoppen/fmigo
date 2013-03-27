#include <fmilib.h>
#include <stdio.h>

#include "main.h"

int fmi2JacobiStep( double time,
                    double communicationTimeStep,
                    int numFMUs,
                    fmi2_import_t ** fmus,
                    int numConnections,
                    connection connections[MAX_CONNECTIONS]){
    return 1;
}

/**
 * Jacobi is a stepping method where we step the subsystems in parallel.
 * The order of connections will therefore not matter.
 */
int fmi1JacobiStep( double time,
                    double communicationTimeStep,
                    int numFMUs,
                    fmi1_import_t ** fmus,
                    int numConnections,
                    connection connections[MAX_CONNECTIONS]){

    int ci, found, k, i, l;

    // Temp for transfering values
    fmi1_base_type_enu_t type;
    fmi1_value_reference_t vrFrom[1];
    fmi1_value_reference_t vrTo[1];
    fmi1_real_t rr[2];
    fmi1_boolean_t bb[2];
    fmi1_integer_t ii[2];
    fmi1_string_t ss[2];

    for(ci=0; ci<numConnections; ci++){ // Loop over connections
        found = 0;
        int fmuFrom = connections[ci].fromFMU;
        vrFrom[0] =   connections[ci].fromOutputVR;
        int fmuTo =   connections[ci].toFMU;
        vrTo[0] =     connections[ci].toInputVR;

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
            fmi1_base_type_enu_t typeFrom = fmi1_import_get_variable_base_type(v);

            // Now find the input variable
            for (l=0; !found && l<numTo; l++) {

                fmi1_import_variable_t* vTo = fmi1_import_get_variable(varsTo,l);
                if(!vTo) break;
                //vrTo[0] = fmi1_import_get_variable_vr(vTo);
                fmi1_base_type_enu_t typeTo = fmi1_import_get_variable_base_type(vTo);

                // Found the input and output. Check if they have equal types
                if(typeFrom == typeTo){

                    //printf("Connection %d at T=%g: Transferring value from FMU%d (vr=%d) to FMU%d (vr=%d)\n",ci,time,fmuFrom,vrFrom[0],fmuTo,vrTo[0]);

                    switch (typeFrom){
                    case fmi1_base_type_real:
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
    fmi1_status_t status = fmi1_status_ok;
    for(i=0; i<numFMUs; i++){
        fmi1_status_t s = fmi1_import_do_step (fmus[i], time, communicationTimeStep, 1);
        if(s != fmi1_status_ok){
            status = s;
            printf("doStep() of FMU %d didn't return fmiOK! Exiting...\n",i);
            return 1;
        }
    }

    return 0;
}
