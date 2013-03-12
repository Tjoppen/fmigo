#include <fmilib.h>
#include <stdio.h>

#include "utils.h"

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
                    break;
            }
        }
    } // for
    
    // terminate this row
    fprintf(file, "\n"); 
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
            //fmi1_import_variable_typedef_t* vt = fmi1_import_get_variable_declared_type(v);
            vr = fmi1_import_get_variable_vr(v);
            const char* s = fmi1_import_get_variable_name(v);

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
                fmi1_import_get_string(fmus[j], vr, 1, &s);
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
