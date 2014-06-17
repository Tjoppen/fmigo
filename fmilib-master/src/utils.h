#include <fmilib.h>
#include <stdio.h>

#ifndef UTILS_H
#define UTILS_H

/**
 * @brief Indicates if the variable in question should be printed to the out file. It basically checks the variable type.
 * @return Nonzero if the variable should be printed
 */
int shouldBePrinted(fmi1_import_variable_t * v);

/**
 * @brief Writes header data in CSV format to file.
 * E.g. time,fmuName.variableName1,fmuName.variableName2,... It always prints an extra "time" variable first.
 * @param file
 * @param fmuNames
 * @param fmus
 * @param numFMUs
 * @param separator CSV separator character to use
 */
void writeCsvHeader(FILE* file,
                    char** fmuNames,
                    fmi1_import_t ** fmus,
                    int numFMUs,
                    char separator);
/**
 * @brief Writes a single CSV row to an outfile.
 * @param file File to write to
 * @param fmus
 * @param numFMUs
 * @param time Current simulation time. This value will be printed first.
 * @param separator CSV separator character to use
 */
void writeCsvRow(FILE* file,
                 fmi1_import_t ** fmus,
                 int numFMUs,
                 fmi1_real_t time,
                 char separator);

#endif /* UTILS_H */
