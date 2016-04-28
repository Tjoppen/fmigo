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

int cgsl_step ( void * _s  ) {

  cgsl_simulation * s = ( cgsl_simulation * ) _s ;
  int status;
  if ( s->fixed_step ) {
    status = gsl_odeiv2_evolve_apply_fixed_step(s->i.evolution, s->i.control, s->i.step, &s->i.system, &s->t, s->h, s->x);
  } else {
    status = gsl_odeiv2_evolve_apply           (s->i.evolution, s->i.control, s->i.step, &s->i.system, &s->t, s->t1, &s->h, s->x);
  }
  return status;
}


int cgsl_step_to(void * _s,  double comm_point, double comm_step ) {

  cgsl_simulation * s = ( cgsl_simulation * ) _s;

  int i; 

  s->t  = comm_point;
  s->t1 = comm_point + comm_step;

  
  while ( s->t < s->t1)
  {
    int status =  cgsl_step( s );
      if (status != GSL_SUCCESS ){
	fprintf(stderr, "bad status: %d \n", status);
	exit(-1);
      }
      if ( s->print  && s->file ) { 
	fprintf (s->file, "%.5e ", s->t );
	for ( i = 0; i < s->i.system.dimension; ++i ){
	  fprintf (s->file, "%.5e ", s->x[ i ]); 
	}
	fprintf (s->file, "\n");
      }
      if ( s->save ) {
	cgsl_save_data( s );
      }
    }
    return 0;

}

cgsl_simulation cgsl_init_simulation(
        int nvars,
        const double *initial_values,
        void *params,
        int (* function) (double t, const double y[], double dydt[], void * params),
        int (* jacobian) (double t, const double y[], double * dfdy, double dfdt[], void * params),
        enum cgsl_integrator_ids integrator,
        double h,           //must be non-zero, even with variable step
        int fixed_step,     //if non-zero, use a fixed step of h
        int save,
        int print,
        FILE *f)
{
    cgsl_simulation sim;

    sim.file = f;
    sim.i.step_type =  cgsl_get_integrator( integrator );
    sim.i.control = gsl_odeiv2_control_y_new (1e-6, 0.0);
    sim.i.system.function = function;
    sim.i.system.jacobian = jacobian;
    sim.i.system.dimension = nvars;
    sim.i.system.params = params;
    sim.i.evolution = gsl_odeiv2_evolve_alloc ( nvars );
    sim.i.step = gsl_odeiv2_step_alloc (sim.i.step_type, nvars);
    sim.i.driver =  gsl_odeiv2_driver_alloc_y_new (&sim.i.system, sim.i.step_type, 1e-6, 1e-6, 0.0);
    gsl_odeiv2_step_set_driver (sim.i.step, sim.i.driver);
    sim.store_data = 0;
    sim.data = (double * ) NULL;
    sim.buffer_size = 0;
    sim.n = 0;
    sim.t  = 0.0;
    sim.t1 = 0.0; 
    sim.h  = h;
    sim.fixed_step = fixed_step;
    sim.save = save;
    sim.print = print;
    sim.x = (double * ) malloc( sizeof( double ) * nvars );
    memcpy(sim.x, initial_values, sizeof(double)*nvars);

    return sim;
}

void  cgsl_free( struct cgsl_simulation * sim ) {
    gsl_odeiv2_driver_free  (sim->i.driver);
    gsl_odeiv2_step_free    (sim->i.step);
    gsl_odeiv2_evolve_free  (sim->i.evolution);
    gsl_odeiv2_control_free (sim->i.control);
    free(sim->x);
    if (sim->data) {
        free(sim->data);
    }
    if (sim->file) {
        fclose(sim->file);
    }
}

/** Resize an array by copying data, deallocate previous memory.  Return
 * new size */
int gsl_hungry_alloc( int  n, double ** x ){
  double * old = *x;
  int N = 2 * n  + 10; 

  *x = ( double * ) malloc ( N * sizeof( double ) ); 
  memcpy( *x, old, n * sizeof( double ) );
  free( old ); 

  return N;
  
}

/**
 *  Do something intelligent here.
 */
void cgsl_flush_data( struct cgsl_simulation * sim, char * file ){
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
