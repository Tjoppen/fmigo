#include "modelDescription.h"
#include "gsl-interface.h"

#define SIMULATION_TYPE cgsl_simulation
#define SIMULATION_INIT exp_init
#define SIMULATION_FREE cgsl_free_simulation

#include "fmuTemplate.h"

cgsl_model  *  init_exp_model(double x0);
cgsl_model  *  init_exp_filter(cgsl_model *exp_model);

static int epce_post_step (int n, const double outputs[], void * params) {
    state_t *s = params;

    s->md.x    = outputs[0];
    s->md.logx = outputs[1];

    return GSL_SUCCESS;
}

static void exp_init(state_t *s) {
    cgsl_model *exp_model  = init_exp_model( s->md.x0 );
    cgsl_model *exp_filter = init_exp_filter( exp_model );
    cgsl_model *epce_model = cgsl_epce_model_init( exp_model, exp_filter, s->md.filter_length, epce_post_step, s );

    s->simulation = cgsl_init_simulation(  epce_model,
        rkf45,
        1e-6,
        0,
        0,
        0,
        NULL
    );
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {
    cgsl_step_to( &s->simulation, currentCommunicationPoint, communicationStepSize );
}

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"
