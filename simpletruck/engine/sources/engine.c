#define MODEL_IDENTIFIER engine
#define MODEL_GUID "{a6a01bd9-863d-4a7c-ac09-58b7f438895b}"

enum {
    THETA,      //angle (output, state)
    OMEGA,      //angular velocity (output, state)
    ALPHA,      //angular acceleration (output)
    TAU,        //coupling torque (input)
    JINV,       //inverse of moment of inertia [1/(kg*m^2)] (parameter)
    D,          //drag (parameter)

    OMEGA_L,    //angular velocity of load (input)
    OMEGA_L0,   //target angular velocity of load (parameter)
    KP,         //gain (parameter)
    TAU_MAX,    //engine maximum torque (parameter))

    BETA,       //gas pedal (output)

    NUMBER_OF_REALS
};

enum {
    CLAMP_BETA, //clamp throttle? (parameter)
    NUMBER_OF_BOOLEANS,
};

#define NUMBER_OF_INTEGERS 0
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0
#define FMI_COSIMULATION

#include "fmuTemplate.h"

//returns partial derivative of vr with respect to wrt
static fmi2Status getPartial(state_t *s, fmi2ValueReference vr, fmi2ValueReference wrt, fmi2Real *partial) {
    if (vr == ALPHA && wrt == TAU) {
        *partial = s->r[JINV];
        return fmi2OK;
    }
    return fmi2Error;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {
    //controller. beta = gas pedal
    //fprintf(stderr, "omega_l0 = %f, omega_l = %f\n", s->r[OMEGA_L0], s->r[OMEGA_L]);
    s->r[BETA] = s->r[KP]*(s->r[OMEGA_L0] - s->r[OMEGA_L]);

    if (s->b[CLAMP_BETA]) {
        //clamp beta between zero and one
        if (s->r[BETA] < 0) s->r[BETA] = 0;
        if (s->r[BETA] > 1) s->r[BETA] = 1;
    }

    s->r[ALPHA] =   s->r[JINV] * (s->r[BETA]*s->r[TAU_MAX] + s->r[TAU] - s->r[D]*s->r[OMEGA]);
    //fprintf(stderr, "#################### ALPHA: %f = %f * (%f * %f + %f - %f * %f\n", s->r[ALPHA], s->r[JINV], s->r[BETA], s->r[TAU_MAX], s->r[TAU], s->r[D], s->r[OMEGA]);
    s->r[OMEGA] += s->r[ALPHA] * communicationStepSize;
    s->r[THETA] += s->r[OMEGA] * communicationStepSize;
}

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"
