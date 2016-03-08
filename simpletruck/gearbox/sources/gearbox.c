#define MODEL_IDENTIFIER gearbox
#define MODEL_GUID "{3281ad5b-a414-4117-b607-b16601466e80}"

enum {
    THETA_E,    //engine angle (output, state)
    OMEGA_E,    //engine angular velocity (output, state)
    OMEGADOT_E, //engine angular acceleration (output)
    TAU_E,      //engine torque (input)
    JINV,       //inverse of moment of inertia [1/(kg*m^2)] (parameter)
    D,          //drag (parameter)

    THETA_L,    //load angle (output)
    OMEGA_L,    //load angular velocity (output)
    OMEGADOT_L, //load angular acceleration (output)
    TAU_L,      //load torque (input)

    ALPHA,       //gear ratio from engine to load (less than one means gear down) (parameter)

    NUMBER_OF_REALS
};

#define NUMBER_OF_INTEGERS 0
#define NUMBER_OF_BOOLEANS 0
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0
#define FMI_COSIMULATION

#include "fmuTemplate.h"

static void setStartValues(state_t *s) {
    int x;
    for (x = 0; x < NUMBER_OF_REALS; x++) {
        s->r[x] = 0;
    }

    s->r[JINV]      = 1/ 4.0;
    s->r[ALPHA]     = 10.0;
    s->r[D]         = 1;            //drag [Nm / (radians/s)]
}

// called by fmiExitInitializationMode() after setting eventInfo to defaults
// Used to set the first time event, if any.
static void initialize(state_t *s, fmi2EventInfo* eventInfo) {
}

// called by fmiGetReal, fmiGetContinuousStates and fmiGetDerivatives
static fmi2Real getReal(state_t *s, fmi2ValueReference vr){
    switch (vr) {
    default:        return s->r[vr];
    }
}

//returns partial derivative of vr with respect to wrt
static fmi2Status getPartial(state_t *s, fmi2ValueReference vr, fmi2ValueReference wrt, fmi2Real *partial) {
    if (vr == OMEGADOT_E && wrt == TAU_E) {
        *partial = s->r[JINV];
        return fmi2OK;
    }
    if (vr == OMEGADOT_L && wrt == TAU_L) {
        *partial = s->r[JINV]/s->r[ALPHA];   //load side sees a higher moment of inertia (lower inverse)
        return fmi2OK;
    }

    return fmi2Error;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {
    s->r[OMEGADOT_E]  = s->r[JINV] * (s->r[TAU_E] + s->r[TAU_L]/s->r[ALPHA] - s->r[D]*s->r[OMEGA_E]);
    s->r[OMEGA_E]    += s->r[OMEGADOT_E] * communicationStepSize;
    s->r[THETA_E]    += s->r[OMEGA_E] * communicationStepSize;

    //load shaft is at fixed ratio to engine shaft
    s->r[OMEGADOT_L] = s->r[OMEGADOT_E] / s->r[ALPHA];
    s->r[OMEGA_L]    = s->r[OMEGA_E]    / s->r[ALPHA];
    s->r[THETA_L]    = s->r[THETA_E]    / s->r[ALPHA];
}

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"
