#ifndef SCVEC3_H
#define SCVEC3_H

#include "stdlib.h"

class scVec3 {
public:
    static double * alloc();
    static void free(double* v);
    static double dot(double* u, double *v);
    static void cross(double* u, double *v, double* target);
    static void set(double* v, double x, double y, double z);
    static void copy(double* out, double* v);
    static void scale(double* out, double scalar);
    static void add(double* out, double * u, double * v);
};

#endif
