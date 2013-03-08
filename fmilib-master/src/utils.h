#include <fmilib.h>
#include <stdio.h>

#ifndef UTILS_H
#define UTILS_H

// output time and all non-alias variables in CSV format
// if separator is ',', columns are separated by ',' and '.' is used for floating-point numbers.
// otherwise, the given separator (e.g. ';' or '\t') is to separate columns, and ',' is used 
// as decimal dot in floating-point numbers.
void outputCSVRow(fmi1_import_t * fmu, fmi1_real_t time, FILE* file, char separator, fmi1_boolean_t header);

#endif /* UTILS_H */