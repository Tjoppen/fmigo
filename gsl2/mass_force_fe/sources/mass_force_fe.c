#include "modelDescription.h"
#include "gsl-interface.h"

#define SIMULATION_TYPE cgsl_simulation
#define SIMULATION_INIT mass_force_init
#define SIMULATION_FREE cgsl_free_simulation

#include "fmuTemplate.h"

/*  Mass subject to a force, and coupled to the outside via a
 *  spring-damper.  this takes a force input and outputs a velocity.

 There are two forces  applied here: an input force and a "counter force"
 from the FMU that's attached to the output.

    x''  = f_o  - K * ( dx ) - G * ( x' - v_in )

 */

int mass_force (double t, const double x[], double dxdt[], void * params){

  state_t *s = (state_t*)params;

  /** compute the coupling force: NOTE THE SIGN!
   *  This is the force *applied* to the coupled system
   */
  
  
  double coupling = - s->md.coupling_spring * x[ 2 ] - s->md.coupling_damping * ( x[ 1 ] - s->md.vin );

  s->md.force_out1 = -coupling;
  s->md.force_out2 =  coupling;
  
  dxdt[ 0 ] =  x[ 1 ];

  dxdt[ 1 ] =  ( coupling + s->md.force_c - s->md.damping * x[ 1 ] ) / s->md.mass;
  dxdt[ 2 ] =  x[ 1 ] - s->md.vin;

  return GSL_SUCCESS;

}



int
jac_mass_force (double t, const double x[], double *dfdx, double dfdt[], void *params)
{
  state_t *s = (state_t*)params;
  gsl_matrix_view dfdx_mat = gsl_matrix_view_array (dfdx, 3, 3);
  gsl_matrix * J           = &dfdx_mat.matrix; 

  /** first row */
  gsl_matrix_set (J, 0, 0, 0.0); 
  gsl_matrix_set (J, 0, 1, 1.0 ); /* position/velocity */

  /** second row */
  gsl_matrix_set (J, 1, 0, -s->md.coupling_spring);
  gsl_matrix_set (J, 1, 1, -s->md.damping / s->md.mass );
  gsl_matrix_set (J, 1, 2,  s->md.damping / s->md.mass );

  gsl_matrix_set (J, 2, 0,   0);
  gsl_matrix_set (J, 2, 1,  -1);
  gsl_matrix_set (J, 2, 2,   0);

  dfdt[0] = 0.0;		
  dfdt[1] = 0.0; /* would have a term here if the force is
		    some polynomial interpolation */
  dfdt[2] = 0.0;		

  return GSL_SUCCESS;
}

static int epce_post_step(int n, const double outputs[], void * params) {
    state_t *s = params;

    s->md.x  = outputs[0];
    s->md.v  = outputs[1];
    s->md.dx = outputs[2];

    return GSL_SUCCESS;
}

static void mass_force_init(state_t *s) {
    const double initials[3] = {s->md.x, s->md.v};
    s->simulation = cgsl_init_simulation(
        cgsl_epce_default_model_init(
            cgsl_model_default_alloc(3, initials, s, mass_force, jac_mass_force, NULL, NULL, 0),
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

//gcc -g mass_force_fe.c ../../../templates/gsl2/gsl-interface.c -DCONSOLE -I../../../templates/gsl2 -I../../../templates/fmi2 -lgsl -lgslcblas -lm -Wall
#ifdef CONSOLE
int main(void) {
    state_t s = {
        {
            10,
            0,
            0,
            0,
	    10,
	    1,
            30,
	    10,
	    0
        }
    };
    mass_force_init(&s);
    s.simulation.file = fopen( "foo.m", "w+" );
    s.simulation.save = 1;
    s.simulation.print = 1;

    cgsl_step_to( &s.simulation, 0.0, 100.0 );
    cgsl_free_simulation(s.simulation);
    return 0;
}
#else

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"

#endif
