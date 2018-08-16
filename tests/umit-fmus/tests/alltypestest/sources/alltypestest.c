#include <math.h>
#include "modelDescription.h"

#define SIMULATION_EXIT_INIT alltypestest_init

#include "fmuTemplate.h"

static fmi2Status alltypestest_init(ModelInstance *comp) {
    state_t *s = &comp->s;
    s->md.r_out = 0.0;
    s->md.i_out = 0;
    s->md.b_out = 0;
    s->md.s_out[0] = 0;
    return fmi2OK;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
    s->md.r_out = sin(currentCommunicationPoint+communicationStepSize);
    s->md.i_out++;
    s->md.b_out = s->md.i_out; //s->md.i_out % 2;

    int i = 0;
    for (; i < s->md.i_out && i < sizeof(s->md.s_out)-1; i++) {
        s->md.s_out[i] = i % 2 == 1 ? '"' : 'a';
    }
    s->md.s_out[i] = 0;
}

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"

