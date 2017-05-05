//fix WIN32 build
#include "hypotmath.h"

#include "modelDescription.h"
#include "gsl-interface.h"

#if defined(_WIN32)
#define alloca _alloca
#endif
#define SIMULATION_TYPE cgsl_simulation
#define SIMULATION_EXIT_INIT scania_driveline_init
#define SIMULATION_FREE cgsl_free_simulation

#include "fmuTemplate.h"


#if defined(_WIN32)
#include <malloc.h>
#define alloca _alloca
#else
#include <alloca.h>
#endif

#define SQ(x)  ( x ) * ( x )
#define min(x,y)  ( ( x ) < ( y ) )?  x : y
#define max(x,y)  ( ( x ) > ( y ) )?  x : y

#define inputs (s->md)
#define outputs (s->md)
int  fcn( double t, const double * x, double *dxdt, void * params){

  state_t *s = (state_t*)params;
  /// copy the struct: we don't write to this
  /* ins   inputs  =   ( ( everything * ) params)->inputs; */
  /* /// pointer access needed for outputs */
  /* outs *outputs = & ( ( everything * ) params)->outputs; */

  /// real state is in x
  /// This makes only temporary changes.
  /// TODO: should we remove these variables from the input struct?
  inputs.w_inShaftNeutral = x[0];
  inputs.w_wheel          = x[1];

  /// unused
  ///inputs.w_inShaftOld = x[ 0 ];

  // coupling torque on shaft from input springs or torque
  outputs.f_shaft_out =    inputs.k2 * (  inputs.w_inShaft - inputs.w_shaft_in  );
  double tq_inputShaft =  -outputs.f_shaft_out + inputs.f_shaft_in;

  // coupling force on wheel from input springs or force
  outputs.f_wheel_out = inputs.k1 * ( inputs.w_wheel - inputs.w_wheel_in );
  double tq_inputWheel = -outputs.f_wheel_out +  inputs.f_wheel_in;


  double tq_retWheel = inputs.tq_retarder * inputs.final_gear_ratio;

  // This is the sum of external forces acting on the vehicle:
  // wind + roll + m*g*sin(slope) + brakes, translated to torque at wheel shaft
  // and added friction loss in final gear
  // in other words the torque required at prop shaft to maintain current vehicle speed

  double tq_loadWheelShaft = inputs.tq_brake + inputs.tq_env + tq_retWheel + inputs.tq_fricLoss + tq_inputWheel;

  // external load translated to prop shaft torque
  double tq_loadPropShaft = tq_loadWheelShaft / inputs.final_gear_ratio;

  // the external load is translated to a torque at the input shaft and
  // the mass of the vehicle is translated to an equivalent rotational inertia
  // at transmission input shaft
  double J_atInShaft;
  double tq_loadAtInShaft;

  if ( inputs.gear_ratio != 0 ){
    tq_loadAtInShaft = tq_loadPropShaft / inputs.gear_ratio;
    J_atInShaft = inputs.m_vehicle * SQ ( ( inputs.r_tire /
					    (inputs.final_gear_ratio*inputs.gear_ratio) ) );
  } else {
    // when in neutral the transmission input shaft is disconnected and the
    // speed is then integrated and the shaft inertia is set to J_neutral
    // w_inShaftNeutral is the integration result during  outside
    outputs.w_inShaft = inputs.w_inShaftNeutral;
    tq_loadAtInShaft = 0;
    J_atInShaft = inputs.J_neutral;
  }
  tq_loadAtInShaft += tq_inputShaft;

  // Clutch balance speed
  // if simplifying the engine and the vehicle as two spinning flywheels attached
  // to each plate of the clutch and then closing the clutch, the resulting
  // rotational speed of the clutch w_bal would be the weighted average
  double w_bal = (inputs.J_eng*inputs.w_eng + J_atInShaft*outputs.w_inShaft )/(inputs.J_eng+J_atInShaft);

  // calculate the torque required to accelerate the engine to w_bal in two
  // timesteps


  double tq_loadBal = (inputs.tq_eng * J_atInShaft + tq_loadAtInShaft * inputs.J_eng) / (inputs.J_eng + J_atInShaft);

  double tq_bal =  (inputs.w_eng-w_bal) * inputs.J_eng / (2*inputs.ts);
  double tq_clutchUnLim = tq_bal + tq_loadBal;

  outputs.tq_clutch = min(max(tq_clutchUnLim,-inputs.tq_clutchMax),inputs.tq_clutchMax);

  // transmission losses are given as input shaft torque loss
  double tq_inTransmission = (outputs.tq_clutch - inputs.tq_losses);

  outputs.tq_outTransmission = tq_inTransmission * inputs.gear_ratio;

  double tq_sumWheel = outputs.tq_outTransmission * inputs.final_gear_ratio
    - tq_loadWheelShaft;

  // w_wheel is integrate outside
  outputs.w_wheelDer = tq_sumWheel / ( inputs.m_vehicle * SQ( inputs.r_tire ) );

  outputs.v_vehicle = inputs.w_wheel * inputs.r_tire;

  // slip estimation (r_slipFilt filtered outside this m-function)
  outputs.r_slip = (inputs.tq_env + tq_sumWheel) / ( inputs.m_vehicle * 8 );

  outputs.v_driveWheel = (inputs.r_slipFilt + 1) * outputs.v_vehicle;

  outputs.w_out = outputs.v_driveWheel * inputs.final_gear_ratio / inputs.r_tire;

  if (inputs.gear_ratio == 0){
    // when gear is in neutral the input shaft speed is integrated using the
    // torque coming from the clutch
    outputs.w_inShaftDer = tq_inTransmission / inputs.J_neutral;
  }
  else{
    // When a gear is engaged the transmission input shaft speed is calculated
    // from the output shaft speed scaled with gear ratio, (the result from
    // the inputshaft neutral integration is ignored)
    outputs.w_inShaft = outputs.w_out * inputs.gear_ratio;

    // when not in neutral, set the inputShaft derivative so that the
    // integrator follows the actual speed aproximately
    outputs.w_inShaftDer = 0.5*(outputs.w_inShaft-inputs.w_inShaftNeutral)/inputs.ts;
  }

  dxdt[ 0 ]  = outputs.w_inShaftDer;
  dxdt[ 1 ]  = outputs.w_wheelDer;
  outputs.w_shaft_out = x[ 0 ];
  outputs.w_wheel_out = x[ 1 ];

  return GSL_SUCCESS;
}

static int sync_out(int n, const double out[], void * params) {
  state_t *s = ( state_t * ) params;
  double * dxdt = ( double * ) alloca( ( (size_t) n ) * sizeof(double));

  fcn (0, out, dxdt,  params );

  return GSL_SUCCESS;
}


static void scania_driveline_init(ModelInstance *comp) {
  state_t *s = &comp->s;
  double initials[] = {
    s->md.w_inShaftNeutral,
    s->md.w_wheel
  };

  s->simulation = cgsl_init_simulation(
    cgsl_epce_default_model_init(
      cgsl_model_default_alloc(sizeof(initials)/sizeof(initials[0]), initials, s, fcn, NULL, NULL, NULL, 0),
      0,//s->md.filter_length,
      sync_out,
      s
      ),
    rkf45, 1e-5, 0, 0, 0, NULL
    );
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {
  cgsl_step_to( &s->simulation, currentCommunicationPoint, communicationStepSize );
}

/**
 *   The mobility matrix looks like 
[ J 0 ]
[ 0 m ] 
if the clutch is disconnected. 

When the clutch is connected and the gear ratio is g (including units), 
first have "total mass" 

M = J + g^2 * m

mu = inv(M)

and

     [ g^2  g ]
mu * [ g    1 ]



 */
static fmi2Status getPartial(ModelInstance *comp, fmi2ValueReference vr, fmi2ValueReference wrt, fmi2Real *partial) {
  if (vr == VR_A_SHAFT_OUT ) {
    if (wrt == VR_F_SHAFT_IN ){
      *partial = 0 ; //XXX;
      return fmi2OK;
      }
    if (wrt == VR_F_WHEEL_IN ){
      *partial = 0; // XXX;
      return fmi2OK;
    }
  }
  if (vr == VR_A_WHEEL ) {
    if (wrt == VR_F_SHAFT_IN ){
      *partial = 0;//XXX;
      return fmi2OK;
    }
    if (wrt == VR_F_WHEEL_IN ){
      *partial = 0;//XXX;
      return fmi2OK;
    }
  }
  return fmi2Error;
}

#ifdef CONSOLE
  int main(){

    state_t s;
    s.md = defaults;
    scania_driveline_init(&s);
    s.simulation.file = fopen( "s.m", "w+" );
    s.simulation.save = 1;
    s.simulation.print = 1;
    cgsl_step_to( &s.simulation, 0.0, 40 );
    cgsl_free_simulation(s.simulation);

    return 0;
  }
#else

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"

#endif
