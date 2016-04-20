#include "modelDescription.h"
#include "fmuTemplate.h"

//returns partial derivative of vr with respect to wrt
static fmi2Status getPartial(state_t *s, fmi2ValueReference vr, fmi2ValueReference wrt, fmi2Real *partial) {
    if (vr == VR_ALPHA && wrt == VR_TAU) {
        *partial = s->md.jinv;
        return fmi2OK;
    }
    return fmi2Error;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {
    s->md.alpha  = s->md.jinv  * (s->md.tau - s->md.d*s->md.omega);
    s->md.omega += s->md.alpha * communicationStepSize;
    s->md.theta += s->md.omega * communicationStepSize;
}

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"
