#include "modelDescription.h"
#include "gsl-interface.h"

#define SIMULATION_TYPE cgsl_simulation
#define SIMULATION_INIT clutch_init
#define SIMULATION_FREE cgsl_free_simulation

#include "fmuTemplate.h"

/**
 *  The force function from the clutch
 */
static double fclutch( double dphi, double domega, double clutch_damping ); 
static double fclutch_dphi_derivative( double dphi );

const static double flip = -1;

/*  
    Two (rotational) bodies connected via a piecewise linear clutch model. 
    
    Each body is coupled via force or velocity (or even position) with the
    outside.  

    Additional external forces are provided for each of the bodies. 

    For the case of velocity coupling, the angle difference is integrated. 

    Variables are listed as: 
    x_e    : position engine plate
    v_e    : velocity engine plate
    dx_e   : angle difference estimate between engine plate and outside coupling

    x_s    : position shaft plate
    v_s    : velocity shaft plate
    dx_s   : angle difference estimate between shaft plate and outside coupling

*/

int clutch (double t, const double x[], double dxdt[], void * params){

  state_t *s = (state_t*)params;

  double force_clutch =  s->md.clutch_position * fclutch( x[ 0 ] - x[ 3 ], x[ 1 ] - x[ 4 ], s->md.clutch_damping );

  /** Second order dynamics */
  dxdt[ 0 ]  = x[ 1 ];

  /** compute the coupling force: NOTE THE SIGN!
   *  This is the force *applied* to the coupled system
   */
  s->md.force_e =   s->md.gamma_ec * ( x[ 1 ] - s->md.v_in_e );
  if ( s->md.integrate_dx_e )
    s->md.force_e +=  s->md.k_ec *  x[ 2 ];
  else
    s->md.force_e +=  s->md.k_ec *  ( x[ 2 ] - s->md.x_in_e );
										   

  /** internal dynamics */ 
  dxdt[ 1 ]  = -s->md.gamma_e * x[ 1 ];		
  /** coupling */ 
  dxdt[ 1 ] += -force_clutch;
  /** counter torque from next module */ 
  dxdt[ 1 ] += -s->md.force_in_e;
  /** additional driver */ 
  dxdt[ 1 ] += s->md.force_in_ex;
  dxdt[ 1 ] -= force_e;
  dxdt[ 1 ] /= s->md.mass_e;
 
  /** angle difference */
  if ( s->md.integrate_dx_e )
    dxdt[ 2 ] = x[ 1 ] - s->md.v_in_e;


  
  /** shaft-side plate */

  dxdt[ 3 ]  = x[ 4 ];
  /** compute the coupling force: NOTE THE SIGN!
   *  This is the force *applied* to the coupled system
   */
  s->md.force_s =   s->md.gamma_sc * ( x[ 4 ] - s->md.v_in_s );
  if ( s->md.integrate_dx_s )
    s->md.force_s +=  s->md.k_sc *  x[ 5 ];
  else
    s->md.force_e +=  s->md.k_sc *  ( x[ 3 ] - s->md.x_in_s );
  
  /** internal dynamics */ 
  dxdt[ 4 ]  = -s->md.gamma_s * x[ 4 ];		
  /** coupling */ 
  dxdt[ 4 ] +=  force_clutch;
  /** counter torque from next module */ 
  dxdt[ 4 ] += s->md.force_in_s;
  /** additional driver */ 
  dxdt[ 4 ] += s->md.force_in_sx;
  dxdt[ 4 ] -= s->md.force_s;
  dxdt[ 4 ] /= s->md.mass_s;
 
 
  /** angle difference */
  if ( s->md.integrate_dx_s )
    dxdt[ 5 ] = x[ 4 ] - s->md.v_in_s;

  return GSL_SUCCESS;

}



/** TODO */
int jac_clutch (double t, const double x[], double *dfdx, double dfdt[], void *params)
{
  
  state_t *s = (state_t*)params;
  gsl_matrix_view dfdx_mat = gsl_matrix_view_array (dfdx, 3, 3);
  gsl_matrix * J = &dfdx_mat.matrix; 


  return GSL_SUCCESS;
}

/**
 *  Parameters should be read from a file but that's quicker to setup.
 *  These numbers were provided by Scania.
 */
static double fclutch( double dphi, double domega, double clutch_damping ) {
  
  //Scania's clutch curve
  static const double b[] = { -0.087266462599716474, -0.052359877559829883, 0.0, 0.09599310885968812, 0.17453292519943295 };
  static const double c[] = { -1000, -30, 0, 50, 3500 };
  size_t N = sizeof( c ) / sizeof( c[ 0 ] );
  size_t END = N-1;
    
  /** look up internal torque based on dphi
      if too low (< -b[ 0 ]) then c[ 0 ]
      if too high (> b[ 4 ]) then c[ 4 ]
      else lerp between two values in c
  */

  double tc = c[ 0 ]; //clutch torque

  if (dphi <= b[ 0 ]) {
    tc = (dphi - b[ 0 ]) / 0.034906585039886591 *  970.0 + c[ 0 ];
  } else if ( dphi >= b[ END ] ) {
    tc = ( dphi - b[ END ] ) / 0.078539816339744828 * 3450.0 + c[ END ];
  } else {
    int i;
    for (i = 0; i < END; ++i) {
      if (dphi >= b[ i ] && dphi <= b[ i+1 ]) {
	double k = (dphi - b[ i ]) / (b[ i+1 ] - b[ i ]);
	tc = (1-k) * c[ i ] + k * c[ i+1 ];
	break;
      }
    }
    if (i >= END ) {
      //too high (shouldn't happen)
      tc = c[ END ];
    }
  }

  //add damping. 
  tc += clutch_damping * domega;

  return tc; 

}

static double fclutch_dphi_derivative( double dphi ) {
  
  //Scania's clutch curve
  static const double b[] = { -0.087266462599716474, -0.052359877559829883, 0.0, 0.09599310885968812, 0.17453292519943295 };
  static const double c[] = { -1000, -30, 0, 50, 3500 };
  size_t N = sizeof( c ) / sizeof( c[ 0 ] );
  size_t END = N-1;
    
  /** look up internal torque based on dphi
      if too low (< -b[ 0 ]) then c[ 0 ]
      if too high (> b[ 4 ]) then c[ 4 ]
      else lerp between two values in c
  */

  double df = 0;		// clutch derivative

  if (dphi <= b[ 0 ]) {
    df =  1.0 / 0.034906585039886591 *  970.0 ;
  } else if ( dphi >= b[ END ] ) {
    df =  1.0 / 0.078539816339744828 * 3450.0 ;
  } else {
    int i;
    for (i = 0; i < END; ++i) {
      if (dphi >= b[ i ] && dphi <= b[ i+1 ]) {
	double k =  1.0  / (b[ i+1 ] - b[ i ]);
	df = k * ( c[ i + 1 ] -  c[ i ] );
	break;
      }
    }
    if (i >= END ) {
      //too high (shouldn't happen)
      df = 0;
    }
  }

  return df; 

}


/** TODO */
static int epce_post_step(int n, const double outputs[], void * params) {
/**
   state_t *s = params;
   double v = flip * s->md.v_in;

   s->md.v            = outputs[1];
   s->md.force_clutch = fclutch( outputs[ 2 ], ( outputs[ 1 ] - v ), s->md.clutch_damping );
*/

  return GSL_SUCCESS;
}


static void clutch_init(state_t *s) {
  const double initials[3] = {
    s->md.x0_e,
    s->md.v0_e,
    s->md.dx0_e,
    s->md.x0_s,
    s->md.v0_s,
    s->md.dx0_s
  };


    
  s->simulation = cgsl_init_simulation(
    cgsl_epce_default_model_init(
      cgsl_model_default_alloc(sizeof(initials)/sizeof(initials[0]), initials, s, clutch, jac_clutch, NULL, NULL, 0),
      s->md.filter_length,
      epce_post_step,
      s
      ),
    rkf45, 1e-5, 0, 0, 0, NULL
    );
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {
  cgsl_step_to( &s->simulation, currentCommunicationPoint, communicationStepSize );
}


//gcc -g clutch.c ../../../templates/gsl2/gsl-interface.c -DCONSOLE -I../../../templates/gsl2 -I../../../templates/fmi2 -lgsl -lgslcblas -lm -Wall
#ifdef CONSOLE
int main(){

  FILE * f = fopen("data.m", "w+");

  state_t s = {{
      8.0, 			/* init position */
      4.0, 			/* init velocity */
      0.0, 			/* init angle difference*/
      10.0, 			/* mass */
      2.0,			/* damping */
      10.0,			/* clutch damping */
      15.0,			/* init angular velocity */
      10,				/* input force */
    }};

  clutch_init(&s);
  s.simulation.file = fopen( "s.m", "w+" );
  s.simulation.save = 1;
  s.simulation.print = 1;
  cgsl_step_to( &s.simulation, 0.0, 10.0 );
  cgsl_free_simulation(s.simulation);

  return 0;
}
#else

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"

#endif
