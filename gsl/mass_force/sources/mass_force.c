#include "modelDescription.h"
#include "gsl-interface.h"

#define SIMULATION_TYPE cgsl_simulation
#define SIMULATION_INIT mass_force_init
#define SIMULATION_FREE cgsl_free

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
  dxdt[ 1 ] =  s->md.force;

  return GSL_SUCCESS;

}



int
jac_mass_force (double t, const double x[], double *dfdx, double dfdt[], void *params)
{
  gsl_matrix_view dfdx_mat = gsl_matrix_view_array (dfdx, 2, 2);
  gsl_matrix * J           = &dfdx_mat.matrix; 

  /** first row */
  gsl_matrix_set (J, 0, 0, 0.0); 
  gsl_matrix_set (J, 0, 1, 1.0 ); /* position/velocity */

  /** second row */
  gsl_matrix_set (J, 1, 0, 0.0);
  gsl_matrix_set (J, 1, 1, 0.0);

  dfdt[0] = 0.0;		
  dfdt[1] = 0.0; /* would have a term here if the force is
		    some polynomial interpolation */

  return GSL_SUCCESS;
}

static void mass_force_init(state_t *s) {
    const double initials[2] = {s->md.x, s->md.v};
    s->simulation = cgsl_init_simulation( 2, initials, s, mass_force, jac_mass_force, rkf45, 1e-5, 0, 0, 0, NULL );
}

//returns partial derivative of vr with respect to wrt
static fmi2Status getPartial(state_t *s, fmi2ValueReference vr, fmi2ValueReference wrt, fmi2Real *partial) {
    return fmi2Error;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {
    cgsl_step_to( &s->simulation, currentCommunicationPoint, communicationStepSize );
}

//gcc -g mass_force.c ../../../templates/gsl/*.c -DCONSOLE -I../../../templates/gsl -I../../../templates/fmi2 -lgsl -lgslcblas -lm -Wall
#ifdef CONSOLE
int main(void) {
    state_t s = {
        {
            1,
            0,
            0,
        }
    };
    mass_force_init(&s);
    s.simulation.file = fopen( "foo.m", "w+" );
    s.simulation.save = 1;
    s.simulation.print = 1;

    cgsl_step_to( &s.simulation, 0.0, 10.0 );
    cgsl_free(&s.simulation);
    return 0;
}
#else

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"

#endif
