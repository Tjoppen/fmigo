#define MODEL_IDENTIFIER clutch
#define MODEL_GUID "{5f71ee8b-047f-4780-a809-cca8f9efe480}"

enum {
    THETA_E,    //engine angle (output, state)
    OMEGA_E,    //engine angular velocity (output, state)
    ALPHA_E,    //engine angular acceleration (output)
    TAU_E,      //engine torque (input)
    J_E,        //moment of inertia of input gear [1/(kg*m^2)] (parameter)

    THETA_L,    //load angle (output, state)
    OMEGA_L,    //load angular velocity (output, state)
    ALPHA_L,    //load angular acceleration (output)
    TAU_L,      //load torque (input)
    J_L,        //moment of inertia of output gear [1/(kg*m^2)] (parameter)

    NUMBER_OF_REALS
};

#define NUMBER_OF_INTEGERS 0
#define NUMBER_OF_BOOLEANS 0
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0
#define FMI_COSIMULATION

#include "fmuTemplate.h"

//returns partial derivative of vr with respect to wrt
static fmi2Status getPartial(state_t *s, fmi2ValueReference vr, fmi2ValueReference wrt, fmi2Real *partial) {
    if (vr == ALPHA_E && wrt == TAU_E) {
        *partial = 1/s->r[J_E];
        return fmi2OK;
    }
    if ((vr == ALPHA_L && wrt == TAU_E) || (vr == ALPHA_E && wrt == TAU_L)) {
        *partial = 0;
        return fmi2OK;
    }
    if (vr == ALPHA_L && wrt == TAU_L) {
        *partial =  1/s->r[J_L];
        return fmi2OK;
    }

    return fmi2Error;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {
    fmi2Real h = communicationStepSize;
    fmi2Real deltaphi = s->r[THETA_E] - s->r[THETA_L]; //angle difference
    
    //Scania's clutch curve
    static const fmi2Real b[5] = { -0.087266462599716474, -0.052359877559829883, 0.0, 0.09599310885968812, 0.17453292519943295 };
    static const fmi2Real c[5] = { -1000, -30, 0, 50, 3500 };

    //look up internal torque based on deltaphi
    //if too low (< -b[0]) then c[0]
    //if too high (> b[4]) then c[4]
    //else lerp between two values in c

    fmi2Real tc = c[0]; //clutch torque
    if (deltaphi <= b[0]) {
        tc = (deltaphi - b[0]) / 0.034906585039886591 *  970.0 + c[0];
    } else if (deltaphi >= b[4]) {
        tc = (deltaphi - b[4]) / 0.078539816339744828 * 3450.0 + c[4];
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
    tc += 100*(s->r[OMEGA_E] - s->r[OMEGA_L]);
    //fprintf(stderr, "clutch: deltaphi %f, deltaomega %f -> tc = %f\n", deltaphi, s->r[OMEGA_E] - s->r[OMEGA_L], tc);

    s->r[ALPHA_E] = (s->r[TAU_E] - tc) / s->r[J_E];
    s->r[ALPHA_L] = (s->r[TAU_L] + tc) / s->r[J_L];
    s->r[OMEGA_E] += s->r[ALPHA_E] * h;
    s->r[OMEGA_L] += s->r[ALPHA_L] * h;
    s->r[THETA_E] += s->r[OMEGA_E] * h;
    s->r[THETA_L] += s->r[OMEGA_L] * h;
}

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"
