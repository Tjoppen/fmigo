#include "modelDescription.h"
#include "gsl-interface.h"

#define SIMULATION_TYPE cgsl_simulation
#define SIMULATION_INIT clutch_init
#define SIMULATION_FREE cgsl_free_simulation

#include "fmuTemplate.h"

const static double flip = -1;

/*  
    One translational body driven by a rotational variable.  

    This represents a trailer moving on a road with variable gradient,
    driven by a shaft, via a differential with gear ratio r_d, via a
    wheel of radius r_w. 

    
    Variables are listed as: 
    x    : position of trailer along road (parametric)
    v    : speed of truck
    dphi : angle difference estimate between the input shaft and the
    differential 


 */

#define SIGNUM( x ) 

int trailer (double t, const double x[], double dxdt[], void * params){

  state_t *s = (state_t*)params;

  /* gravity */
  double force = - s->md.mass * s->md.g * sin( s->md.alpha );

  /* drag */
  force += 
    - ( (x > 0) ? 1 : ( (x < 0) ? -1 : 0) ) *
    0.5 * s->md.rho  * s->md.A * s->md.c_d * x[ 1 ] * x[ 1 ];

  /* brake */ 
  force +=
    - s->md.brake * s->md.mu * s->md.g * cos( s->md.alpha );

  /* any additional force */
  force += s->md.force_in;

  s->md.torque_e =   s->md.gamma_c * ( x[ 1 ] / s->md->r_d / s->md->r_w - s->md.omega_in );
  
  if ( s->md.integrate_d_omega )
    s->md.torque_e +=  s->md.k_c *  x[ 2 ];
  else
    s->md.torque_e +=  s->md.k_c *  ( x[ 2 ] - s->md.phi_in );

  /* coupling force */
										   
  dxdt[ 1 ]  = ( 1.0 / s->md.mass ) * ( force - s->md.torque_e / s->md.r_w );
 
  /** angle difference */
  if ( s->md.integrate_dx_e )
    dxdt[ 2 ] = x[ 1 ] - s->md.omega_in;

  
  return GSL_SUCCESS;

}



  /** TODO */
int jac_trailer (double t, const double x[], double *dfdx, double dfdt[], void *params)
{
  
  state_t *s = (state_t*)params;
  gsl_matrix_view dfdx_mat = gsl_matrix_view_array (dfdx, 3, 3);
  gsl_matrix * J = &dfdx_mat.matrix; 


  return GSL_SUCCESS;
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


static void trailer_init(state_t *s) {
    const double initials[3] = {s->md.x0_e,
				s->md.v0_e,
				s->md.dx0};


    
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


//gcc -g trailer.c ../../../templates/gsl2/gsl-interface.c -DCONSOLE -I../../../templates/gsl2 -I../../../templates/fmi2 -lgsl -lgslcblas -lm -Wall
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
  cgsl_free_simulation(s.simulation);

  return 0;
}
#else

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"

#endif
