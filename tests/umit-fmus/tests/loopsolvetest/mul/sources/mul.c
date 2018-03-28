#include "modelDescription.h"
#include "fmuTemplate.h"

#define HAVE_INITIALIZATION_MODE
static int get_initial_states_size(state_t *s) {
  return 0;
}

static void get_initial_states(state_t *s, double *initials) {
}

static int sync_out(double t, double dt, int n, const double outputs[], void * params) {
  state_t *s = params;
  s->md.out = s->md.in1 * s->md.in2;
  return 0;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
  s->md.out = s->md.in1 * s->md.in2;
}

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"
