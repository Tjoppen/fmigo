#include "gsl-interface.h"
#include <string.h>

#ifdef WIN32
//http://stackoverflow.com/questions/6809275/unresolved-external-symbol-hypot-when-using-static-library#10051898
double hypot(double x, double y) {return _hypot(x, y);}
#endif

const gsl_odeiv2_step_type * FMIGo_get_integrator( int  i ) {

  const gsl_odeiv2_step_type * integrators [] =
    {
      gsl_odeiv2_step_rk2,	/* 0 */
      gsl_odeiv2_step_rk4,	/* 1 */
      gsl_odeiv2_step_rkf45,	/* 2 */
      gsl_odeiv2_step_rkck,	/* 3 */
      gsl_odeiv2_step_rk8pd,	/* 4 */
      gsl_odeiv2_step_rk1imp,	/* 5 */
      gsl_odeiv2_step_rk2imp,	/* 6 */
      gsl_odeiv2_step_rk4imp,	/* 7 */
      gsl_odeiv2_step_bsimp,	/* 8 */
      gsl_odeiv2_step_msadams,	/* 9 */
      gsl_odeiv2_step_msbdf	/*10 */
    };

  return integrators [ i ];

}


/**
 * Integrate the equations by one full step.
 * Used by FMIGo_step_to() only, can't be external because we need comm_step for
 * being able to apply the ECPE filters (among other things).
 */
static int FMIGo_step ( void * _s  ) {

  FMIGo_simulation * s = ( FMIGo_simulation * ) _s ;
  int status;
  if ( s->fixed_step ) {
    status = gsl_odeiv2_evolve_apply_fixed_step(s->i.evolution, s->i.control, s->i.step, &s->i.system, &s->t, s->h, s->model->x);
  } else {
    status = gsl_odeiv2_evolve_apply           (s->i.evolution, s->i.control, s->i.step, &s->i.system, &s->t, s->t1, &s->h, s->model->x);
  }

  return status;

}

/**
 * Integrate the equations up to a given end point.
 */
int FMIGo_step_to(void * _s,  double comm_point, double comm_step ) {

  FMIGo_simulation * s = ( FMIGo_simulation * ) _s;

  int i;

  s->t  = comm_point;		/* start point */
  s->t1 = comm_point + comm_step; /* end point */
  s->iterations = 0;

  if (s->model->pre_step) {
    s->model->pre_step(comm_point, comm_step, s->model->x, s->model->parameters);
  }

  ///
  /// GSL integrators return after each successful step hence the while
  /// loop.
  ///
  while ( s->t < s->t1)
  {
    int status =  FMIGo_step( s );
    s->iterations++;

    // integreate  all variables

    /// Diagnostics and printing below this point
    if (status != GSL_SUCCESS ){
      fprintf(stderr, "GSL integrator: bad status: %d \n", status);
      exit(-1);
    }
    if ( s->print  && s->file ) {
      fprintf (s->file, "%.5e ", s->t );
      for ( i = 0; i < s->i.system.dimension; ++i ){
	fprintf (s->file, "%.5e ", s->model->x[ i ]);
      }
      fprintf (s->file, "\n");
    }

    if ( s->save ) {
      FMIGo_save_data( s );
    }

  }

  if (s->model->post_step) {
    s->model->post_step(comm_point, comm_step, s->model->x, s->model->parameters);
  }

  return 0;

}

/**
 * Note: we use pass-by-value semantics for all structs anywhere possible.
 */
FMIGo_simulation FMIGo_init_simulation(
  void *model, /** the model we work on */
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
  sim.sim                    = FMIGo_get_simulation(lib);
  sim.sim                    = sim.init_simulation(model,integrator,h,fixed_step,save,print,f);

  return sim;

}

void FMIGo_model_default_free(FMIGo_simulation& sim) {
    sim.default_free(sim.model);
}

FMIGo_model* FMIGo_model_default_alloc(FMIGo_simulation &sim, int n_variables, const double *x0, void *parameters,
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
    sim->save_data(sim.sim);
    return;
}

void FMIGo_simulation_set_fixed_step( FMIGo_simulation * s, double h){
    sim->set_fixed_step(s->sim,h);
    return;
}

void FMIGo_simulation_set_variable_step( FMIGo_simulation * s ) {
    switch (s->lib){
    case gsl:{
        cgsl_simulation_set_variable_step(s);
        break;
    }
    case sundial:{
        fprintf(stderr,"csundial_simulation_set_variable_step(s) not implemented\n");
        exit(1);
        break;
    }
    }
  return;
}

  return;
}

void * FMIGo_epce_default_model_init(
        void *m,
        int filter_length,
        epce_post_step_ptr epce_post_step,
        void *epce_post_step_params) {

    return cgsl_epce_default_model_init((cgsl_model*)m, filter_length, epce_post_step, epce_post_step_params);
}
