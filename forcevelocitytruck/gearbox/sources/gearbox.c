#include "modelDescription.h"
#include "fmuTemplate.h"

static const double gear_ratios[] = {
   0,
   13.0,
   10.0,
   9.0,
   7.0,
   5.0,
   4.6,
   3.7,
   3.0,
   2.4,
   1.9,
   1.5,
   1.2,
   1.0,
   0.8
};
static const int ngears = sizeof(gear_ratios)/sizeof(gear_ratios[0]);

//returns partial derivative of vr with respect to wrt
static fmi2Status getPartial(state_t *s, fmi2ValueReference vr, fmi2ValueReference wrt, fmi2Real *partial) {
    return fmi2Error;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {
  int gear = s->md.gear;
  if (gear < 0) {
      gear = 0;
  } else if (gear >= ngears) {
      gear = ngears-1;
  }

  double ratio = gear_ratios[ gear ];

  if ( gear != 0 ){
    s->md.theta_l = s->md.theta_e / ratio;
    s->md.omega_l = s->md.omega_e / ratio;
    s->md.tau_e   = -s->md.d1*s->md.omega_e + (-s->md.d2*s->md.omega_l + s->md.tau_l) / ratio;
  }
  else { 
    s->md.theta_l = 0;
    s->md.omega_l = 0;
    s->md.tau_e   = 0;
  }
}

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"
