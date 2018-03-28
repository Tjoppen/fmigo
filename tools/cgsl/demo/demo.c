#include <stdio.h>
#include "hypotmath.h"
#include "gsl-interface.h"

static int sine(double t, const double y[], double dydt[], void * params) {
    //x'' = -x
    //y[0] = x
    //y[1] = x'
    dydt[0] = y[1];
    dydt[1] = -y[0];

    return GSL_SUCCESS;
}

int main(void) {
    double x0[2] = {1, 0};

    cgsl_model *m = cgsl_model_default_alloc(2, x0, NULL, sine, NULL, NULL, NULL, 0);
    cgsl_simulation sim = cgsl_init_simulation(m, rkf45, 1e-6, 0, 0, 0, NULL);

    double t = 0;
    double dt = M_PI/10;
    printf("%f,%f,%f\n", t, sim.model->x[0], sim.model->x[1]);

    for (; t < 2*M_PI; t += dt) {
        cgsl_step_to(&sim, t, dt);
        printf("%f,%f,%f\n", t+dt, sim.model->x[0], sim.model->x[1]);
    }

    cgsl_free_simulation(sim);

    return 0;
}
