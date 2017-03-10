#ifndef GETOPT_H
#define GETOPT_H

int getopt(int nargc, char * const nargv[], const char *ostr);

//Jag la till dessa - Tomas, 2015-10-13
extern int     opterr, optind, optopt, optreset;
extern char    *optarg;
//

#endif