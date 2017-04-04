#ifndef GSL_INTERFACE_H
#define GSL_INTERFACE_H


#include <gsl/gsl_odeiv2.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_matrix.h>

#ifdef __cplusplus
extern "C" {
#endif

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


//see <gsl/gsl_odeiv2.h> for an explanation of these
typedef int (* ode_function_ptr ) (double t, const double y[], double dydt[], void * params);
typedef int (* ode_jacobian_ptr ) (double t, const double y[], double * dfdy, double dfdt[], void * params);

/** Pre/post step callbacks
 *      t: Start of the current time step (communicationPoint)
 *     dt: Length of the current time step (communicationStepSize)
 *      y: For pre, the values of the variables before stepping. For post, after.
 * params: Opaque pointer
 */
typedef int (* pre_post_step_ptr ) (double t, double dt, const double y[], void * params);

/**
 *
 *  This is intended to contain everything needed to integrate only one
 *  little bit at a time *and* to be able to backtrack when needed, and to
 *  have multiple instances of the model.
 *
 *  Here we use this for inheritance by aggregation so that each model is
 *  declared as
 *  typedef struct my_model{
 *     cgs_model   m;
 *         .
 *         .
 *     extra stuff
 *         .
 *         .
 *  }  my_model ;
*/

typedef struct cgsl_model{
  int n_variables;
  double *x;  	        /** state variables */
  double *x_backup;     /** for get/set FMU state */
  void * parameters;

  /** Definition of the dynamical system: this assumes an *explicit* ODE */
  ode_function_ptr function;
  /** Jacobian */
  ode_jacobian_ptr jacobian;

  /** Pre/post step functions */
  pre_post_step_ptr pre_step;
  pre_post_step_ptr post_step;

  /** Get/set FMU state
   * Used to copy/retreive internal state to/from temporary storage inside the model
   * params: Opaque pointer
   */
  void (* get_state) (struct cgsl_model *model);
  void (* set_state) (struct cgsl_model *model);

  /** Destructor */
  void (* free) (struct cgsl_model * model);

  /** Needed by ModelExchange to store content from parameters */
  void* (*get_model_parameters)(const struct cgsl_model *model);
} cgsl_model;

/**
 * Useful function for allocating the most common type of model
 */
cgsl_model* cgsl_model_default_alloc(
        int n_variables,            /** Number of variables */
        const double *x0,           /** Initial values. If NULL, initialize model->x to all zeroes instead */
        void *parameters,           /** User pointer */
        ode_function_ptr function,  /** ODE function */
        ode_jacobian_ptr jacobian,  /** Jacobian */
        pre_post_step_ptr pre_step, /** Pre-step function */
        pre_post_step_ptr post_step,/** Post-step function */
        size_t sz                   /** If sz > sizeof(cgsl_model) then allocate sz bytes instead.
                                        Useful for the my_model case described earlier in this file */
);


/** Default destructor. Frees model->x and the model itself */
void cgsl_model_default_free(cgsl_model *model);

/**
 * Finally we can put everything in a bag.  The spefic model only has to
 * fill in these fields.
 */
typedef struct cgsl_simulation {

  cgsl_model * model;         /** this contains the state variables */
  cgsl_integrator i;
  double t;		      /** current time */
  double t1;		      /** stop time */
  double h;		      /** first stepsize and current value */

  int   fixed_step;	      /** whether or not we move at fixed step */
  FILE * file;
  int store_data;	      /** whether or not we save the data in an array */
  double * data;              /** store variables as integration proceeds */
  int buffer_size;            /** size of data storage */
  int n;		      /** number of time steps taken */
  int save;		      /** persistence to file */
  int print;		      /** verbose on stderr */

  int iterations;             /** Number of iterations done by last call to cgsl_step_to() */
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

/**
 * Essentially the constructor for the cgsl_simulation object.
 * The dynamical model is defined by a function, its Jacobian, an
 * opaque parameter struct, and a set of initial values.
 * All variables are assumed to be continuous.
 *
 *  \TODO: fix semantics for saving.  Use a filename
 *   instead of descriptor?  Open and close file automatically?
 */
cgsl_simulation cgsl_init_simulation(
  cgsl_model * model, /** the model we work on */
        enum cgsl_integrator_ids integrator, /** Integrator ID   */
        double h,                    /** Initial time-step: must be non-zero, even  with variable step*/
        int fixed_step,		     /** Boolean */
        int save,		     /** Boolean */
        int print,		     /** Boolean  */
        FILE *f		             /** File descriptor if 'save' is enabled  */
  );

/**
 *  Memory deallocation.
 */
void  cgsl_free_simulation( cgsl_simulation sim );

/**  Step from current time to next communication point. */
int cgsl_step_to(void * _s,  double comm_point, double comm_step ) ;

/** Accessor */
const gsl_odeiv2_step_type * cgsl_get_integrator( int  i ) ;

/**  Commit to file. */
void cgsl_save_data( struct cgsl_simulation * sim );


/** Mutators for fixed step*/
/** \TODO: make this a toggle */
void cgsl_simulation_set_fixed_step( cgsl_simulation * s, double h );
void cgsl_simulation_set_variable_step( cgsl_simulation * s );

/** Get/set FMU state */
void cgsl_simulation_get( cgsl_simulation *s );
void cgsl_simulation_set( cgsl_simulation *s );

typedef int (*epce_post_step_ptr) (
    int n,                          /** Number of variables */
    const double outputs[],         /** Outputs. Filtered or not, depending on filter_length */
    void * params                   /** User pointer */
);

cgsl_model * cgsl_epce_model_init( cgsl_model  *m, cgsl_model *f,
        int filter_length,
        epce_post_step_ptr epce_post_step,
        void *epce_post_step_params
);

/**
 * Allocate an EPCE filtered model for the given model with the default filter:
 *
 *  zdot = x
 *
 * The corresponding Jacobian is the identity matrix.
 */
cgsl_model * cgsl_epce_default_model_init(
        cgsl_model  *m,
        int filter_length,
        epce_post_step_ptr epce_post_step,
        void *epce_post_step_params
);

#ifdef __cplusplus
}
#endif

#endif
