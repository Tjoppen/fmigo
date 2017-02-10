/* ---------------------------------------------------------------------------*
 * Sample implementation of an FMU - a bouncing ball.
 * This demonstrates the use of state events and reinit of states.
 * Equations:
 *  der(h) = v;
 *  der(v) = -g;
 *  when h<0 then v := -e * v;
 *  where
 *    h      height [m], used as state, start = 1
 *    v      velocity of ball [m/s], used as state
 *    der(h) velocity of ball [m/s]
 *    der(v) acceleration of ball [m/s2]
 *    g      acceleration of gravity [m/s2], a parameter, start = 9.81
 *    e      a dimensionless parameter, start = 0.7
 *
 * Copyright QTronic GmbH. All rights reserved.
 * ---------------------------------------------------------------------------*/

// include code that implements the FMI based on the above definitions
#include "modelDescription.h"
#include "gsl-interface.h"

#define SIMULATION_TYPE cgsl_simulation
#define SIMULATION_INIT springs_init
#define SIMULATION_FREE cgsl_free_simulation

#include "fmuTemplate.h"
// define class name and unique id
#define MODEL_IDENTIFIER bouncingBall
#define MODEL_GUID "{8c4e810f-3df3-4a00-8276-176fa3c9f003}"

// define initial state vector as vector of value references
static void update_all(modelDescription_t *md){
    md->der_h = md->v;
    md->der_v = -md->g;
}

// offset for event indicator, adds hysteresis and prevents z=0 at restart
#define EPS_INDICATORS 1e-14
fmi2Real getEventIndicator(const modelDescription_t* md, fmi2Real eventIndicators[]) {
    eventIndicators[0] = md->h + (md->v>0 ? EPS_INDICATORS : -EPS_INDICATORS);
    return fmi2OK;
}

// used to set the next time event, if any.
static void eventUpdate(ModelInstance *comp, fmi2EventInfo *eventInfo) {
    modelDescription_t* md = &comp->s.md;
    if (md->h<0 && md->v<0) {
        md->v = - md->e * md->v;
    }
    eventInfo->valuesOfContinuousStatesChanged   = fmi2True;
    eventInfo->nominalsOfContinuousStatesChanged = fmi2False;
    eventInfo->terminateSimulation   = fmi2False;
    eventInfo->nextEventTimeDefined  = fmi2False;
}



























// used to set the next time event, if any.

static void SIMULATION_INIT(state_t *s) {
    return;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {
    cgsl_step_to( &s->simulation, currentCommunicationPoint, communicationStepSize );
}

//gcc -g springs.c ../../../templates/gsl2/gsl-interface.c -DCONSOLE -I../../../templates/gsl2 -I../../../templates/fmi2 -lgsl -lgslcblas -lm -Wall
#ifdef CONSOLE
int main(){

  return 0;
}
#else

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"

#endif
