#define MODEL_IDENTIFIER gearbox
#define MODEL_GUID "{16176ce9-5941-49b2-9f50-b6870dd10c46}"

#define TAU_MAX 135

enum {
    THETA_E,    //engine angle (output, state)
    OMEGA_E,    //engine angular velocity (output, state)
    TAU_E,      //engine torque (input)
    D1,         //drag of input gear (parameter)

    THETA_L,    //load angle (output)
    OMEGA_L,    //load angular velocity (output)
    TAU_L,      //load torque (input)
    D2,         //drag of output gear (parameter)

    ALPHA,       //gear ratio from engine to load (less than one means gear down) (parameter)

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
    return fmi2Error;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {
    s->r[THETA_L] = s->r[THETA_E] / s->r[ALPHA];
    s->r[OMEGA_L] = s->r[OMEGA_E] / s->r[ALPHA];
    s->r[TAU_E]   = -s->r[D1]*s->r[OMEGA_E] + (-s->r[D2]*s->r[OMEGA_L] + s->r[TAU_L]) / s->r[ALPHA];
}

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"
