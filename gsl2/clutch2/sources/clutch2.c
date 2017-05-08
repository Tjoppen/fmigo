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
} clutchgear_simulation;

#define SIMULATION_TYPE clutchgear_simulation
#define SIMULATION_EXIT_INIT clutch_init
#define SIMULATION_FREE(s) cgsl_free_simulation((s).sim)
#define SIMULATION_GET(s)  cgsl_simulation_get(&(s)->sim);
#define SIMULATION_SET(s)  cgsl_simulation_set(&(s)->sim);

#include "fmuTemplate.h"


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
    dx_e   : angle difference estimate between engine plate and outside coupling
    dx_s   : angle difference estimate between shaft plate and outside coupling

*/

static void compute_forces(state_t *s, const double x[], double *force_e, double *force_s, double *force_clutch) {
  int dx_s_idx = s->md.integrate_dx_e ? 5 : 4;

  /** compute the coupling force: NOTE THE SIGN!
   *  This is the force *applied* to the coupled system
   */
  *force_e =   s->md.gamma_ec * ( x[ 1 ] - s->md.v_in_e );
  if ( s->md.integrate_dx_e )
    *force_e +=  s->md.k_ec *  x[ 4 ];
  else
    *force_e +=  s->md.k_ec *  ( x[ 0 ] - s->md.x_in_e );

  *force_s =   s->md.gamma_sc * ( x[ 3 ] - s->md.v_in_s );
  if ( s->md.integrate_dx_s )
    *force_s +=  s->md.k_sc *  x[ dx_s_idx ];
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
  
  /** the index of dx_s depends on whether we're integrating dx_e or not */
  int dx_s_idx = s->md.integrate_dx_e ? 5 : 4;

  double force_e, force_s, force_clutch;
  compute_forces(s, x, &force_e, &force_s, &force_clutch);

  /** Second order dynamics */
  dxdt[ 0 ]  = x[ 1 ];

  /** internal dynamics */ 
  dxdt[ 1 ]  = -s->md.gamma_e * x[ 1 ];		
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
  dxdt[ 3 ]  = -s->md.gamma_s * x[ 3 ];
  /** coupling */ 
  dxdt[ 3 ] +=  force_clutch;
  /** counter torque from next module */ 
  dxdt[ 3 ] += s->md.force_in_s;
  /** additional driver */ 
  dxdt[ 3 ] += s->md.force_in_sx;
  dxdt[ 3 ] -= force_s;
  dxdt[ 3 ] /= s->md.mass_s;
 
 
  /** angle difference */
  if ( s->md.integrate_dx_e )
    dxdt[ 4 ]        = x[ 1 ] - s->md.v_in_e;
  if ( s->md.integrate_dx_s )
    dxdt[ dx_s_idx ] = x[ 3 ] - s->md.v_in_s;


  return GSL_SUCCESS;

}



/** TODO */
int jac_clutch (double t, const double x[], double *dfdx, double dfdt[], void *params)
{
  
  state_t *s = (state_t*)params;
  int N = 4 + s->md.integrate_dx_e + s->md.integrate_dx_s;
  int i, j;

  gsl_matrix_view dfdx_mat = gsl_matrix_view_array (dfdx, N, N);
  gsl_matrix * J = &dfdx_mat.matrix; 
    
  /* second order dynamics  on first variable */
  for ( i=0; i < N; ++i  )
    for( j = 0; j < N; ++j )
      gsl_matrix_set (J, i, j, 0.0);

  gsl_matrix_set (J, 0, 1, 1.0);

  /* dynamics of the first plate */
  gsl_matrix_set (J, 1, 0, 0.0);
  gsl_matrix_set (J, 1, 1, 0.0);
  gsl_matrix_set (J, 1, 2, 0.0);
  gsl_matrix_set (J, 1, 3, 0.0);
  
   
  /* second order dynamics on second plate*/
  gsl_matrix_set (J, 2, 3, 1.0);  

  /*  dynamics of second plate */
  gsl_matrix_set (J, 3, 0, 0.0);
  gsl_matrix_set (J, 3, 1, 0.0);
  gsl_matrix_set (J, 3, 2, 0.0);
  gsl_matrix_set (J, 3, 3, 0.0); 

  
  i = 4;
  if ( s->md.integrate_dx_e )
    gsl_matrix_set(J, i++, 1, 1.0);
  if ( s->md.integrate_dx_s )
    gsl_matrix_set(J, i++, 3, 1.0);


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
  return 4 + s->md.integrate_dx_e + s->md.integrate_dx_s;
}

static void get_initial_states(state_t *s, double *initials) {
  int N = get_initial_states_size(s);
  initials[0] = s->md.x0_e;
  initials[1] = s->md.v0_e;
  initials[2] = s->md.x0_s;
  initials[3] = s->md.v0_s;
  if (N >= 5) {
    initials[4] = s->md.integrate_dx_e ? s->md.dx0_e : s->md.dx0_s;
  }
  if (N >= 6) {
    initials[5] = s->md.dx0_s;
  }
}

static int sync_out(int n, const double outputs[], void * params) {

  state_t *s = params;
  double dxdt[6];

  //compute accelerations. we need them for Server::computeNumericalJacobian()
  clutch(0, outputs, dxdt, params);

  s->md.x_e = outputs[ 0 ];
  s->md.v_e = outputs[ 1 ];
  s->md.a_e = dxdt[ 1 ];
  s->md.x_s = outputs[ 2 ];
  s->md.v_s = outputs[ 3 ];
  s->md.a_s = dxdt[ 3 ];

  compute_forces(s, outputs, &s->md.force_e, &s->md.force_s, NULL);

  return GSL_SUCCESS;
}


static fmi2Status clutch_init(ModelInstance *comp) {
  state_t *s = &comp->s;
  /** system size and layout depends on which dx's are integrated */
  double initials[6];
  get_initial_states(s, initials);

  if ( s->md.integrator < rk2 || s->md.integrator > msbdf ) {
    fprintf(stderr, "Invalid choice of integrator : %d.  Defaulting to rkf45. \n", s->md.integrator); 
    s->md.integrator = rkf45;
  }
  s->simulation.sim = cgsl_init_simulation(
    cgsl_epce_default_model_init(
      cgsl_model_default_alloc(get_initial_states_size(s), initials, s, clutch, jac_clutch, NULL, NULL, 0),
      s->md.filter_length,
      sync_out,
      s
      ),
    //rkf45, 1e-5, 0, 0, s->md.octave_output, s->md.octave_output ? fopen("clutch2.m", "w") : NULL
    s->md.integrator, 1e-6, 0, 0, s->md.octave_output, s->md.octave_output ? fopen(s->md.octave_output_file, "w") : NULL
    );
  s->simulation.last_gear = s->md.gear;
  s->simulation.delta_phi = 0;
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

  //don't dump tentative steps
  s->simulation.sim.print = noSetFMUStatePriorToCurrentPoint;
  cgsl_step_to( &s->simulation, currentCommunicationPoint, communicationStepSize );

  s->simulation.last_gear = s->md.gear;
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
