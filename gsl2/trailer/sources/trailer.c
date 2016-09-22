#include "modelDescription.h"
#include "gsl-interface.h"

#define SIMULATION_TYPE cgsl_simulation
#define SIMULATION_INIT trailer_init
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

#define SIGNUM( x ) ( (x > 0) ? 1 : ( (x < 0) ? -1 : 0 ) )

int trailer (double t, const double x[], double dxdt[], void * params){

  state_t *s = (state_t*)params;

  /* gravity */
  double force = - s->md.mass * s->md.g * sin( s->md.alpha );

  double sgnv = SIGNUM( x[ 1 ] )
  /* drag */
  force += 
    -  sgnv *  abs_v * 0.5 * s->md.rho  * s->md.A * s->md.c_d * x[ 1 ] * x[ 1 ];

  /* brake */ 
  force +=
    - sgnv * s->md.brake * s->md.mu * s->md.g * cos( s->md.alpha );

  /* rolling resistance */
  force += 
    - sgnv * ( s->md.c_r_1 * fabs( x[ 1 ] ) + s->md.c_r_0 ) * s->md.mass * s->md.g * cos( s->md.alpha );

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
  gsl_matrix_view dfdx_mat = gsl_matrix_view_array (dfdx, 2, 2);
  gsl_matrix * J = &dfdx_mat.matrix; 


  return GSL_SUCCESS;
}




/** TODO: this is the sync out function */
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

  const double initials[] = {s->md.x0,
			     s->md.v0
  };


    
  s->simulation = cgsl_init_simulation(
    cgsl_epce_default_model_init(
      cgsl_model_default_alloc(sizeof(initials)/sizeof(initials[0]), initials, s, trailer, jac_trailer, NULL, NULL, 0),
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


  fclose( f );

  state_t s = {{
      8.0, 			/* init position */
      4.0, 			/* init velocity */
      10.0, 			/* mass */
      2.0,			/* area */
      10.0,			/* drag coeff c_d*/
      15.0,			/* rho*/
      10,			/* wheel radius r_w*/
      1,			/* differential gear ratio */
      10,			/* gravity */
      1,			/* c_r_1 rolling resistance */
      1,			/* c_r_2 rolling resistance */
      1000,			/* coupling spring constant  */
      10,			/* coupling damping constant  */
      1,			/* friction coeff*/
      rk4,		/* which method*/
      0,		/* coupling displacement */
      0,		/* coupling speed*/
      0,		/* torque input */
      0,		/* road elevation */
      0,		/* brake position */
    }};
  trailer_init(&s);
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
