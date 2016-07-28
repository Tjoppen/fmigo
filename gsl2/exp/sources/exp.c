/**
 * Exponential with outputs x and ln(x):
 *
 * xdot = x
 * y    = {x, ln(x)} = zdot
 *
 * Solution:
 *
 * x = e^t
 * y = {e^t, t}
 * z = {e^t, 1/2*t^2}
 *
 * Filtered output solution:
 * 
 * [z(t)-z(t-dt)]/dt = {e^t*(e^dt - e^-dt)/(2*dt),  t - dt} =(lim dt->0) {e^t, t}
 */

#include "gsl-interface.h"
#include <math.h>

static int exp_function (double t, const double x[], double dxdt[], void * params){
    dxdt[ 0 ]  = x[ 0 ];
    return GSL_SUCCESS;
}

static int exp_jacobian (double t, const double x[], double *dfdx, double dfdt[], void *params)
{
    dfdx[0] = 1;
    dfdt[0] = 0;
    return GSL_SUCCESS;
}


static int exp_filter (double t, const double x[], double dzdt[], void * params){
    dzdt[ 0 ]  = x[ 0 ];
    dzdt[ 1 ]  = log(x[ 0 ]);
    return GSL_SUCCESS;
}

static int exp_filter_jacobian (double t, const double x[], double *dgdx, double dgdt[], void *params)
{
    dgdx[0] = 1;
    dgdx[1] = 1/x[0];
    dgdt[0] = 0;
    dgdt[1] = 0;
    return GSL_SUCCESS;
}

cgsl_model  *  init_exp_model(double x0){

    cgsl_model      * m = (cgsl_model * ) malloc( sizeof( cgsl_model ) );
    m->parameters  = NULL;
    m->n_variables = 1;
    m->x = (double * ) malloc( sizeof( double ) * m->n_variables );
    m->x[0] = x0;

    m->function  = exp_function;
    m->jacobian  = exp_jacobian;
    m->pre_step  = NULL;
    m->post_step = NULL;

    return  m;

}

cgsl_model  *  init_exp_filter(cgsl_model *exp_model){

    cgsl_model      * m = (cgsl_model * ) malloc( sizeof( cgsl_model ) );
    m->parameters  = NULL;
    m->n_variables = 2;
    m->x = (double * ) malloc( sizeof( double ) * m->n_variables );
    exp_filter(0, exp_model->x, m->x, NULL);

    m->function  = exp_filter;
    m->jacobian  = exp_filter_jacobian;
    m->pre_step  = NULL;
    m->post_step = NULL;

    return  m;

}

int main() {
    FILE *file = fopen("s.m", "w+");
    cgsl_model *exp_model  = init_exp_model( 1 );
    cgsl_model *exp_filter = init_exp_filter( exp_model );
    cgsl_model *epce_model = cgsl_epce_model_init( *exp_model, *exp_filter );

    cgsl_simulation  sim = cgsl_init_simulation(  epce_model,
    rkf45,
    1e-6,
    0,
    1,
    1,
    file
    );

    int x;
    for (x = 0; x < 6; x++) {
        cgsl_step_to( &sim, x, 1);
    }

    //free_exp_model( (exp_model * ) sim.model );
    //cgsl_free_model(&sim.model);

    cgsl_free_simulation( &sim );

    return 0;
}