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
    fmi2_real_t* ei;
    fmi2_real_t* ei_b;
    fmi2_real_t* x;
    unsigned long failed_steps;
    fmi2_event_info_t eventInfo;
}Backup;
typedef struct fmu_parameters{
    int nx;
    int ni;

    fmi2_import_t* fmi2Instance;
    double t_ok;
    double t_past;
    int count;                    /* number of function evaluations */

    bool stateEvent;
}fmu_parameters;
typedef struct fmu_model{
    cgsl_model *model;
}fmu_model;

void setFMUstate();
Backup* getBackup();
cgsl_simulation* getSim();
fmi2_import_t** getFMU();
void restoreStates(cgsl_simulation *sim, Backup *backup);
void storeStates(cgsl_simulation *sim, Backup *backup);
void runIteration(double t, double dt, Backup *backup);
void prepare();
#endif
