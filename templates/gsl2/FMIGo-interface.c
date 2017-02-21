#include "gsl-interface.h"
#include "sundial-interface.h"
#include <string.h>

/**
 * Integrate the equations up to a given end point.
 */
int FMIGo_step_to(void *sim,  double comm_point, double comm_step ) {
    return sim.step_to(sim,comm_point,comm_step);
}

/**
 * Note: we use pass-by-value semantics for all structs anywhere possible.
 */
FMIGo_simulation FMIGo_init_simulation(
  FMIGo_model model, /** the model we work on */
  enum FMIGo_lib_ids lib,
  int integrator,
  double h,           //must be non-zero, even with variable step
  int fixed_step,     //if non-zero, use a fixed step of h
  int save,
  int print,
  FILE *f)
{

  FMIGo_simulation sim;

  sim.model                  = model;
  sim.s                      = FMIGo_get_simulation(lib);
  sim.sim                    = sim.s.init_simulation(model,integrator,h,fixed_step,save,print,f);

  return sim;

}

void FMIGo_model_default_free(FMIGo_simulation& sim) {
    sim.default_free(sim.model);
}

FMIGo_model FMIGo_model_default_alloc(FMIGo_simulation &sim, int n_variables, const double *x0, void *parameters,
        ode_function_ptr function, ode_jacobian_ptr jacobian,
        pre_post_step_ptr pre_step, pre_post_step_ptr post_step, size_t sz) {
    switch (sim->lib){
    case gsl:{
        gsl_model_default_alloc(n_variables,x0,parameters,function,Jacobian,pre_step,post_step,sz);

    }
    }
    return m;

}

void  FMIGo_free_simulation( FMIGo_simulation &sim ) {
    sim.free_simulation(sim.sim);
  return;

}

/** Resize an array by copying data, deallocate previous memory.  Return
 * new size */
static int gsl_hungry_alloc(FMIGo_simulation &sim, int  n, double ** x ){
    return sim.hungry_alloc(n,x);
}

void FMIGo_save_data( FMIGo_simulation &sim ){
    sim.save_data(sim.sim);
    return;
}

void FMIGo_simulation_set_fixed_step( FMIGo_simulation * sim, double h){
    sim->i.set_fixed_step(sim->sim,h);
    return;
}

void FMIGo_simulation_set_variable_step( FMIGo_simulation * sim ) {
    sim->i.set_variable_step(sim->sim);
    return;
}

  return;
}

FMIGo_model FMIGo_epce_default_model_init(
        FMIGo_model m,
        int filter_length,
        epce_post_step_ptr epce_post_step,
        void *epce_post_step_params) {

    return cgsl_epce_default_model_init((cgsl_model*)m, filter_length, epce_post_step, epce_post_step_params);
}
