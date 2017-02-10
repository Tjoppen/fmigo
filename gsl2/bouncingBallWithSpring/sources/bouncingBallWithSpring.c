#include "modelDescription.h"
#include "gsl-interface.h"

#define SIMULATION_TYPE cgsl_simulation
#define SIMULATION_INIT springs_init
#define SIMULATION_FREE cgsl_free_simulation

#include "fmuTemplate.h"

static void update_all(modelDescription_t *md){
    md->dx = md->v;
    md->force_out = md->k * (md->x - md->x_in);
    md->dv = -md->force_out - md->g;
}

// offset for event indicator, adds hysteresis and prevents z=0 at restart
#define EPS_INDICATORS 1e-14

static fmi2Status getEventIndicator(modelDescription_t *md, fmi2Real eventIndicators[]){
    /* if(md->dirty) { */
    /*     update_all(md); */
    /*     md->dirty = 0; */
    /* } */
    eventIndicators[0] = md->x + (md->x>0 ? EPS_INDICATORS : -EPS_INDICATORS);

    return fmi2OK;
}

// used to set the next time event, if any.
static void eventUpdate(ModelInstance *comp, fmi2EventInfo *eventInfo) {

    modelDescription_t *md = &comp->s.md;
    if (!(md->x>0)) {
        md->x = - md->c * md->v;
    }
    eventInfo->valuesOfContinuousStatesChanged   = fmi2True;
    eventInfo->nominalsOfContinuousStatesChanged = fmi2False;
    eventInfo->terminateSimulation   = fmi2False;
    eventInfo->nextEventTimeDefined  = fmi2False;
    return;
}

static void SIMULATION_INIT(state_t *s) {
    return;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {
    cgsl_step_to( &s->simulation, currentCommunicationPoint, communicationStepSize );
}

//gcc -g springs.c ../../../templates/gsl2/gsl-interface.c -DCONSOLE -I../../../templates/gsl2 -I../../../templates/fmi2 -lgsl -lgslcblas -lm -Wall
#ifdef CONSOLE
int main(){

  return 0;
}
#else

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"

#endif
