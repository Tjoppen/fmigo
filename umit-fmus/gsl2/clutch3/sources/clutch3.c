//TODO: add some kind of flag that switches this one between a clutch and a gearbox, to reduce the amount of code needed

//fix WIN32 build
#include "hypotmath.h"

#include "modelDescription.h"
#include "gsl-interface.h"
#include <memory.h>

typedef struct {
  cgsl_simulation sim;
  int last_gear;      /** for detecting when the gear changes */
  double delta_phi;   /** angle difference at gear change, for preventing
                       *  "springing" when changing gears */
/* this is to help with computation of dphi */
  double communication_time;
  double xe0;			/* angles from last time */
  double xs0;
  double xe00;			/* angles from last time */
  double xs00;
  double dxe0;			/* angle diffs from last time */
  double dxs0;			
  double dxe00;			/* angle diffs from last time */
  double dxs00;			
  double zdxe;			/* filtered dx from last time */
  double zdxs;
} clutchgear_simulation;

#define SIMULATION_TYPE clutchgear_simulation
#define SIMULATION_EXIT_INIT clutch_init
#define SIMULATION_FREE(s) cgsl_free_simulation((s).sim)
#define SIMULATION_GET(s)  cgsl_simulation_get(&(s)->sim);
#define SIMULATION_SET(s)  cgsl_simulation_set(&(s)->sim);

#include "fmuTemplate.h"

#if 0 
// desperate debugging
static FILE * outfile = (FILE * ) NULL;
#endif

static const double clutch_dphi  [] = { -0.087266462599716474, -0.052359877559829883, 0.0, 0.09599310885968812, 0.17453292519943295 };
static const double clutch_torque[] = { -1000, -30, 0, 50, 3500 };
static const size_t N_segments      = sizeof( clutch_torque ) / sizeof( clutch_torque[ 0 ] );

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

static double gear2ratio(state_t *s) {
  if (s->md.gear < 0) {
    return -gear_ratios[1];
  } else {
    int i = s->md.gear;
    int n = sizeof(gear_ratios)/sizeof(gear_ratios[0]);
    if (i >= n) i = n-1;
    return gear_ratios[i];
  }
}

/**
 *  The force function from the clutch
 */
static double fclutch( double dphi, double domega, double clutch_damping ); 
static double fclutch_dphi_derivative( double dphi );

static double get_dx(double phi, double phi0, double dphi0, double omega_in, double t ){
  double dx = phi - phi0 - t * omega_in + dphi0;
//  fprintf(stderr, "dx = %f phi = %f  phi0 = %f dphi0 = %f omega_in = %f  t = %f\n", dx, phi, phi0, dphi0, omega_in, t);
  return dx;

}
/*  
    Two (rotational) bodies connected via a piecewise linear clutch model. 
    
    Each body is coupled via force or velocity (or even position) with the
    outside.  

    Additional external forces are provided for each of the bodies. 

    For the case of velocity coupling, the angle difference is integrated. 

    Variables are listed as: 
    x_e    : position engine plate
    v_e    : velocity engine plate
    x_s    : position shaft plate
    v_s    : velocity shaft plate

*/

static void compute_forces(state_t *s, const double x[],
			   double dxe,
			   double dxs,
			   double *force_e, double *force_s,
			   double *force_clutch, double t) {


/** compute the coupling force: NOTE THE SIGN!
 *  This is the force *applied* to the coupled system
 */
  
  *force_e =   s->md.gamma_ec * ( x[ 1 ] - s->md.v_in_e );

  
  if ( s->md.integrate_dx_e )
    *force_e +=  s->md.k_ec *  dxe;
  else
    *force_e +=  s->md.k_ec *  ( x[ 0 ] - s->md.x_in_e );

  *force_s =   s->md.gamma_sc * ( x[ 3 ] - s->md.v_in_s );
  if ( s->md.integrate_dx_s )
    *force_s +=  s->md.k_sc *  dxs;
  else
    *force_s +=  s->md.k_sc *  ( x[ 2 ] - s->md.x_in_s );

  if (force_clutch) {
    if (s->md.is_gearbox) {
      if (s->md.gear == 0) {
// neutral
	*force_clutch = 0;
      } else {
	double ratio = gear2ratio(s);
	*force_clutch = s->md.gear_k * (x[ 0 ] - ratio*x[ 2 ] + s->simulation.delta_phi) + s->md.gear_d * (x[ 1 ] - ratio*x[ 3 ]);
      }
    } else {
      *force_clutch =  s->md.clutch_position * fclutch( x[ 0 ] - x[ 2 ], x[ 1 ] - x[ 3 ], s->md.clutch_damping );
    }
  }
}


int clutch (double t, const double x[], double dxdt[], void * params){

  state_t *s = (state_t*)params;
  
  double force_e, force_s, force_clutch;
  double t0 = s->simulation.communication_time;
  double dxe = get_dx( x[ 0 ],  s->md.x_e, s->simulation.dxe0, s->md.v_in_e, t-t0);
  double dxs = get_dx( x[ 2 ],  s->md.x_s, s->simulation.dxs0, s->md.v_in_s, t-t0);
  compute_forces(s, x, dxe, dxs, &force_e, &force_s, &force_clutch, t);

/** Second order dynamics */
  dxdt[ 0 ]  = x[ 1 ];

  double damping_e = -s->md.gamma_e * x[ 1 ];		
  double damping_s = -s->md.gamma_s * x[ 3 ];		
/** internal dynamics */ 
  dxdt[ 1 ]  = damping_e;
/** coupling */ 
  dxdt[ 1 ] += -force_clutch;
/** counter torque from next module */ 
  dxdt[ 1 ] += s->md.force_in_e;
/** additional driver */ 
  dxdt[ 1 ] += s->md.force_in_ex;
  dxdt[ 1 ] -= force_e;

  dxdt[ 1 ] /= s->md.mass_e;
 
/** shaft-side plate */

  dxdt[ 2 ]  = x[ 3 ];
  
/** internal dynamics */ 
  dxdt[ 3 ]  = damping_s;
/** coupling */ 
  dxdt[ 3 ] +=  force_clutch;
/** counter torque from next module */ 
  dxdt[ 3 ] += s->md.force_in_s;
/** additional driver */ 
  dxdt[ 3 ] += s->md.force_in_sx;
  dxdt[ 3 ] -= force_s;
  dxdt[ 3 ] /= s->md.mass_s;
 
  return GSL_SUCCESS;

}



/** TODO */
int jac_clutch (double t, const double x[], double *dfdx, double dfdt[], void *params)
{
  
  state_t *s = (state_t*)params;
  int N = 4;
  double r = gear2ratio(s);
  int i, j;
  double dphid = fclutch_dphi_derivative( x[0]-x[2] ) ;
  double tmp;
  double mu_e = 1.0 / s->md.mass_e;
  double mu_s = 1.0 / s->md.mass_s;

  gsl_matrix_view dfdx_mat = gsl_matrix_view_array (dfdx, N, N);
  gsl_matrix * J = &dfdx_mat.matrix; 
    
  for ( i=0; i < N; ++i  )
    for( j = 0; j < N; ++j )
      gsl_matrix_set (J, i, j, 0.0);

  /* second order dynamics on the angles */
  gsl_matrix_set (J, 0, 1, 1.0);
  gsl_matrix_set (J, 2, 3, 1.0);
  
  gsl_matrix_set(J, 1, 0, -s->md.k_ec * mu_e);

  gsl_matrix_set(J, 3, 2, -s->md.k_sc * mu_s);
  
  /* dynamics of the first plate */
  if ( s->md.is_gearbox) {
    
    gsl_matrix_set (J, 1, 1, -( s->md.gear_d + s->md.gamma_ec + s->md.gamma_e ) * mu_e);
    gsl_matrix_set (J, 1, 2, s->md.gear_k * mu_e); /* spring to phi2 */
    gsl_matrix_set (J, 1, 3, s->md.gear_d * mu_e); /* damping with w2 */

    gsl_matrix_set (J, 3, 3, -( r * s->md.gear_d + s->md.gamma_sc + s->md.gamma_s ) * mu_s);
    gsl_matrix_set (J, 3, 0, r * s->md.gear_k * mu_s); /* spring to phi1 */
    gsl_matrix_set (J, 3, 1, r * s->md.gear_d * mu_s); /* damping with w1 */
   
    
  } else{
    tmp = -dphid;
    gsl_matrix_set (J, 1, 0, tmp * mu_e  ); 

    gsl_matrix_set (J, 1, 1, ( -s->md.clutch_damping - s->md.gamma_ec - s->md.gamma_e ) *mu_e );
    gsl_matrix_set (J, 1, 2, dphid); /* spring to phi2 */
    gsl_matrix_set (J, 1, 3, s->md.clutch_damping * mu_e ); /* damping with w2 */

    gsl_matrix_set (J, 3, 3,(-s->md.clutch_damping - s->md.gamma_sc - s->md.gamma_s ) * mu_s);
    gsl_matrix_set (J, 3, 0, dphid * mu_s); /* spring to phi1 */
    gsl_matrix_set (J, 3, 1, s->md.clutch_damping *mu_s); /* damping with w1 */

    tmp =  -dphid;
    gsl_matrix_set (J, 3, 2, tmp  ); 
  }

  return GSL_SUCCESS;
}

/**
 *  Parameters should be read from a file but that's quicker to setup.
 *  These numbers were provided by Scania.
 */
static double fclutch( double dphi, double domega, double clutch_damping ) {
  
  const size_t N  = N_segments;
  const size_t END = N-1;
  double tc = clutch_torque[ 0 ]; //clutch torque
    
  /** look up internal torque based on dphi
      if too low (< -b[ 0 ]) then c[ 0 ]
      if too high (> b[ 4 ]) then c[ 4 ]
      else lerp between two values in c
  */


  if (dphi <= clutch_dphi[ 0 ]) {
    tc = ( clutch_torque[ 1 ] - clutch_torque[ 0 ] ) * (dphi - clutch_dphi[ 0 ]) / ( clutch_dphi[ 0 ] - clutch_dphi[ 1 ] ) + clutch_torque[ 0 ];
  } else if ( dphi >= clutch_dphi[ END ] ) {
    tc = ( clutch_torque[ END ] - clutch_torque[ END - 1 ] ) * ( dphi - clutch_dphi[ END ] ) / ( clutch_dphi[ END ] - clutch_dphi[ END - 1 ] ) + clutch_torque[ END ];
  } else {
    size_t i;
    for (i = 0; i < END; ++i) {
      if (dphi >= clutch_dphi[ i ] && dphi <= clutch_dphi[ i+1 ]) {
	double k = (dphi - clutch_dphi[ i ]) / (clutch_dphi[ i+1 ] - clutch_dphi[ i ]);
	tc = (1-k) * clutch_torque[ i ] + k * clutch_torque[ i+1 ];
	break;
      }
    }
    if (i >= END ) {
      //too high (shouldn't happen)
      tc = clutch_torque[ END ];
    }
  }

  //add damping. 
  tc += clutch_damping * domega;

  return tc; 

}

static double fclutch_dphi_derivative( double dphi ) {
  
  const size_t N  = N_segments;
  const size_t END = N-1;
  double tc = clutch_torque[ 0 ]; //clutch torque

  double df = 0;		// clutch derivative

  if (dphi <= clutch_dphi[ 0 ]) {
    df =  ( clutch_torque[ 1 ] - clutch_torque[ 0 ] ) / ( clutch_dphi[ 1 ] - clutch_dphi[ 0 ] ); 
  } else if ( dphi >= clutch_dphi[ END ] ) {
    df =  ( clutch_torque[ END ] - clutch_torque[ END - 1 ] ) / ( clutch_dphi[ END ] - clutch_dphi[ END - 1 ] );
  } else {
    size_t i;
    for (i = 0; i < END; ++i) {
      if (dphi >= clutch_dphi[ i ] && dphi <= clutch_dphi[ i+1 ]) {
	double k =  1.0  / (clutch_dphi[ i+1 ] - clutch_dphi[ i ]);
	df = k * ( clutch_torque[ i + 1 ] -  clutch_torque[ i ] );
	break;
      }
    }
  }

  return df; 

}

#define HAVE_INITIALIZATION_MODE
static int get_initial_states_size(state_t *s) {
  return 4;
}

/// WTF?  This is called 25 times during initialization!
static void get_initial_states(state_t *s, double *initials) {
  initials[0] = s->md.x0_e;
  initials[1] = s->md.v0_e;
  initials[2] = s->md.x0_s;
  initials[3] = s->md.v0_s;
}

/**
   TODO: 
   In case of filtering, we have to compute the filtered values of dx. 
   This means that we need to keep the previous value in the simulation struct. 

   The correct formula for dx is 
   <dx>  = <x> - x0 - 0.5 * H * w_in  + dx0

   We get the 1/2 factor because 
   dx(s)  = x(s) - x0 - s * w_in  + dx0
   
   and this is then integrated

 */
static int sync_out(double t, double dt, int n, const double outputs[], void * params) {

  state_t *s = (state_t *) params;
  double dxdt[4];
  int x;

  s->simulation.xe00 =  outputs[ 0 ];
  s->simulation.xs00 =  outputs[ 2 ];
  s->simulation.dxe00 =  get_dx( s->simulation.xe0, s->md.x_e, s->simulation.dxe0, s->md.v_in_e, t - s->simulation.communication_time);
  s->simulation.dxs00 =  get_dx( s->simulation.xs0, s->md.x_s, s->simulation.dxs0, s->md.v_in_s, t - s->simulation.communication_time); 

//compute accelerations. we need them for Server::computeNumericalJacobian()

  double t0   = s->simulation.communication_time;
  double dxe  = get_dx( outputs[ 0 ],  s->md.x_e, s->simulation.dxe0, s->md.v_in_e, t-t0);
  double dxs  = get_dx( outputs[ 2 ],  s->md.x_s, s->simulation.dxs0, s->md.v_in_s, t-t0);

  clutch(0, outputs, dxdt, params);
  /*if ( s->md.filter_length == 0 )
    compute_forces(s, outputs,  dxe, dxs, &s->md.force_e, &s->md.force_s, NULL, t);
  else {*/
    double zdxe = get_dx( 0,  s->md.x_e, s->simulation.dxe0, s->md.v_in_e, t-t0);
    double zdxs = get_dx( 0,  s->md.x_s, s->simulation.dxs0, s->md.v_in_s, t-t0);

    compute_forces(s, outputs,  outputs[0] + 0.5 * ( zdxe + s->simulation.zdxe ) ,
		   outputs[ 2 ] + 0.5 * ( zdxs + s->simulation.zdxs ) ,
		   &s->md.force_e, &s->md.force_s, NULL, t);

    s->simulation.zdxe = zdxe;
    s->simulation.zdxs = zdxs;
  //}

  s->simulation.dxe0 =  s->simulation.dxe00;
  s->simulation.dxs0 =  s->simulation.dxs00;
  s->simulation.xe0  =  s->simulation.xe00;
  s->simulation.xs0  =  s->simulation.xs00;

  s->md.x_e = outputs[ 0 ];
  s->md.v_e = outputs[ 1 ];
  s->md.a_e = dxdt[ 1 ];
  s->md.x_s = outputs[ 2 ];
  s->md.v_s = outputs[ 3 ];
  s->md.a_s = dxdt[ 2 ];

  s->md.n_steps = s->simulation.sim.iterations;

  if ( ! s->md.reset_dx_e ) { 
    s->simulation.dxe0 =  get_dx( s->simulation.xe0, s->md.x_e, s->simulation.dxe0, s->md.v_in_e,
				 t - s->simulation.communication_time);
  }
  if ( ! s->md.reset_dx_s ) { 
    s->simulation.dxs0 =  get_dx( s->simulation.xs0, s->md.x_s, s->simulation.dxs0, s->md.v_in_s, t - s->simulation.communication_time);
  }


  s->simulation.communication_time = t;

  return GSL_SUCCESS;

}

static fmi2Status clutch_init(ModelInstance *comp) {
  state_t *s = &comp->s;
  /** system size and layout depends on which dx's are integrated */
  double initials[4];
#if 0 
  // desperate debugging
  if ( outfile == (FILE * ) NULL ) {
    outfile = fopen("cripes.m", "w");
  }
#endif
  get_initial_states(s, initials);
  if ( s->md.integrator < rk2 || s->md.integrator > msbdf ) {
    fprintf(stderr, "Invalid choice of integrator : %d.  Defaulting to rkf45. \n", s->md.integrator); 
    s->md.integrator = rkf45;
  }

  s->simulation.sim = cgsl_init_simulation(
    cgsl_model_default_alloc(sizeof(initials)/sizeof(initials[0]), initials, s, clutch, jac_clutch, NULL, sync_out, 0),
    s->md.integrator, 1e-6, 0, 0, s->md.octave_output, s->md.octave_output ? fopen(s->md.octave_output_file, "w") : NULL
  );

  s->simulation.last_gear = s->md.gear;
  s->simulation.delta_phi = 0;
  s->simulation.xe0 = 0;
  s->simulation.xs0 = 0;
  s->simulation.dxe0 = 0;
  s->simulation.dxs0 = 0;
  s->simulation.zdxe = 0;
  s->simulation.zdxs = 0;
  s->simulation.communication_time = 0;

  return fmi2OK;
}

static fmi2Status getPartial(ModelInstance *comp, fmi2ValueReference vr, fmi2ValueReference wrt, fmi2Real *partial) {
  if (vr == VR_A_E) {
    if (wrt == VR_FORCE_IN_E || wrt == VR_FORCE_IN_EX) {
      *partial = 1.0/comp->s.md.mass_e;
      return fmi2OK;
    }
    if (wrt == VR_FORCE_IN_S || wrt == VR_FORCE_IN_SX) {
      *partial = 0;
      return fmi2OK;
    }
  }
  if (vr == VR_A_S) {
    if (wrt == VR_FORCE_IN_E || wrt == VR_FORCE_IN_EX) {
      *partial = 0;
      return fmi2OK;
    }
    if (wrt == VR_FORCE_IN_S || wrt == VR_FORCE_IN_SX) {
      *partial = 1.0/comp->s.md.mass_s;
      return fmi2OK;
    }
  }
  return fmi2Error;
}



static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {

  if (s->md.is_gearbox && s->md.gear != s->simulation.last_gear) {
    /** gear changed - compute impact that keeps things sane */
    double ratio = gear2ratio(s);
    s->simulation.delta_phi = ratio*s->simulation.sim.model->x[ 2 ] - s->simulation.sim.model->x[ 0 ];
  }
  /* this is to integrate angle differences */
  s->simulation.communication_time = currentCommunicationPoint; 

  //don't dump tentative steps
  s->simulation.sim.print = noSetFMUStatePriorToCurrentPoint;
  cgsl_step_to( &s->simulation, currentCommunicationPoint, communicationStepSize );


  return;
}


//gcc -g clutch.c ../../../templates/gsl2/gsl-interface.c -DCONSOLE -I../../../templates/gsl2 -I../../../templates/fmi2 -lgsl -lgslcblas -lm -Wall
#ifdef CONSOLE
int main(){

//  FILE * f = fopen("data.m", "w+");

  state_t s;
  
  s.md = defaults;
  s.md.v0_e = 50;
  s.md.clutch_damping = 0;
  s.md.gamma_e = 0.0;
  s.md.gamma_s = 0.0;
  s.md.mass_e = 1.0;
  s.md.mass_s = 100.0;
//  s.md.force_in_e = 20;
  // s.md.force_in_s = -20;

  clutch_init(&s);
  s.simulation.sim.file = fopen( "s.m", "w+" );
  s.simulation.sim.save = 1;
  s.simulation.sim.print = 1;
  cgsl_step_to( &s.simulation.sim, 0.0, 100.0 );
  cgsl_free_simulation(s.simulation.sim);

  return 0;
}
#else

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"

#endif
