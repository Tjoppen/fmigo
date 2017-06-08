//fix WIN32 build
#include "hypotmath.h"

#include "modelDescription.h"
#include "gsl-interface.h"

#define SIMULATION_TYPE cgsl_simulation
#define SIMULATION_EXIT_INIT engine2_init
#define SIMULATION_FREE cgsl_free_simulation
#define SIMULATION_GET  cgsl_simulation_get   //TODO: some kind of error if these aren't defined
#define SIMULATION_SET  cgsl_simulation_set


#include "fmuTemplate.h"

//this function returns the amount of "gas" which is applied
static double beta( state_t *s, double omega_engine ) {
  double ret = s->md.kp * (s->md.omega_target - omega_engine);
  if (ret < 0) return 0;
  if (ret > 1) return 1;
  return ret;
}

static void compute_forces(state_t *s, const double x[], double *tau_coupling) {
  if (tau_coupling) {
    *tau_coupling =   s->md.k_in * (s->md.integrate_dtheta ? x[ 2 ] : (x[ 0 ] - s->md.theta_in))
                    + s->md.d_in * (x[ 1 ] - s->md.omega_in);
  }
}

int engine2 (double t, const double x[], double dxdt[], void * params) {

  state_t *s = (state_t*)params;

  double tau_coupling;
  double tau_total;
  compute_forces(s, x, &tau_coupling);
  tau_total = (s->md.tau_max * beta(s, x[ 1 ])
	       - s->md.k1 * x[ 1 ]
	       - s->md.k2 * x[ 1 ] * floor(fabs(x[ 1 ]))
	       + s->md.tau_in
	       - tau_coupling
    );
  dxdt[ 0 ] = x[ 1 ];
  dxdt[ 1 ] = s->md.jinv * tau_total;

  dxdt[ 2 ] = s->md.integrate_dtheta ? x[ 1 ] - s->md.omega_in : 0.0;


  return GSL_SUCCESS;

}



int jac_engine2 (double t, const double x[], double *dfdx, double dfdt[], void *params)
{

  state_t *s = (state_t*)params;
  gsl_matrix_view dfdx_mat = gsl_matrix_view_array (dfdx, 3, 3);
  gsl_matrix * J = &dfdx_mat.matrix;
  double b = beta(s, x[ 1 ]);

  /** first row */
  gsl_matrix_set (J, 0, 0, 0.0);
  gsl_matrix_set (J, 0, 1, 1.0 ); /* position/velocity */
  gsl_matrix_set (J, 0, 2, 0.0);

  /** second row */
  gsl_matrix_set (J, 1, 0, s->md.integrate_dtheta ? 0.0 : -s->md.k_in );
  gsl_matrix_set (J, 1, 1, s->md.jinv * (s->md.tau_max * (b > 0 && b < 1 ? -s->md.kp : 0)
                                         - s->md.k1
                                         - 2 * s->md.k2 * floor(fabs(x[ 1 ])) // was abs before, floor to maintain same behavior, create an impulse
                                         - s->md.d_in
                                         ));
  gsl_matrix_set (J, 1, 2, s->md.integrate_dtheta ? -s->md.k_in : 0.0);

  gsl_matrix_set (J, 2, 0, 0.0);
  gsl_matrix_set (J, 2, 1, s->md.integrate_dtheta ? 1.0 : 0.0);
  gsl_matrix_set (J, 2, 2, 0.0);

  dfdt[0] = 0.0;
  dfdt[1] = 0.0; /* would have a term here if the force is
		    some polynomial interpolation */

  return GSL_SUCCESS;
}

#define HAVE_INITIALIZATION_MODE
static int get_initial_states_size(state_t *s) {
  return 3;
}

static void get_initial_states(state_t *s, double *initials) {
  initials[0] = s->md.theta0;
  initials[1] = s->md.omega0;
  initials[2] = 0;
}

static int sync_out(double t, int n, const double outputs[], void * params) {
  state_t *s = params;
  double dxdt[3];

  engine2(0, outputs, dxdt, params);
  compute_forces(s, outputs, &s->md.tau_out);

  s->md.theta_out = outputs[ 0 ];
  s->md.omega_out = outputs[ 1 ];
  s->md.alpha_out = dxdt[ 1 ];

  return GSL_SUCCESS;
}


static fmi2Status engine2_init(ModelInstance *comp) {
  state_t *s = &comp->s;

  double initials[3];
  get_initial_states(s, initials);

  s->simulation = cgsl_init_simulation(
    cgsl_epce_default_model_init(
      cgsl_model_default_alloc(get_initial_states_size(s), initials, s, engine2, jac_engine2, NULL, NULL, 0),
      s->md.filter_length,
      sync_out,
      s
    ),
    s->md.integrator, 1e-6, 0, 0, s->md.octave_output, s->md.octave_output ? fopen(s->md.octave_output_file, "w") : NULL
  );
  //s->simulation.file = fopen( "engine2.m", "w+" );
  //s->simulation.print = 1;
  return fmi2OK;
}

static fmi2Status getPartial(ModelInstance *comp, fmi2ValueReference vr, fmi2ValueReference wrt, fmi2Real *partial) {
  if (vr == VR_ALPHA_OUT && wrt == VR_TAU_IN) {
    *partial = comp->s.md.jinv;
    return fmi2OK;
  }
  return fmi2Error;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
  //don't dump tentative steps
  s->simulation.print = noSetFMUStatePriorToCurrentPoint;
  cgsl_step_to( &s->simulation, currentCommunicationPoint, communicationStepSize );
}


#ifdef CONSOLE
int main(){

  return 0;
}
#else
#include "fmuTemplate_impl.h"
#endif
