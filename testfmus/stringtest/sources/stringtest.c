#include "modelDescription.h"

#define SIMULATION_INIT stringtest_init

#include "fmuTemplate.h"

static void stringtest_init(state_t *s) {
    snprintf(s->md.s_in, sizeof(s->md.s_in), "%s", s->md.s0);
    snprintf(s->md.s_out, sizeof(s->md.s_out), "%s", s->md.s0);
    snprintf(s->md.s_in2, sizeof(s->md.s_in2), "%s", s->md.s02);
    snprintf(s->md.s_out2, sizeof(s->md.s_out2), "%s", s->md.s02);
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {
    snprintf(s->md.s_out, sizeof(s->md.s_out), "%s", s->md.s_in);
    snprintf(s->md.s_out2, sizeof(s->md.s_out2), "%s", s->md.s_in2);
}

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"
