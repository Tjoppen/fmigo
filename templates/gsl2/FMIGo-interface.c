#include "FMIGo-interface.h"
#include <string.h>
#define NOT_IMPLEMENTED(name)\
    fprintf(stderr,"FMIGo: function not implemented - "#name"\n");
/**
 * Integrate the equations up to a given end point.
 */
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

  sim.model         = model;
  sim.lib           = lib;
  switch (lib){
  case gsl:{
      cgsl_simulation * s = (cgsl_simulation*)calloc(1,sizeof(cgsl_simulation));
      *s = cgsl_init_simulation((cgsl_model*)model,
                                (cgsl_integrator_ids)integrator,
                                h,fixed_step,save,print,f);
  sim.sim = (void *)s;
      break;
  }
  case sundial:{
      csundial_simulation * s = (csundial_simulation*)calloc(1,sizeof(csundial_simulation));
      *s = csundial_init_simulation((csundial_model*)model,
                                    (csundial_integrator_ids)integrator,
                                    save,f);
  sim.sim = (void *)s;
      break;
  }
  }


  return sim;

}

void FMIGo_model_default_free(FMIGo_simulation& sim) {
    switch (sim.lib){
    case gsl:{
        cgsl_model_default_free((cgsl_model*)sim.model);
    }
    case sundial:{
    }
    }
}

static int FMIGo_function(double t, const double *y, double *ydot, void *params){
    FMIGo_params * p = (FMIGo_params*)params;
    memcpy(NV_DATA_S(p->y), y, p->n_variables);
    NV_DATA_S(p->ydot) = ydot;
    return p->sundial_f(t, p->y, p->ydot, p->params);
}

static int FMIGo_jacobian(double t, const double y[], double * dfdy, double dfdt[], void * params){
    FMIGo_params * p = (FMIGo_params*)params;
    memcpy(NV_DATA_S(p->y), y, p->n_variables);
    NV_DATA_S(p->dfdy) = dfdy;
    return p->sundial_j(t, p->y, dfdy, p->params);
}

FMIGo_model FMIGo_model_default_alloc(FMIGo_simulation &sim,
                                      int n_variables,
                                      int n_roots,
                                      const N_Vector x0,
                                      const N_Vector abstol,
                                      void *parameters,
                                      CVRhsFn function,
                                      CVDlsDenseJacFn jacobian,
                                      CVRootFn rootfinding,
                                      pre_post_step_ptr pre_step, pre_post_step_ptr post_step,
                                      size_t sz) {

    switch (sim.lib){
    case gsl:{
        FMIGo_params *p;
        p->n_variables = n_variables;
        p->params = parameters;
        p->sundial_f = function;
        p->sundial_j = jacobian;
        p->y    = N_VNew_Serial(n_variables);
        p->ydot = N_VNew_Serial(n_variables);
        //free because ther are only shells
        free(NV_DATA_S(p->y));
        free(NV_DATA_S(p->ydot));

        if(jacobian){
            p->dfdy    = N_VNew_Serial(n_variables);
            //free because ther are only shells
            free(NV_DATA_S(p->dfdy));
        }
        return cgsl_model_default_alloc(n_variables,NV_DATA_S(x0),parameters,FMIGo_function,FMIGo_jacobian,pre_step,post_step,sz);

    }
    case sundial:{
        return csundial_model_default_alloc(n_variables,x0,parameters,function,NULL,pre_step,post_step,sz);
    }
    }
    return NULL;

}

void  FMIGo_free_simulation( FMIGo_simulation &sim ) {
    switch (sim.lib){
    case gsl: {
        cgsl_free_simulation(*(cgsl_simulation*)sim.sim);
        break;
    }
    case sundial:{
        csundial_free_simulation(*(csundial_simulation*)sim.sim);
        break;
    }
    }
  return;

}

void FMIGo_save_data( FMIGo_simulation &sim ){
    switch (sim.lib){
    case gsl:{
        cgsl_save_data((cgsl_simulation*)sim.sim);
        break;
    }
    case sundial:{
        NOT_IMPLEMENTED(csundial_simulatian_save_data);
        break;
    }
    }
    return;
}

void FMIGo_simulation_set_fixed_step( FMIGo_simulation &sim, double h){
    switch (sim.lib){
    case gsl:{
        cgsl_simulation_set_fixed_step((cgsl_simulation*)sim.sim,h);
        break;
    }
    case sundial:{
        csundial_simulation_set_fixed_step((csundial_simulation*)sim.sim,h);
        break;
    }
    }
    return;
}

void FMIGo_simulation_set_variable_step( FMIGo_simulation &sim ) {
    switch (sim.lib){
    case gsl:{
        cgsl_simulation_set_variable_step((cgsl_simulation*)sim.sim);
        break;
    }
    case sundial:{
        csundial_simulation_set_variable_step((csundial_simulation*)sim.sim);
        break;
    }
    }
    return;
}
