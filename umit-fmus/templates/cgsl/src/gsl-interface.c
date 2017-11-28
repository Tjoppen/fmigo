#include "gsl-interface.h"
#include <string.h>

#ifdef WIN32
//This fixes linking on MSVC14
#if _MSC_VER >= 1900
static FILE iob[3];
FILE* _imp___iob() {
  iob[0] = *stdin;
  iob[1] = *stdout;
  iob[2] = *stderr;
  return iob;
}
#endif
#endif


const gsl_odeiv2_step_type * cgsl_get_integrator( int  i ) {

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
  
  if ( i < 0 || i  > sizeof( integrators ) / sizeof( integrators[ 0 ] ) ){
    fprintf(stderr, "Invalid integrator choice : %i.  Defaulting to RKF45.\n", i);
    i = 2;
  }
  return integrators [ i ];

}


/**
 * Integrate the equations by one full step.
 * Used by cgsl_step_to() only, can't be external because we need comm_step for
 * being able to apply the ECPE filters (among other things).
 */
static int cgsl_step ( void * _s  ) {

  cgsl_simulation * s = ( cgsl_simulation * ) _s ;
  int status;
  if ( s->fixed_step ) {
    status = gsl_odeiv2_evolve_apply_fixed_step(s->i.evolution, s->i.control, s->i.step, &s->i.system, &s->t, s->h, s->model->x);
  } else {
    status = gsl_odeiv2_evolve_apply           (s->i.evolution, s->i.control, s->i.step, &s->i.system, &s->t, s->t1, &s->h, s->model->x);
  }

  return status;

}

static void cgsl_print_data( cgsl_simulation * s ){
  if ( s->print  && s->file ) {
    int i;
    fprintf (s->file, "%.5e", s->t );
    for ( i = 0; i < s->i.system.dimension; ++i ){
      fprintf (s->file, " %.5e", s->model->x[ i ]);
    }
    fprintf (s->file, "\n");
  }
}

/**
 * Integrate the equations up to a given end point.
 */
int cgsl_step_to(void * _s,  double comm_point, double comm_step ) {

  cgsl_simulation * s = ( cgsl_simulation * ) _s;
  int i;

  s->t  = comm_point;		/* start point */
  s->t1 = comm_point + comm_step; /* end point */
  s->iterations = 0;

  if (s->model->pre_step) {
    if( s->model->pre_step(comm_point, comm_step, s->model->n_variables, s->model->x, s->model->parameters) == CGSL_RESTART ){
      gsl_odeiv2_evolve_reset( s->i.evolution);
      gsl_odeiv2_step_reset( s->i.step);
    } 
  }
  
  // make sure to catch the initial conditions
  if ( s->print  && s->file ) {
    cgsl_print_data( s );
  }
  if ( s->save ) {
    cgsl_save_data( s );
  }
  ///
  /// GSL integrators return after each successful step hence the while
  /// loop.
  ///
  while ( s->t < s->t1)
  {
    int status =  cgsl_step( s );
    s->iterations++;

    // integreate  all variables

    /// Diagnostics and printing below this point
    if (status != GSL_SUCCESS ){
      fprintf(stderr, "GSL integrator: bad status: %d \n", status);
      exit(-1);
    }
    if ( s->print  && s->file ) {
      cgsl_print_data( s );
    }
    if ( s->save ) {
      cgsl_save_data( s );
    }

  }

  if (s->model->post_step) {
    s->model->post_step(comm_point, comm_step, s->model->n_variables, s->model->x, s->model->parameters);
  }

  return 0;

}


void * cgsl_default_get_model_param(const cgsl_model *m){
    return m->parameters;
}


/**
 * Note: we use pass-by-value semantics for all structs anywhere possible.
 */
cgsl_simulation cgsl_init_simulation_tolerance(
  cgsl_model * model, /** the model we work on */
  enum cgsl_integrator_ids integrator,
  double h,           //must be non-zero, even with variable step
  int fixed_step,     //if non-zero, use a fixed step of h
  int save,
  int print,
  FILE *f,
  double reltol,
  double abstol)
{

  cgsl_simulation sim;

  sim.model                  = model;
  sim.model->get_model_parameters = cgsl_default_get_model_param;
  sim.i.step_type            =  cgsl_get_integrator( integrator );
  sim.i.control              = gsl_odeiv2_control_y_new (reltol, abstol); /** \TODO: IN-TEXT-DEFAULT  */

  /** pass model definition to gsl integration structure */
  sim.i.system.function      = model->function;
  sim.i.system.jacobian      = model->jacobian;
  sim.i.system.dimension     = model->n_variables;
  sim.i.system.params        = ( void * ) model->parameters;

  sim.i.evolution            = gsl_odeiv2_evolve_alloc ( model->n_variables );
  sim.i.step                 = gsl_odeiv2_step_alloc (sim.i.step_type, model->n_variables);
  sim.i.driver               =  gsl_odeiv2_driver_alloc_y_new (&sim.i.system, sim.i.step_type, 1e-6, 1e-6, 0.0); /** \TODO: IN-TEXT-DEFAULT */
  gsl_odeiv2_step_set_driver (sim.i.step, sim.i.driver);
  sim.n                      = 0;
  sim.t                      = 0.0;
  sim.t1                     = 0.0;
  sim.h                      = h;
  sim.fixed_step             = fixed_step;

  sim.store_data             = 0;
  sim.data                   = ( double * ) NULL;
  sim.buffer_size            = 0;
  sim.file                   = f;
  sim.save                   = save;
  sim.print                  = print;

#if 0 
  ///CL: this should be part of the FMU logic.  Using a POST callback
  //before anything happens is silly.
  //call post_step() so the initial values get synced outside the FMU
  if (sim.model->post_step) {
      sim.model->post_step(sim.t, sim.h, sim.model->x, sim.model->parameters);
  }
#endif
  /// this will print initial conditions
  cgsl_print_data(&sim);
  return sim;

}


/**
 * Note: we use pass-by-value semantics for all structs anywhere possible.
 */
cgsl_simulation cgsl_init_simulation(
  cgsl_model * model, /** the model we work on */
  enum cgsl_integrator_ids integrator,
  double h,           //must be non-zero, even with variable step
  int fixed_step,     //if non-zero, use a fixed step of h
  int save,
  int print,
  FILE *f)
  {
    return cgsl_init_simulation_tolerance(model, integrator, h, fixed_step, save, print, f, 1e-6, 0.0);
  }

void cgsl_model_default_free(cgsl_model *model) {
    free(model->x);
    free(model->x_backup);
    free(model);
}

static void cgsl_model_default_get_state(cgsl_model *model) {
  memcpy(model->x_backup, model->x, model->n_variables * sizeof(model->x[0]));
}

static void cgsl_model_default_set_state(cgsl_model *model) {
  memcpy(model->x, model->x_backup, model->n_variables * sizeof(model->x[0]));
}

cgsl_model* cgsl_model_default_alloc(int n_variables, const double *x0, void *parameters,
        ode_function_ptr function, ode_jacobian_ptr jacobian,
        pre_post_step_ptr pre_step, pre_post_step_ptr post_step, size_t sz) {

    //allocate at least the size of cgsl_model
    if ( sz < sizeof(cgsl_model) ) {
      sz = sizeof(cgsl_model);
    }

    cgsl_model * m = (cgsl_model*)calloc( 1, sz );

    m->n_variables = n_variables;
    m->x           = (double*)calloc( n_variables, sizeof(double));
    m->x_backup    = (double*)calloc( n_variables, sizeof(double));

    if (x0) {
        memcpy(m->x, x0, n_variables * sizeof(double));
    }

    m->parameters  = parameters;
    m->function    = function;
    m->jacobian    = jacobian;
    m->pre_step    = pre_step;
    m->post_step   = post_step;
    m->free        = cgsl_model_default_free;
    m->get_state   = cgsl_model_default_get_state;
    m->set_state   = cgsl_model_default_set_state;

    return m;

}

void  cgsl_free_simulation( cgsl_simulation sim ) {

  gsl_odeiv2_driver_free  (sim.i.driver);
  gsl_odeiv2_step_free    (sim.i.step);
  gsl_odeiv2_evolve_free  (sim.i.evolution);
  gsl_odeiv2_control_free (sim.i.control);

  if (sim.data) {
    free(sim.data);
  }

  if (sim.file) {
    fclose(sim.file);
  }

  if (sim.model->free) {
    sim.model->free(sim.model);
  }

  return;

}

/** Resize an array by copying data, deallocate previous memory.  Return
 * new size */
static int gsl_hungry_alloc( int  n, double ** x ){

  double * old = *x;
  int N = 2 * n  + 10;

  *x = ( double * ) malloc ( N * sizeof( double ) );
  memcpy( *x, old, n * sizeof( double ) );
  free( old );

  return N;

}

void cgsl_save_data( struct cgsl_simulation * sim ){

  if ( sim->store_data ){

    int stride =  1 + sim->i.system.dimension;
    int N =  stride * sim->n;

    while ( sim->buffer_size < ( N + stride ) ) {
      sim->buffer_size = gsl_hungry_alloc(  sim->buffer_size, &sim->data );
    }

    sim->data[ N ] = sim->t;

  }

  return;

}

void cgsl_simulation_set_fixed_step( cgsl_simulation * s, double h){

  s->h = h;
  s->fixed_step = 1;

  return;
}

void cgsl_simulation_set_variable_step( cgsl_simulation * s ) {

  s->fixed_step = 0;

  return;
}

void cgsl_simulation_get( cgsl_simulation *s ) {
  if (s->model->get_state) {
    s->model->get_state(s->model);
  }
  else {
    cgsl_model_default_get_state(s->model);
  }
  
}

void cgsl_simulation_set( cgsl_simulation *s ) {
  if (s->model->set_state) {
    s->model->set_state(s->model);
  } else {
    cgsl_model_default_set_state(s->model);
  }
}
