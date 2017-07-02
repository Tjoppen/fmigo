#include "modelDescription.h"

typedef struct {
    int current_step;
    int missed_initial_impulse;
} impulse_simulation;

#define SIMULATION_TYPE impulse_simulation
#define SIMULATION_EXIT_INIT impulse_init
#define SIMULATION_GET impulse_get
#define SIMULATION_SET impulse_set

#include "fmuTemplate.h"

#define PULSE_TYPE_THETA 0
#define PULSE_TYPE_OMEGA 1

static void impulse_get(impulse_simulation *s) {
}

static void impulse_set(impulse_simulation *s) {
}

static void pulse_for_current_step(state_t *s, fmi2Real communicationStepSize) {
    if (s->md.pulse_type == PULSE_TYPE_THETA) {
        if (s->simulation.current_step >= s->md.pulse_start &&
            s->simulation.current_step <  s->md.pulse_start + s->md.pulse_length) {
            s->md.theta = s->md.dc_offset + s->md.pulse_amplitude;
        } else {
            s->md.theta = s->md.dc_offset;
        }

	s->md.itheta = s->md.theta;
        s->md.omega = 0;

        if (s->simulation.current_step == s->md.pulse_start - 1 ||
                s->simulation.missed_initial_impulse > 0) {
            if (communicationStepSize > 0) {
                s->md.omega += s->md.pulse_amplitude / communicationStepSize;
                s->simulation.missed_initial_impulse = 0;
            } else {
                //defer..
                s->simulation.missed_initial_impulse++;
            }
        }

        if (s->simulation.current_step ==  s->md.pulse_start + s->md.pulse_length - 1 ||
                s->simulation.missed_initial_impulse < 0) {
            if (communicationStepSize > 0) {
                s->md.omega -= s->md.pulse_amplitude / communicationStepSize;
                s->simulation.missed_initial_impulse = 0;
            } else {
                s->simulation.missed_initial_impulse--;
            }
        }
    } else {    //PULSE_TYPE_OMEGA
        if (s->simulation.current_step >= s->md.pulse_start &&
            s->simulation.current_step <  s->md.pulse_start + s->md.pulse_length) {
            s->md.omega = s->md.dc_offset + s->md.pulse_amplitude;
        } else {
            s->md.omega = s->md.dc_offset;
        }
    }
}

static fmi2Status impulse_init(ModelInstance *comp) {
    pulse_for_current_step(&comp->s, 0);
    return fmi2OK;
}

//returns partial derivative of vr with respect to wrt
static fmi2Status getPartial(ModelInstance *comp, fmi2ValueReference vr, fmi2ValueReference wrt, fmi2Real *partial) {
    if (vr == VR_ALPHA && wrt == VR_TAU) {
        *partial = 0;
        return fmi2OK;
    }
    return fmi2Error;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real
    communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
    if (s->md.pulse_type == PULSE_TYPE_OMEGA) {
        s->md.theta += s->md.omega * communicationStepSize;
    }

    s->simulation.current_step++;
    pulse_for_current_step(s, communicationStepSize);
    s->md.itheta = (int)s->md.theta;
}

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"
