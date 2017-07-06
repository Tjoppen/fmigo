#include "modelDescription.h"
#include "fmuTemplate.h"

//returns partial derivative of vr with respect to wrt
static fmi2Status getPartial(ModelInstance *comp, fmi2ValueReference vr, fmi2ValueReference wrt, fmi2Real *partial) {
    fmi2Real a2 = comp->s.md.alpha*comp->s.md.alpha;

    if (vr == VR_OMEGADOT_E && wrt == VR_TAU_E) {
        *partial = a2/(comp->s.md.j2 + a2*comp->s.md.j1);
        return fmi2OK;
    }
    if ((vr == VR_OMEGADOT_L && wrt == VR_TAU_E) || (vr == VR_OMEGADOT_E && wrt == VR_TAU_L)) {
        *partial = comp->s.md.alpha/(comp->s.md.j2 + a2*comp->s.md.j1);
        return fmi2OK;
    }
    if (vr == VR_OMEGADOT_L && wrt == VR_TAU_L) {
        *partial =  1/(comp->s.md.j2 + a2*comp->s.md.j1);   //load side sees a higher moment of inertia (lower inverse)
        return fmi2OK;
    }

    return fmi2Error;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
    fmi2Real a = s->md.alpha, a2 = a*a, h = communicationStepSize;
    fmi2Real A = s->md.tau_e - s->md.d1*s->md.omega_e;
    fmi2Real B = s->md.tau_l - s->md.d2*s->md.omega_l;

    fmi2Real lambda = (-s->md.omega_e + a*s->md.omega_l - h*s->md.tau_e/s->md.j1 + h*a*s->md.tau_l/s->md.j2) / (s->md.j1 + a2*s->md.j2);
    
    fmi2Real delta1 = (   lambda + h*A) / s->md.j1;
    fmi2Real delta2 = (-a*lambda + h*B) / s->md.j2;

    s->md.omegadot_e  = delta1 / h;
    s->md.omega_e    += delta1;
    s->md.theta_e    += s->md.omega_e * h;

    s->md.omegadot_l  = delta2 / h;
    s->md.omega_l    += delta2;
    s->md.theta_l    += s->md.omega_l * h;
}

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"
