//fix WIN32 build
#include "hypotmath.h"

#include "modelDescription.h"
#include "gsl-interface.h"

#define SIMULATION_TYPE cgsl_simulation
#define SIMULATION_EXIT_INIT chained_sho_init
#define SIMULATION_FREE cgsl_free_simulation

#include "fmuTemplate.h"

/*  The simple forced harmonic oscillator with an external coupling:

    m * x'' + damping_i * x'  + k_i * x = -  k_c *  dx  - damping_c * ( x'
    - x0' )  + force_c + force
    dx' = x' - x0';
    
    dx is the estimated angle difference. 
    

 */

static int chained_sho (double t, const double x[], double dxdt[], void * params){

  state_t *s  = (state_t*)params;

  /** compute the coupling force: NOTE THE SIGN!
   *  This is the force *applied* to the coupled system
   */
  double imass = 1.0 / s->md.mass;

  s->md.force_c =   s->md.damping_c * ( x[ 1 ] - s->md.v_c )  + s->md.k_c * x[ 2 ];

  double force_i = -s->md.k_i * ( x[ 0 ] - s->md.x0 ) - s->md.damping_i * x[ 1 ];

  dxdt[ 0 ]  = x[ 1 ];

  /** dynamics */ 
  dxdt[ 1 ] = imass *  ( force_i - s->md.force_c  + s->md.force + s->md.e_force);

  /** integrate angle difference */
  dxdt[ 2 ] = x[ 1 ] - s->md.v_c;

  return GSL_SUCCESS;

}



static int jac_chained_sho (double t, const double x[], double *dfdx, double dfdt[], void *params)
{
  state_t *s  = (state_t*)params;
  double imass = 1.0 / s->md.mass;

  gsl_matrix_view dfdx_mat = gsl_matrix_view_array (dfdx, 3, 3);
  gsl_matrix * J = &dfdx_mat.matrix; 

  /** first row */
  gsl_matrix_set (J, 0, 0, 0.0); 
  gsl_matrix_set (J, 0, 1, 1.0 ); /* position/velocity */
  gsl_matrix_set (J, 0, 2, 0.0 ); 

  /** second row */
  gsl_matrix_set (J, 1, 0, -imass * s->md.k_i );
  gsl_matrix_set (J, 1, 1, -imass * ( s->md.damping_i + s->md.damping_c ) );
  gsl_matrix_set (J, 1, 2, -imass * s->md.k_c );

  /** third row */
  gsl_matrix_set (J, 2, 0, 0.0 );
  gsl_matrix_set (J, 2, 1, 1.0 ); /* angle difference */
  gsl_matrix_set (J, 2, 2, 0.0 ); 

  dfdt[0] = 0.0;		
  dfdt[1] = 0.0;		
  dfdt[2] = 0.0; /* would have a term here if the force is
		    some polynomial interpolation */

  return GSL_SUCCESS;
}

static int epce_post_step(double t, int n, const double outputs[], void * params) {
    state_t *s = params;

    s->md.x = outputs[0];
    s->md.v = outputs[1];
    s->md.force_c    = s->md.damping_c * ( outputs[ 1 ] - s->md.v_c )  + s->md.k_c * outputs[ 2 ];
    s->md.steps = s->simulation.iterations;

    return GSL_SUCCESS;
}

static fmi2Status chained_sho_init(ModelInstance *comp) {
    state_t *s = &comp->s;
    const double initials[3] = {s->md.xstart, s->md.vstart, s->md.dxstart};

    FILE *f = NULL;

    if (s->md.dump_data) {
        f = fopen("sho.m", "w");
    }

    s->simulation = cgsl_init_simulation(
        cgsl_epce_default_model_init(
            cgsl_model_default_alloc(3, initials, s, chained_sho, jac_chained_sho, NULL, NULL, 0),
            s->md.filter_length,
            epce_post_step,
            s
        ),
        rkf45, 1e-5, 0, 0, s->md.dump_data, f
    );

    return fmi2OK;
}


static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
    cgsl_step_to( &s->simulation, currentCommunicationPoint, communicationStepSize );
}

//gcc -g chained_sho.c ../../../templates/gsl2/gsl-interface.c -DCONSOLE -I../../../templates/gsl2 -I../../../templates/fmi2 -lgsl -lgslcblas -lm -Wall
#ifdef CONSOLE
int main(void) {
#if 0 
TODO : revise this to adapt to a fully dimensional model
    state_t s = {
        {
            1.0,
            1.0,
            0.0,
            0.0,
            0.0,
            1.0, 
            0.0,
            8,
            -4,
            0,
            0,
        }
    };
    chained_sho_init(&s);
    s.simulation.file = fopen( "foo.m", "w+" );
    s.simulation.save = 1;
    s.simulation.print = 1;

    cgsl_step_to( &s.simulation, 0.0, 10.0 );
    cgsl_free(s.simulation);
#endif
    return 0;
}
#else

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"
#endif
