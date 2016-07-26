#include "gsl-interface.h"
#include <string.h>


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

  return integrators [ i ];

}


/**
 * Integrate the equations by one full step.
 */
int cgsl_step ( void * _s  ) {

  cgsl_simulation * s = ( cgsl_simulation * ) _s ;
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
int cgsl_step_to(void * _s,  double comm_point, double comm_step ) {

  cgsl_simulation * s = ( cgsl_simulation * ) _s;

  int i; 

  s->t  = comm_point;		/* start point */
  s->t1 = comm_point + comm_step; /* end point */

  ///
  /// GSL integrators return after each successful step hence the while
  /// loop.
  ///
  while ( s->t < s->t1)
  {
    int status =  cgsl_step( s );

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
      cgsl_save_data( s );
    }
    
  }

  return 0;

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

  cgsl_simulation sim;

  sim.model                  = model;
  sim.i.step_type            =  cgsl_get_integrator( integrator );
  sim.i.control              = gsl_odeiv2_control_y_new (1e-6, 0.0); /** \TODO: IN-TEXT-DEFAULT  */

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

  return sim;

}

void  cgsl_free_simulation( cgsl_simulation *sim ) {

  gsl_odeiv2_driver_free  (sim->i.driver);
  gsl_odeiv2_step_free    (sim->i.step);
  gsl_odeiv2_evolve_free  (sim->i.evolution);
  gsl_odeiv2_control_free (sim->i.control);

  if (sim->data) {
    free(sim->data);
  }

  if (sim->file) {
    fclose(sim->file);
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




