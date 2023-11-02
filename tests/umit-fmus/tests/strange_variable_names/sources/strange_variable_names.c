#include "modelDescription.h"

#define SIMULATION_EXIT_INIT strange_variable_names_init

#include "fmuTemplate.h"

static fmi2Status strange_variable_names_init(ModelInstance *comp) {
    state_t *s = &comp->s;
    s->md.dot_in    = s->md.dot_out   = s->md.dot_0;
    s->md.colon_in  = s->md.colon_out = s->md.colon_0;
    s->md.der_in_   = s->md.der_out_  = s->md.der_0_;
    strlcpy2(s->md.space_in,  s->md.space_0, sizeof(s->md.space_in));
    strlcpy2(s->md.space_out, s->md.space_0, sizeof(s->md.space_out));
    return fmi2OK;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
    s->md.dot_out   = s->md.dot_in;
    s->md.colon_out = s->md.colon_in;
    s->md.der_out_  = s->md.der_in_;
    strlcpy2(s->md.space_out, s->md.space_in, sizeof(s->md.space_out));
}

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"
