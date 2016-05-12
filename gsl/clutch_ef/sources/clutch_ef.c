#include "modelDescription.h"
#include "gsl-interface.h"

#define SIMULATION_TYPE cgsl_simulation
#define SIMULATION_INIT clutch_init
#define SIMULATION_FREE cgsl_free

#include "fmuTemplate.h"

/**
 *  The force function from the clutch
 */
static double fclutch( double dphi, double domega, double clutch_damping ); 
static double fclutch_dphi_derivative( double dphi, double domega );

const static double flip = -1;

/*  
    Two rotational bodies.  The force between them comes for an empirical
    clutch deviation-torque curve. 

    The first of the two bodies receives a torque from the outside.  The
    second body reports velocity.  Feedback torque is applied directly on
    the output body.

 */

int clutch (double t, const double x[], double dxdt[], void * params){

  state_t *s = (state_t*)params;

  s->md.force_clutch =   fclutch( x[ 2 ] - x[ 0 ], ( x[ 3 ] - x[ 1 ] ), s->md.clutch_damping );

  dxdt[ 0 ]  = x[ 1 ];
  dxdt[ 1 ] =  -s->md.force_clutch - s->md.gamma1 * x[ 1 ] ;
  dxdt[ 1 ] += s->md.force_in1;
  dxdt[ 1 ] /= s->md.mass1;

  dxdt[ 2 ]  = x[ 3 ];
  dxdt[ 3 ] =  -s->md.force_clutch - s->md.gamma2 * x[ 3 ] ;
  dxdt[ 3 ] += s->md.force_in2;
  dxdt[ 3 ] /= s->md.mass2;

  return GSL_SUCCESS;

}



int jac_clutch (double t, const double x[], double *dfdx, double dfdt[], void *params)
{
  
#if 0
  NEEDS WORK!
  state_t *s = (state_t*)params;
  gsl_matrix_view dfdx_mat = gsl_matrix_view_array (dfdx, 4, 4);
  gsl_matrix * J = &dfdx_mat.matrix; 
  double v =  flip * s->md.v_in;

  /** first row */
  gsl_matrix_set (J, 0, 0, 0.0); 
  gsl_matrix_set (J, 0, 1, 1.0 ); /* position/velocity */
  gsl_matrix_set (J, 0, 2, 0.0 ); 
  gsl_matrix_set (J, 0, 3, 0.0 ); 

  /** third row*/
  gsl_matrix_set (J, 2, 0, 0.0); 
  gsl_matrix_set (J, 2, 1, 0.0 ); /* position/velocity */
  gsl_matrix_set (J, 2, 2, 0.0 ); 
  gsl_matrix_set (J, 2, 3, 1.0 ); 


  
  dfdt[0] = 0.0;		
  dfdt[1] = 0.0;		
  dfdt[2] = 0.0; /* would have a term here if the force is
		    some polynomial interpolation */

#endif
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

static double fclutch_dphi_derivative( double dphi, double domega ) {
  
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


static void sync_out(state_t *s) {
    s->md.v = s->simulation.x[1];
}

static void clutch_init(state_t *s) {
    const double initials[4] = {s->md.x0, s->md.v0, s->md.dx0};
    s->simulation = cgsl_init_simulation( 4, initials, s, clutch, jac_clutch, rkf45, 1e-5, 0, 0, 0, NULL );
    sync_out(s);
}

//returns partial derivative of vr with respect to wrt
static fmi2Status getPartial(state_t *s, fmi2ValueReference vr, fmi2ValueReference wrt, fmi2Real *partial) {
    return fmi2Error;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {
    cgsl_step_to( &s->simulation, currentCommunicationPoint, communicationStepSize );
    sync_out(s);
}


//gcc -g clutch.c ../../../templates/gsl/*.c -DCONSOLE -I../../../templates/gsl -I../../../templates/fmi2 -lgsl -lgslcblas -lm -Wall
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
  cgsl_free(s.simulation);

  return 0;
}
#else

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"

#endif
