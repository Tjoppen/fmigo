//fix WIN32 build
#include "hypotmath.h"

#include "modelDescription.h"
#include "gsl-interface.h"

#define SIMULATION_TYPE cgsl_simulation
#define SIMULATION_INIT engine2_init
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
  compute_forces(s, x, &tau_coupling);

  dxdt[ 0 ] = x[ 1 ];
  dxdt[ 1 ] = s->md.jinv * (s->md.tau_max * beta(s, x[ 1 ])
                            - s->md.k1 * x[ 1 ]
                            - s->md.k2 * x[ 1 ] * floor(fabs(x[ 1 ]))
                            + s->md.tau_in
                            - tau_coupling
                            );
  dxdt[ 2 ] = s->md.integrate_dtheta ? x[ 1 ] - s->md.omega_in : 0.0;
  //fprintf(stderr, "forces: %f - %f - %f + %f @ t = %f, x = %f\n", s->md.tau_max * beta(s, x[ 1 ]), s->md.k1 * x[ 1 ], s->md.k2 * x[ 1 ] * floor(fabs(x[ 1 ])), s->md.tau_coupling, t, x[0]);

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
                                         - 2 * s->md.k2 * floor(fabs(abs(x[ 1 ])))
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

static int sync_out(int n, const double outputs[], void * params) {
  state_t *s = params;
  double dxdt[3];

  engine2(0, outputs, dxdt, params);
  compute_forces(s, outputs, &s->md.tau_out);

  s->md.theta_out = outputs[ 0 ];
  s->md.omega_out = outputs[ 1 ];
  s->md.alpha_out = dxdt[ 1 ];

  return GSL_SUCCESS;
}


static void engine2_init(state_t *s) {

  double initials[3];
  get_initial_states(s, initials);

  s->simulation = cgsl_init_simulation(
    cgsl_epce_default_model_init(
      cgsl_model_default_alloc(get_initial_states_size(s), initials, s, engine2, jac_engine2, NULL, NULL, 0),
      s->md.filter_length,
      sync_out,
      s
    ),
    rkf45, 1e-5, 0, 0, 0, NULL
  );
  s->simulation.file = fopen( "engine2.m", "w+" );
  s->simulation.print = 1;

}

static fmi2Status getPartial(state_t *s, fmi2ValueReference vr, fmi2ValueReference wrt, fmi2Real *partial) {
  if (vr == VR_ALPHA_OUT && wrt == VR_TAU_IN) {
    *partial = s->md.jinv;
    return fmi2OK;
  }
  return fmi2Error;
}

#define NEW_DOSTEP //to get noSetFMUStatePriorToCurrentPoint
static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
  //don't dump tentative steps
  s->simulation.print = noSetFMUStatePriorToCurrentPoint;
  cgsl_step_to( &s->simulation, currentCommunicationPoint, communicationStepSize );
}


#ifdef CONSOLE
int main(){

  state_t s = {defaults};
  s.md.jinv = 0.02;
  s.md.k2 = 0.5;

  engine2_init(&s);
  s.simulation.file = fopen( "s.m", "w+" );
  s.simulation.save = 1;
  s.simulation.print = 1;
  cgsl_step_to( &s.simulation, 0.0, 2.5 );
  cgsl_free_simulation(s.simulation);

  return 0;
}
#else
#include "fmuTemplate_impl.h"
#endif

