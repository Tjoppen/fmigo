//fix WIN32 build
#include "hypotmath.h"

#include "modelDescription.h"
#include "gsl-interface.h"

#define SIMULATION_TYPE cgsl_simulation
#define SIMULATION_EXIT_INIT mass_force_init
#define SIMULATION_FREE cgsl_free_simulation

#include "fmuTemplate.h"

/*  Unit mass subject to a force

    x''  = f

 */

int mass_force (double t, const double x[], double dxdt[], void * params){

  state_t *s = (state_t*)params;

  /** compute the coupling force: NOTE THE SIGN!
   *  This is the force *applied* to the coupled system
   */
  
  dxdt[ 0 ] = x[ 1 ];
  dxdt[ 1 ] =  ( s->md.force + s->md.force_c - s->md.damping * x[ 1 ] ) / s->md.mass;

  return GSL_SUCCESS;

}



int
jac_mass_force (double t, const double x[], double *dfdx, double dfdt[], void *params)
{
  state_t *s = (state_t*)params;
  gsl_matrix_view dfdx_mat = gsl_matrix_view_array (dfdx, 2, 2);
  gsl_matrix * J           = &dfdx_mat.matrix; 

  /** first row */
  gsl_matrix_set (J, 0, 0, 0.0); 
  gsl_matrix_set (J, 0, 1, 1.0 ); /* position/velocity */

  /** second row */
  gsl_matrix_set (J, 1, 0, 0.0);
  gsl_matrix_set (J, 1, 1, -s->md.damping / s->md.mass );

  dfdt[0] = 0.0;		
  dfdt[1] = 0.0; /* would have a term here if the force is
		    some polynomial interpolation */

  return GSL_SUCCESS;
}

static int epce_post_step(int n, const double outputs[], void * params) {
    state_t *s = params;

    s->md.x = outputs[0];
    s->md.v = outputs[1];

    return GSL_SUCCESS;
}

static fmi2Status mass_force_init(ModelInstance *comp) {
    state_t *s = &comp->s;
    const double initials[2] = {s->md.x, s->md.v};
    s->simulation = cgsl_init_simulation(
        cgsl_epce_default_model_init(
            cgsl_model_default_alloc(2, initials, s, mass_force, jac_mass_force, NULL, NULL, 0),
            s->md.filter_length,
            epce_post_step,
            s
        ),
        rkf45, 1e-5, 0, 0, 0, NULL
    );
    return fmi2OK;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {
    cgsl_step_to( &s->simulation, currentCommunicationPoint, communicationStepSize );
}

//gcc -g mass_force.c ../../../templates/gsl2/gsl-interface.c -DCONSOLE -I../../../templates/gsl2 -I../../../templates/fmi2 -lgsl -lgslcblas -lm -Wall
#ifdef CONSOLE
int main(void) {
    state_t s = {
        {
            1,
            0,
            0,
            0,
            0,
            1,
        }
    };
    mass_force_init(&s);
    s.simulation.file = fopen( "foo.m", "w+" );
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
