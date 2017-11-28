//fix WIN32 build
#include "hypotmath.h"

#include "modelDescription.h"
#include "gsl-interface.h"

#define SIMULATION_TYPE cgsl_simulation
#define SIMULATION_EXIT_INIT clutch_init
#define SIMULATION_FREE cgsl_free_simulation

#include "fmuTemplate.h"

/**
 *  The force function from the clutch
 */
static double fclutch( double dphi, double domega, double clutch_damping ); 
static double fclutch_dphi_derivative( double dphi );


/*  
    Two rotational bodies.  The force between them comes for an empirical
    clutch deviation-torque curve. 

    The first of the two bodies receives a torque from the outside.  The
    second body reports velocity.  Feedback torque is applied directly on
    the output body.

*/

int clutch (double t, const double x[], double dxdt[], void * params){

  
  state_t *s = (state_t*)params;
  double engaged = fabs( s->md.on_off ) > 0.1  ;
  double force_clutch =   fclutch( x[ 2 ] - x[ 0 ], ( x[ 3 ] - x[ 1 ] ), s->md.clutch_damping );
  force_clutch = engaged  * force_clutch;

  /** fixme here: need some slip etc. */

    dxdt[ 0 ]  = engaged * x[ 1 ];
    dxdt[ 1 ] =  force_clutch - s->md.gamma1 * x[ 1 ] ;
    dxdt[ 1 ] += s->md.force_in1;
    dxdt[ 1 ] /= s->md.mass1;

    dxdt[ 2 ]  = engaged * x[ 3 ];
    dxdt[ 3 ] =  -force_clutch - s->md.gamma2 * x[ 3 ] ;
    dxdt[ 3 ] += s->md.force_in2;
    dxdt[ 3 ] /= s->md.mass2;

  return GSL_SUCCESS;

}



int jac_clutch (double t, const double x[], double *dfdx, double dfdt[], void *params)
{
  state_t *s = (state_t*)params;
  gsl_matrix_view dfdx_mat = gsl_matrix_view_array (dfdx, 4, 4);
  gsl_matrix * J = &dfdx_mat.matrix;

  double engaged = fabs( s->md.on_off ) > 0.1  ;
  
  double dfdphi = engaged * fclutch_dphi_derivative( x[ 2 ] - x[ 0 ] );

  gsl_matrix_set (J, 0, 0, 1.0-engaged);
  gsl_matrix_set (J, 0, 1, engaged);  /* v1 */
  gsl_matrix_set (J, 0, 2, 0);
  gsl_matrix_set (J, 0, 3, 0);

  gsl_matrix_set (J, 1, 0,  -dfdphi                               / s->md.mass1);
  gsl_matrix_set (J, 1, 1, (-s->md.clutch_damping - s->md.gamma1) / s->md.mass1);
  gsl_matrix_set (J, 1, 2,   dfdphi                               / s->md.mass1);
  gsl_matrix_set (J, 1, 3,   s->md.clutch_damping                 / s->md.mass1);

  gsl_matrix_set (J, 2, 0, 0);
  gsl_matrix_set (J, 2, 1, 0);
  gsl_matrix_set (J, 2, 2, 1.0-engaged);
  gsl_matrix_set (J, 2, 3, engaged);  /* v2 */

  gsl_matrix_set (J, 3, 0,   dfdphi                               / s->md.mass2);
  gsl_matrix_set (J, 3, 1,   s->md.clutch_damping                 / s->md.mass2);
  gsl_matrix_set (J, 3, 2,  -dfdphi                               / s->md.mass2);
  gsl_matrix_set (J, 3, 3, (-s->md.clutch_damping - s->md.gamma2) / s->md.mass2);

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
    size_t i;
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
    size_t i;
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


static int epce_post_step(double t, double dt, int n, const double outputs[], void * params) {
    state_t *s = params;

    s->md.v1 = outputs[1];
    s->md.v2 = outputs[3];

    return GSL_SUCCESS;
}

static fmi2Status clutch_init(ModelInstance *comp) {
  state_t *s = &comp->s;
  const double initials[4] = {
    s->md.xi0,
    s->md.vi0,
    s->md.xo0,
    s->md.vo0
  };

//    s->simulation = cgsl_init_simulation( 4, initials, s, clutch, jac_clutch, s->md.integrator_type, 1e-5, 0, 0, 0, NULL );
  s->simulation = cgsl_init_simulation(
    cgsl_model_default_alloc(4, initials, s, clutch, jac_clutch, NULL, epce_post_step, 0),
    rkf45, 1e-5, 0, 0, 0, NULL
  );
  return fmi2OK;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
  if ( fabs( s->md.on_off ) < 0.1 ){ 
    s->simulation.model->x[ 0 ] = 0;
    s->simulation.model->x[ 2 ] = 0;
  }
  
  cgsl_step_to( &s->simulation, currentCommunicationPoint, communicationStepSize );
}


//gcc -g clutch_ef.c ../../../templates/gsl2/gsl-interface.c -DCONSOLE -I../../../templates/gsl2 -I../../../templates/fmi2 -lgsl -lgslcblas -lm -Wall
#ifdef CONSOLE
int main(){

  size_t N = 1000;
  double start = -0.2;
  double end   =  0.2;
  FILE * f = fopen("data.m", "w+");
  double dx = ( end - start ) / ( double ) N;
  double  x;
  double  y;
  size_t i;

  for( i=0;  i < N; ++i ){
    x = start + dx * ( double ) i;
    y = fclutch( x, 0.0 , 0.0);
    fprintf( f, "%1.5g  %1.5g\n", x, y);
  }

  fclose( f );

  state_t s = {{
      0,
      0,
      0,
      0,
      1,
      1,
      1,
      1,
      100,
      1,
      -1,
      0,
      0,
      rk2imp,
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
