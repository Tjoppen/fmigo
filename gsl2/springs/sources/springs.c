#include "modelDescription.h"
#include "gsl-interface.h"

#define SIMULATION_TYPE cgsl_simulation
#define SIMULATION_INIT strings_init
#define SIMULATION_FREE cgsl_free_simulation

#include "fmuTemplate.h"


static void strings_init(state_t *s) {
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {
    //cgsl_step_to( &s->simulation, currentCommunicationPoint, communicationStepSize );
}
fmi2Status getUserDefinedDerivatives(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]){
    int i;
    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case VR_X0: value[i] = md->v0; break;
        case VR_X1: value[i] = md->v1; break;
        case VR_V0: value[i] = md->k1 * (md->x0 - md->x1); break;
        case VR_V1: value[i] = md->k1 * (md->x1 - md->x0) -
                               md->k2 * (md->x1 - md->x_in); break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

// used to set the next time event, if any.
void eventUpdate(ModelInstance *comp, fmi2EventInfo *eventInfo) {
}

//gcc -g clutch.c ../../../templates/gsl2/gsl-interface.c -DCONSOLE -I../../../templates/gsl2 -I../../../templates/fmi2 -lgsl -lgslcblas -lm -Wall
#ifdef CONSOLE
int main(){

  return 0;
}
#else

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"

#endif
