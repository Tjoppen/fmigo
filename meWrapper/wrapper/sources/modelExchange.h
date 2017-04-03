#ifndef MODELEXCHANGE_H
#define MODELEXCHANGE_H
#include "modelExchangeFmiInterface.h"
typedef struct TimeLoop
{
    fmi2_real_t t_safe, dt_new, t_crossed, t_end;
} TimeLoop;
typedef struct Backup
{
    double t;
    double h;
    double *dydt;
    unsigned long failed_steps;
    fmi2_event_info_t eventInfo;
}Backup;
typedef struct fmu_parameters{
    int nx;
    int ni;

    fmi2_real_t* ei;
    fmi2_real_t* ei_backup;
    fmi2_import_t* fmi2Instance;
    double t_ok;
    double t_past;
    int count;                    /* number of function evaluations */

    bool stateEvent;
    Backup backup;
}fmu_parameters;
typedef struct fmu_model{
    cgsl_model *model;
}fmu_model;

fmi2_import_t** getfmi2Instance();
double* getContinuousStates();
void meGetReal(const fmi2_value_reference_t* vr, int nvr,  fmi2_real_t* value);

void setFmi2Instance(fmi2_import_t* instance);

void runIteration(double t, double dt);
void prepare();
#endif
