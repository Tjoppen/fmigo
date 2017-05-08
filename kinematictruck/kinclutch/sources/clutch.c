#include "modelDescription.h"
#include "fmuTemplate.h"

//returns partial derivative of vr with respect to wrt
static fmi2Status getPartial(ModelInstance *comp, fmi2ValueReference vr, fmi2ValueReference wrt, fmi2Real *partial) {
    if (vr == VR_ALPHA_E && wrt == VR_TAU_E) {
        *partial = 1/comp->s.md.j_e;
        return fmi2OK;
    }
    if ((vr == VR_ALPHA_L && wrt == VR_TAU_E) || (vr == VR_ALPHA_E && wrt == VR_TAU_L)) {
        *partial = 0;
        return fmi2OK;
    }
    if (vr == VR_ALPHA_L && wrt == VR_TAU_L) {
        *partial =  1/comp->s.md.j_l;
        return fmi2OK;
    }

    return fmi2Error;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
    fmi2Real h = communicationStepSize;
    fmi2Real deltaphi = s->md.theta_e - s->md.theta_l; //angle difference
    
    //Scania's clutch curve
    static const fmi2Real b[5] = { -0.087266462599716474, -0.052359877559829883, 0.0, 0.09599310885968812, 0.17453292519943295 };
    static const fmi2Real c[5] = { -1000, -30, 0, 50, 3500 };

    //look up internal torque based on deltaphi
    //if too low (< -b[0]) then c[0]
    //if too high (> b[4]) then c[4]
    //else lerp between two values in c

    fmi2Real tc = c[0]; //clutch torque
    if (deltaphi <= b[0]) {
        tc = (deltaphi - b[0]) / (b[1] - b[0]) *  970.0 + c[0];
    } else if (deltaphi >= b[4]) {
        tc = (deltaphi - b[4]) / (b[4] - b[3]) * 3450.0 + c[4];
    } else {
        int i;
        for (i = 0; i < 4; i++) {
            if (deltaphi >= b[i] && deltaphi <= b[i+1]) {
                fmi2Real k = (deltaphi - b[i]) / (b[i+1] - b[i]);
                tc = (1-k) * c[i] + k * c[i+1];
                break;
            }
        }
        if (i >= 4) {
            //too high (shouldn't happen)
            tc = c[4];
        }
    }

    //add damping. Scania uses D=100
    tc += 100*(s->md.omega_e - s->md.omega_l);
    //fprintf(stderr, "clutch: deltaphi %f, deltaomega %f -> tc = %f\n", deltaphi, s->md.omega_e - s->md.omega_l, tc);

    s->md.alpha_e = (s->md.tau_e - tc) / s->md.j_e;
    s->md.alpha_l = (s->md.tau_l + tc) / s->md.j_l;
    s->md.omega_e += s->md.alpha_e * h;
    s->md.omega_l += s->md.alpha_l * h;
    s->md.theta_e += s->md.omega_e * h;
    s->md.theta_l += s->md.omega_l * h;
}

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"
