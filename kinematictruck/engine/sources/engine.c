#include "modelDescription.h"
#include "fmuTemplate.h"

//returns partial derivative of vr with respect to wrt
static fmi2Status getPartial(ModelInstance *comp, fmi2ValueReference vr, fmi2ValueReference wrt, fmi2Real *partial) {
    if (vr == VR_ALPHA && wrt == VR_TAU) {
        *partial = comp->s.md.jinv;
        return fmi2OK;
    }
    return fmi2Error;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {
    //controller. beta = gas pedal
    //fprintf(stderr, "omega_l0 = %f, omega_l = %f\n", s->md.omega_l0, s->md.omega_l);
    s->md.beta = s->md.kp*(s->md.omega_l0 - s->md.omega_l);

    if (s->md.clamp_beta) {
        //clamp beta between zero and one
        if (s->md.beta < 0) s->md.beta = 0;
        if (s->md.beta > 1) s->md.beta = 1;
    }

    s->md.alpha =   s->md.jinv * (s->md.beta*s->md.tau_max + s->md.tau - s->md.d*s->md.omega);
    //fprintf(stderr, "#################### ALPHA: %f = %f * (%f * %f + %f - %f * %f\n", s->md.alpha, s->md.jinv, s->md.beta, s->md.tau_max, s->md.tau, s->md.d, s->md.omega);
    s->md.omega += s->md.alpha * communicationStepSize;
    s->md.theta += s->md.omega * communicationStepSize;
}

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"
