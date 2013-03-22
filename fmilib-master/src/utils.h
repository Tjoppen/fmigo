#include <fmilib.h>
#include <stdio.h>

#ifndef UTILS_H
#define UTILS_H

/**
 * Indicates if the variable in question should be printed to the out file.
 */
int shouldBePrinted(fmi1_import_variable_t * v);

/**
 * Writes header data in CSV format to file. E.g. fmuIdentifyer0.variableName,fmuIdentifyer1.variableName,...
 */
void writeCsvHeader(FILE* file,
                    char** fmuNames,
                    fmi1_import_t ** fmus,
                    int numFMUs,
                    char separator);
/**
 * Writes a single CSV row to an outfile.
 */
void writeCsvRow(FILE* file,
                 fmi1_import_t ** fmus,
                 int numFMUs,
                 fmi1_real_t time,
                 char separator);

#endif /* UTILS_H */