#define MODEL_IDENTIFIER body
#define MODEL_GUID "{cd4d8666-fd68-4809-b9bb-5b62a55cab84}"
#define HAVE_DIRECTIONAL_DERIVATIVE 0

enum {
    THETA,      //angle (input)
    OMEGA,      //angular velocity (input)
    TAU,        //coupling spring torque (output)
    JINV,       //inverse of moment of inertia [1/(kg*m^2)] (parameter)
    KC,         //Coupling spring constant (parameter)
    DC,         //Coupling spring damping (parameter)
    D,          //drag (parameter)

    //internal state
    OMEGA2,
    THETA2,

    NUMBER_OF_REALS
};

#define NUMBER_OF_INTEGERS 0
#define NUMBER_OF_BOOLEANS 0
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0
#define FMI_COSIMULATION

#include "fmuTemplate.h"

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
    //coupling spring torque
    s->r[TAU] = s->r[KC]*(s->r[THETA2] - s->r[THETA]) + s->r[DC]*(s->r[OMEGA2] - s->r[OMEGA]);

    //angular acceleration = J^-1 * (-spring torque - drag)
    fmi2Real alpha = s->r[JINV] * (-s->r[TAU] - s->r[D]*s->r[OMEGA2]);
    s->r[OMEGA2] +=        alpha * communicationStepSize;
    s->r[THETA2] += s->r[OMEGA2] * communicationStepSize;
}

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"
