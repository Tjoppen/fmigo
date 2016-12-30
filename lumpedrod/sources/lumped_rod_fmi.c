#include "lumped_rod.h"
#include <math.h>
#include <assert.h>
#include "modelDescription.h"

#define WRITE_TO_FILE 0

#define SIMULATION_TYPE lumped_rod_sim
//called after getting default values from XML
#define SIMULATION_INIT setStartValues
#define SIMULATION_FREE lumped_rod_sim_free_a
#define SIMULATION_GET lumped_rod_sim_store
#define SIMULATION_SET lumped_rod_sim_restore

#include "fmuTemplate.h"

#if WRITE_TO_FILE
static char filename [] = "lumped_rod.dat";
static FILE * data_file;
#endif

static void lumped_rod_sim_free_a( lumped_rod_sim  sim    ){

  lumped_rod_sim_free( sim ) ;
#if WRITE_TO_FILE
  fclose( data_file);
#endif
  
}
static void lumped_rod_fmi_sync_out( lumped_rod_sim * sim, state_t *s){
  
  s->md.theta1  = sim->coupling_states.x1;
  s->md.omega1  = sim->coupling_states.v1;

  s->md.theta2  = sim->coupling_states.xN;
  s->md.omega2  = sim->coupling_states.vN;

  s->md.alpha1  = sim->coupling_states.a1;
  s->md.alpha2  = sim->coupling_states.aN;

  s->md.dtheta1  = sim->coupling_states.dx1;
  s->md.dtheta2  = sim->coupling_states.dxN;

  s->md.out_torque1  = sim->coupling_states.coupling_f1;
  s->md.out_torque2  = sim->coupling_states.coupling_fN;

  return;
  
}

static void lumped_rod_fmi_sync_in( lumped_rod_sim * sim, state_t *s){
  
  assert( s );
  assert( sim );

  sim->coupling_states.f1         =  s->md.tau1;
  sim->coupling_states.fN         =  s->md.tau2;

  sim->coupling_states.x1         =  s->md.theta_drive1;
  sim->coupling_states.v1         =  s->md.omega_drive1;

  sim->coupling_states.xN         =  s->md.theta_drive2;
  sim->coupling_states.vN         =  s->md.omega_drive2;

  return;

}
 
/**
   Instantiate the simulation and set initial conditions.
*/
static void setStartValues(state_t *s) {
  /** read the init values given by the master, either from command line
      arguments or as defaults from modelDescription.xml
  */
  lumped_rod_sim p = { 
    s->md.step,			/* VR=27 */
    {
      s->md.n_elements, //VR=28
      s->md.J, //VR=16
      s->md.compliance, //VR=17
      s->md.D, //VR=18
    },
    {				/* coupling parameters */
      s->md.K_drive1, //VR=19
      s->md.D_drive1, //VR=20
      s->md.K_drive2, //VR=21
      s->md.D_drive2,  //VR=22
      s->md.driver_sign1, // VR=23
      s->md.driver_sign2, // VR=24
      s->md.integrate_dt1,	/* VR=25 */
      s->md.integrate_dt2,	/* VR=26 */
    }, 
    {				/* coupling states: ouputs */
      0,			/* x */
      0,			/* v */
      0,			/* a */
      0,			/* dx */
      0,			/* x */
      0,			/* v */
      0,			/* a */
      0,			/* dx */
      /* force output*/
      0, 			/* coupling_f1 */
      /* force output*/
      0, 			/* coupling_fN */
      /* force inputs */ 
      s->md.tau1,		/* force in: VR10*/
      s->md.tau2,		/* force in: VR11 */
      /* velocity displacement couplings */
      0, 			/* coupling_x1 */
      0, 			/* coupling_v1 */
      0, 			/* coupling_xN */
      0 			/* coupling_vN */
    }
  };
    
  lumped_rod_init_conditions init = {
    s->md.theta01, //VR=0
    s->md.omega01, //VR=2
    s->md.theta02, //VR=1
    s->md.omega02, //VR=3
  };

  s->simulation = lumped_rod_sim_initialize(p, init );

#if WRITE_TO_FILE
  data_file  = fopen(filename, "w+");
  int i;
  for ( i = 0; i < s->md.n_elements; ++i ){
    fprintf(data_file, " %f ", s->simulation.rod.state.x[ i ] );
  }
  fprintf(data_file, "\n");
#endif
    

};

/** Returns partial derivative of vr with respect to wrt  
 *  We could define a smart convention here.  
 */ 
static fmi2Status getPartial(state_t *s, fmi2ValueReference vr, fmi2ValueReference wrt, fmi2Real *partial) {
  if (vr == VR_ALPHA1 && wrt == VR_TAU1 ) {
    *partial = s->simulation.rod.mobility[ 0 ];
    return fmi2OK;
  }

  if (vr == VR_ALPHA1 && wrt == VR_TAU2 ) {
    *partial = s->simulation.rod.mobility[ 1 ];
    return fmi2OK;
  }

  if (vr == VR_ALPHA2 && wrt == VR_TAU1 ) {
    *partial = s->simulation.rod.mobility[ 2 ];
    return fmi2OK;
  }
    
  if (vr == VR_ALPHA2 && wrt == VR_TAU2 ) {
    *partial = s->simulation.rod.mobility[ 3 ];
    return fmi2OK;
  }

  return fmi2Error;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {
  /*  Copy the input variable from the state vector */
  assert( s );
  lumped_rod_fmi_sync_in(&s->simulation, s);

  assert( s->simulation.step );
  int n = ( int ) ceil( communicationStepSize / s->simulation.step );
  /* Execute the simulation */
  rod_sim_do_step(&s->simulation , n );
  /* Copy state variables to ouputs */
  lumped_rod_fmi_sync_out(&s->simulation, s);
#if WRITE_TO_FILE
  {
    int i;
    for ( i = 0; i < s->md.n_elements; ++i ){
      fprintf(data_file, " %f ", s->simulation.rod.state.x[ i ] );
    }
  }
#endif 
}

// include code that implements the FMI based on the above definitions
#include "../../templates/fmi2/fmuTemplate_impl.h"

