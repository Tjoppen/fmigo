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

//fix WIN32 build
#include "hypotmath.h"

#include "gsl-interface.h"

static int exp_function (double t, const double x[], double dxdt[], void * params){
    dxdt[ 0 ]  = x[ 0 ];
    return GSL_SUCCESS;
}

static int exp_jacobian (double t, const double x[], double *dfdx, double dfdt[], void *params)
{
    dfdx[0] = 1.0;
    dfdt[0] = 0.0;
    return GSL_SUCCESS;
}


static int exp_filter (double t, const double x[], double dzdt[], void * params){
    dzdt[ 0 ]  = x[ 0 ];
    dzdt[ 1 ]  = log(x[ 0 ]);
    return GSL_SUCCESS;
}

static int exp_filter_jacobian (double t, const double x[], double *dgdx, double dgdt[], void *params)
{
    dgdx[0] = 1.0;
    dgdx[1] = 1.0/x[0];
    dgdt[0] = 0.0;
    dgdt[1] = 0.0;
    return GSL_SUCCESS;
}

cgsl_model  *  init_exp_model(double x0){
    return cgsl_model_default_alloc(1, &x0, NULL, exp_function, exp_jacobian, NULL, NULL, 0);
}

cgsl_model  *  init_exp_filter(cgsl_model *exp_model){
    cgsl_model *m = cgsl_model_default_alloc(2, NULL, NULL, exp_filter, exp_filter_jacobian, NULL, NULL, 0);
    exp_filter(0, exp_model->x, m->x, NULL);

    return  m;
}

static int epce_post_step (
  double t, 
        int n,
        const double outputs[],
        void * params) {

    int x;

    for (x = 0; x < n; x++) {
        fprintf(stderr, "%i: %f\n", x, outputs[x]);
    }

    return GSL_SUCCESS;
}

int main() {
    FILE *file = fopen("s.m", "w+");
    //cgsl_model *exp_model  = init_exp_model( 1 );
    //cgsl_model *exp_filter = init_exp_filter( exp_model );
    //cgsl_model *epce_model = cgsl_epce_model_init( exp_model, exp_filter, epce_post_step, NULL );

    cgsl_model *epce_model = cgsl_epce_default_model_init(
            init_exp_model( 1 ),
            2,
            epce_post_step,
            NULL
    );

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

    //frees everything, including models
    cgsl_free_simulation( sim );

    return 0;
}
