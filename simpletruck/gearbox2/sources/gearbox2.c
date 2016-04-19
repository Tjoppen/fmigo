#define MODEL_IDENTIFIER gearbox2
#define MODEL_GUID "{9b727233-c36a-4ede-a3e9-ec1ab2cee17b}"

#define TAU_MAX 135

enum {
    THETA_E,    //engine angle (output, state)
    OMEGA_E,    //engine angular velocity (output, state)
    OMEGADOT_E, //engine angular acceleration (output)
    TAU_E,      //engine torque (input)
    J1,         //moment of inertia of input gear [1/(kg*m^2)] (parameter)
    D1,         //drag of input gear (parameter)

    THETA_L,    //load angle (output)
    OMEGA_L,    //load angular velocity (output)
    OMEGADOT_L, //load angular acceleration (output)
    TAU_L,      //load torque (input)

    ALPHA,       //gear ratio from engine to load (less than one means gear down) (parameter)
    J2,         //moment of inertia of output gear [1/(kg*m^2)] (parameter)
    D2,         //drag of output gear (parameter)

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
    fmi2Real a2 = s->r[ALPHA]*s->r[ALPHA];

    if (vr == OMEGADOT_E && wrt == TAU_E) {
        *partial = a2/(s->r[J2] + a2*s->r[J1]);
        return fmi2OK;
    }
    if ((vr == OMEGADOT_L && wrt == TAU_E) || (vr == OMEGADOT_E && wrt == TAU_L)) {
        *partial = s->r[ALPHA]/(s->r[J2] + a2*s->r[J1]);
        return fmi2OK;
    }
    if (vr == OMEGADOT_L && wrt == TAU_L) {
        *partial =  1/(s->r[J2] + a2*s->r[J1]);   //load side sees a higher moment of inertia (lower inverse)
        return fmi2OK;
    }

    return fmi2Error;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {
    fmi2Real a = s->r[ALPHA], a2 = a*a, h = communicationStepSize;
    fmi2Real A = s->r[TAU_E] - s->r[D1]*s->r[OMEGA_E];
    fmi2Real B = s->r[TAU_L] - s->r[D2]*s->r[OMEGA_L];

    fmi2Real lambda = (-s->r[OMEGA_E] + a*s->r[OMEGA_L] - h*s->r[TAU_E]/s->r[J1] + h*a*s->r[TAU_L]/s->r[J2]) / (s->r[J1] + a2*s->r[J2]);
    
    fmi2Real delta1 = (   lambda + h*A) / s->r[J1];
    fmi2Real delta2 = (-a*lambda + h*B) / s->r[J2];

    s->r[OMEGADOT_E]  = delta1 / h;
    s->r[OMEGA_E]    += delta1;
    s->r[THETA_E]    += s->r[OMEGA_E] * h;

    s->r[OMEGADOT_L]  = delta2 / h;
    s->r[OMEGA_L]    += delta2;
    s->r[THETA_L]    += s->r[OMEGA_L] * h;
}

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"
