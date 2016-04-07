#include "lumped_rod.h"
#include "fmuTemplate.h"



#define MODEL_IDENTIFIER lumped_rod
#define MODEL_GUID "{b8998512-96a7-4e6d-8350-6d1f9aeae4a1}"

enum {
  THETA1,      //angle (output, state)
  THETA2,      //angle (output, state)
  OMEGA1,      //angular velocity (output, state)
  OMEGA2,      //angular velocity (output, state)
  ALPHA1,      //angular acceleration (output)
  ALPHA2,      //angular acceleration (output)
  TAU1,        //coupling torque (input)
  TAU2,        //coupling torque (input)
  J0,          // moment of inertia [1/(kg*m^2)] (parameter)
  COMP,           // compliance of the rod
  DAMP,          //drag (parameter)
  STEP,          // *internal* time step
    NUMBER_OF_REALS
};

enum {
  NELEM,                            // number of elements
  NUMBER_OF_INTEGERS
};


#define NUMBER_OF_BOOLEANS 0
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0
#define FMI_COSIMULATION

#include "fmuTemplate.h"


static void lumped_rod_fmi_sync_out( lumped_rod_sim * sim, state_t *s){
  
  s->r[ THETA1 ]  = sim->state.x1;
  s->r[ THETA2 ]  = sim->state.xN;
  s->r[ OMEGA1 ]  = sim->state.v1;
  s->r[ OMEGA2 ]  = sim->state.vN;
  s->r[ ALPHA1 ]  = sim->state.a1;
  s->r[ ALPHA2 ]  = sim->state.aN;
  s->r[ TAU1 ]    = sim->state.f1;
  s->r[ TAU2 ]    = sim->state.fN;

  return;
  
}

static void lumped_rod_fmi_sync_in( lumped_rod_sim * sim, state_t *s){
  
  sim->forces[ 0 ] =  s->r[ TAU1 ];
  sim->forces[ 1 ] =  s->r[ TAU2 ];

  return;

}
 
static void setStartValues(state_t *s) {

  lumped_rod_sim_parameters p = {
    s->r[ J0 ], 
    s->i[ NELEM ],
    s->r[ COMP ],
    s->r[ STEP ],
    s->r[ DAMP ],
    s->r[ THETA1 ],
    s->r[ THETA2 ],
    s->r[ OMEGA1 ],
    s->r[ OMEGA2 ],
    s->r[ TAU1 ],
    s->r[ TAU2 ]
  };

  lumped_rod_sim * sim =  lumped_rod_sim_create( p );
  s->simulation = (void * ) sim; 

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

  lumped_rod_sim * sim  = ( lumped_rod_sim  *) s->simulation;

    if (vr == ALPHA1 && wrt == TAU1 ) {
      *partial = sim->mobility[ 0 ];
        return fmi2OK;
    }

    if (vr == ALPHA1 && wrt == TAU2 ) {
      *partial = sim->mobility[ 1 ];
        return fmi2OK;
    }

    if (vr == ALPHA2 && wrt == TAU1 ) {
      *partial = sim->mobility[ 2 ];
        return fmi2OK;
    }
    
    if (vr == ALPHA2 && wrt == TAU2 ) {
      *partial = sim->mobility[ 3 ];
        return fmi2OK;
    }

    return fmi2Error;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {
  
  lumped_rod_sim * sim  = ( lumped_rod_sim  *) s->simulation;
   
  lumped_rod_fmi_sync_in(sim, s);

  int n = ( int ) ceil( communicationStepSize / sim->step );
  
  step_rod_sim( sim , n );
  
  lumped_rod_fmi_sync_out( sim, s);
  
}

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"

