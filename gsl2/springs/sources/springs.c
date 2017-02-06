#include "modelDescription.h"
#include "gsl-interface.h"

#define SIMULATION_TYPE cgsl_simulation
#define SIMULATION_INIT springs_init
#define SIMULATION_FREE cgsl_free_simulation

#include "fmuTemplate.h"

static void updateStates(modelDescription_t *md){
    md->x0 = md->x_in;
    /* md->dx0 = md->v0; */
    /* md->dv0 = -md->k1 * (md->x0 - md->x1); */
    /* md->dx1 = md->v1; */
    /* md->dv1 = -md->k1 * (md->x1 - md->x0) -md->k2 * (md->x1 - md->x_in); */
    //md->x_out = md->x1;
}

// used to set the next time event, if any.
static void eventUpdate(ModelInstance *comp, fmi2EventInfo *eventInfo) {
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
