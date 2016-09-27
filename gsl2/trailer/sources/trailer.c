#include "modelDescription.h"
#include "gsl-interface.h"
#include <math.h>

#define SIMULATION_TYPE cgsl_simulation
#define SIMULATION_INIT trailer_init
#define SIMULATION_FREE cgsl_free_simulation

#include "fmuTemplate.h"

const static double flip = -1;

/*  
    One translational body driven by a rotational variable.  

    This represents a trailer moving on a road with variable gradient,
    driven by a shaft, via a differential with gear ratio r_g, via a
    wheel of radius r_w. 

    
    Variables are listed as: 
    x    : position of trailer along road (parametric)
    v    : speed of truck
    dphi : angle difference estimate between the input shaft and the
    differential 
    dx   : position difference between attached load


*/

#define SIGNUM( x ) ( (x > 0) ? 1 : ( (x < 0) ? -1 : 0 ) )

int trailer (double t, const double x[], double dxdt[], void * params){

  state_t *s = (state_t*)params;

  /* gravity */
  double force = - s->md.mass * s->md.g * sin( s->md.alpha );

  double sgnv = SIGNUM( x[ 1 ] );
  /* drag */
  force += 
    -  sgnv *   0.5 * s->md.rho  * s->md.area * s->md.c_d * x[ 1 ] * x[ 1 ];

  /* brake */ 
  force +=
    - sgnv * s->md.brake * s->md.mu * s->md.g * cos( s->md.alpha );

  /* rolling resistance */
  force += 
    - sgnv * ( s->md.c_r_1 * fabs( x[ 1 ] ) + s->md.c_r_0 ) * s->md.mass * s->md.g * cos( s->md.alpha );

  /* any additional force */
  force += s->md.tau_e / s->md.r_w;

  /* coupling torque */
  s->md.tau_c =   s->md.gamma * ( x[ 1 ] / s->md.r_g / s->md.r_w - s->md.omega_i );
  
  if ( s->md.integrate_d_omega ) { 
    s->md.tau_c +=  s->md.k *  x[ 2 ];
    dxdt[ 2 ] = x[ 1 ] / s->md.r_g / s->md.r_w - s->md.omega_i;
  }
  else{
    s->md.tau_c +=  s->md.k *  ( x[ 0 ] / s->md.r_g / s->md.r_w - s->md.phi_i );
    dxdt[ 2 ] = 0;
  }

  /* total acceleration */
  dxdt[ 1 ]  = ( 1.0 / s->md.mass ) * ( force - s->md.tau_c / s->md.r_w );
  dxdt[ 0 ]  = x[ 1 ];
 
  
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




/** TODO: this is the sync out function */
static int epce_post_step(int n, const double outputs[], void * params) {
  state_t *s = ( state_t * ) params;
   s->md.x            = outputs[0];
   s->md.v            = outputs[1];
   s->md.a            = 0;	/* could make another call to trailer(...) to get the correct value here. */

  return GSL_SUCCESS;
}


static void trailer_init(state_t *s) {

  const double initials[] = {s->md.x0,
			     s->md.v0,
			     0.0
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


#ifdef CONSOLE
int main(){

  state_t s = {{
      0.0, 			/* init position */
      4.0, 			/* init velocity */
      10.0, 			/* mass */
      10,			/* wheel radius r_w*/
      1,			/* differential gear ratio */
      2.0,			/* area */
      1.0,			/* rho*/
      1.0,			/* drag coeff c_d*/
      10,			/* gravity */
      0.2,			/* c_r_1 rolling resistance */
      0.4,			/* c_r_2 rolling resistance */
      1,			/* friction coeff*/
      1e4,			/* coupling spring constant  */
      1e2,			/* coupling damping constant  */
      rkf45,		/* which method*/
      1,		/* integrate input speed on differential*/
      1,		/* coupling speed for load*/
      0,		/* differential input displacement */
      10,		/* differential input speed */
      0,		/* torque input on differential*/
      0,		/* extra torque input on differential*/
      0,		/* road elevation angle */
      0,		/* brake position */
    }};
  trailer_init(&s);
  s.simulation.file = fopen( "s.m", "w+" );
  s.simulation.save = 1;
  s.simulation.print = 1;
  cgsl_step_to( &s.simulation, 0.0, 8.0 );
  cgsl_free_simulation(s.simulation);

  return 0;
}
#else

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"

#endif
