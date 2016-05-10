#ifndef GSL_INTERFACE_H
#define GSL_INTERFACE_H


#include <gsl/gsl_odeiv2.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_matrix.h>

/*****************************
 * Structure definitions
 *****************************
 */

/**
 * Encapsulation of the integrator;
*/
typedef struct cgsl_integrator{

  const gsl_odeiv2_step_type * step_type; /* time step allocation */
  gsl_odeiv2_step      * step;		  /* time step control */ 
  gsl_odeiv2_control   * control;	  /* definition of the system */ 
  gsl_odeiv2_system      system;	  /* definition of the driver */
  gsl_odeiv2_evolve    * evolution;	  /* emove forward in time*/
  gsl_odeiv2_driver    * driver;	  /* high level interface */

} cgsl_integrator;

/**
 * Finally we can put everything in a bag.  The spefic model only has to
 * fill in these fields. 
 */
typedef struct cgsl_simulation {
  FILE * file;
  cgsl_integrator i;
  int store_data;	      /* whether or not we save the data in an array */
  double * data;		/* store variables as integration proceeds */
  int buffer_size;
  int n;			/* number of time steps taken */
  double t; 		/* current time */
  double t1;		/* stop time */
  double h;		/* first stepsize and current value */
  int   fixed_step;	/* whether or not we move at fixed step */
  int save;
  int print;

  double *x;  	        /* states */
} cgsl_simulation;

/*****************************
 * Enum  definitions
 *****************************
 */

/**
 * List of available time integration methods in the gsl_odeiv2 library
 */ 
enum cgsl_integrator_ids
{
  rk2,		/* 0 */
  rk4,		/* 1 */ 
  rkf45,	/* 2 */
  rkck,		/* 3 */
  rk8pd,	/* 4 */
  rk1imp,	/* 5 */
  rk2imp,	/* 6 */
  rk4imp,	/* 7 */
  bsimp,	/* 8 */
  msadams,	/* 9 */
  msbdf		/*10 */
};

/**************
 * Function declarations.
 **************
 */

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
        FILE *f);
void  cgsl_free( cgsl_simulation sim );

int cgsl_step(void  * s ) ;     /* fixed step, autonomous systems */

int cgsl_step_to(void * _s,  double comm_point, double comm_step ) ; /* from current time to next communication point */

const gsl_odeiv2_step_type * cgsl_get_integrator( int  i ) ;

void cgsl_save_data( struct cgsl_simulation * sim );
void cgsl_flush_data( struct cgsl_simulation * sim, char * file );

/** Resize an array by copying data, deallocate previous memory.  This
 * doubles the size each time. */
int gsl_hungry_alloc( int  n, double ** x );

void cgsl_simulation_set_fixed_step( cgsl_simulation * s, double h );
void cgsl_simulation_set_variable_step( cgsl_simulation * s );

#endif
