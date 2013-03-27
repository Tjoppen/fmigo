#include <fmilib.h>
#include <stdio.h>

#include "main.h"
#include "utils.h"

/**
 * Transfer values in a connection
 */
void fmi1TransferConnectionValues(connection c, fmi1_import_t ** fmus){

    int ci, found=0, k, i, l;

    // Temp for transfering values
    fmi1_base_type_enu_t type;
    fmi1_value_reference_t vrFrom[1];
    fmi1_value_reference_t vrTo[1];
    fmi1_real_t rr[2];
    fmi1_boolean_t bb[2];
    fmi1_integer_t ii[2];
    fmi1_string_t ss[2];

    int fmuFrom = c.fromFMU;
    vrFrom[0] =   c.fromOutputVR;
    int fmuTo =   c.toFMU;
    vrTo[0] =     c.toInputVR;

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

                //printf("Connection %d: Transferring value from FMU%d (vr=%d) to FMU%d (vr=%d)\n",ci,fmuFrom,vrFrom[0],fmuTo,vrTo[0]);

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


int shouldBePrinted(fmi1_import_variable_t * v){
    // Get variability
    fmi1_variability_enu_t variability = fmi1_import_get_variability(v);
    // Don't print parameters
    switch(variability){
        case fmi1_variability_enu_continuous:
        case fmi1_variability_enu_discrete:
            return 1;
        case fmi1_variability_enu_constant:
        case fmi1_variability_enu_parameter:
        case fmi1_variability_enu_unknown:
            return 0;
    }
}

void writeCsvHeader(FILE* file,
                    char** fmuNames,
                    fmi1_import_t ** fmus,
                    int numFMUs,
                    char separator){
    int k,j;
    fmi1_real_t r;
    fmi1_integer_t i;
    fmi1_boolean_t b;
    fmi1_string_t s;
    fmi1_value_reference_t vr;
    char buffer[32];
    
    // First column is always time
    fprintf(file, "time"); 
    
    for(j=0; j<numFMUs; j++){
        // print all other columns
        fmi1_import_variable_list_t* vl = fmi1_import_get_variable_list(fmus[j]);
        int n = fmi1_import_get_variable_list_size(vl);
        for (k=0; n; k++) {
            fmi1_import_variable_t* v = fmi1_import_get_variable(vl,k);
            if(!v) break;
            //fmi1_import_variable_typedef_t* vt = fmi1_import_get_variable_declared_type(v);
            vr = fmi1_import_get_variable_vr(v);

            if(!shouldBePrinted(v))
                continue;

            const char* s = fmi1_import_get_variable_name(v);
            // output names only
            if (separator==',') {
                // treat array element, e.g. print a[1, 2] as a[1.2]
                fprintf(file, "%c%s.", separator,fmuNames[j]);
                while(*s){
                   if(*s!=' ') fprintf(file, "%c", *s==',' ? '.' : *s);
                   s++;
                }
            } else
                fprintf(file, "%c%s.%s", separator, fmuNames[j], s);
        } // for
    }
    
    // terminate this row
    fprintf(file, "\n"); 
}

void writeCsvRow(FILE* file,
                 fmi1_import_t ** fmus,
                 int numFMUs,
                 fmi1_real_t time,
                 char separator){
    int k,j;
    fmi1_real_t r;
    fmi1_integer_t i;
    fmi1_boolean_t b;
    fmi1_string_t s;
    fmi1_value_reference_t vr;
    char buffer[32];
    
    // First column is always time
    fprintf(file, "%.16g",time); 
    
    for(j=0; j<numFMUs; j++){
        // print all other columns
        fmi1_import_variable_list_t* vl = fmi1_import_get_variable_list(fmus[j]);
        int n = fmi1_import_get_variable_list_size(vl);
        for (k=0; n; k++) {
            fmi1_import_variable_t* v = fmi1_import_get_variable(vl,k);
            if(!v) break;

            if(!shouldBePrinted(v))
                continue;

            //fmi1_import_variable_typedef_t* vt = fmi1_import_get_variable_declared_type(v);
            vr = fmi1_import_get_variable_vr(v);
            const char* s = fmi1_import_get_variable_name(v);

            // output values
            fmi1_base_type_enu_t type = fmi1_import_get_variable_base_type(v);
            fmi1_value_reference_t vr[1];
            vr[0] = fmi1_import_get_variable_vr(v);
            fmi1_real_t rr[1];
            fmi1_boolean_t bb[1];
            fmi1_integer_t ii[1];
            fmi1_string_t ss[1000];
            switch (type){
            case fmi1_base_type_real :
                fmi1_import_get_real(fmus[j], vr, 1, rr);
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
                fmi1_import_get_integer(fmus[j], vr, 1, ii);
                fprintf(file, "%c%d", separator, ii[0]);
                break;
            case fmi1_base_type_bool:
                fmi1_import_get_boolean(fmus[j], vr, 1, bb);
                fprintf(file, "%c%d", separator, bb[0]);
                break;
            case fmi1_base_type_str:
                fmi1_import_get_string(fmus[j], vr, 1, ss);
                fprintf(file, "%c%s", separator, ss[0]);
                break;
            default: 
                fprintf(file, "NoValueForType");
                break;
            }
        }
    }
    
    // terminate this row
    fprintf(file, "\n"); 
}
