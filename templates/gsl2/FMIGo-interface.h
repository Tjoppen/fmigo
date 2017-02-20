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
typedef struct FMIGo_integrator{

  const gsl_odeiv2_step_type * step_type; /* time step allocation */
  gsl_odeiv2_step      * step;		  /* time step control */
  gsl_odeiv2_control   * control;	  /* definition of the system */
  gsl_odeiv2_system      system;	  /* definition of the driver */
  gsl_odeiv2_evolve    * evolution;	  /* emove forward in time*/
  gsl_odeiv2_driver    * driver;	  /* high level interface */

} FMIGo_integrator;


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

typedef struct FMIGo_model{
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
  void (* get_state) (struct FMIGo_model *model);
  void (* set_state) (struct FMIGo_model *model);

  /** Destructor */
  void (* free) (struct FMIGo_model * model);
} FMIGo_model;

/**
 * Useful function for allocating the most common type of model
 */
FMIGo_model* FMIGo_model_default_alloc(
        int n_variables,            /** Number of variables */
        const double *x0,           /** Initial values. If NULL, initialize model->x to all zeroes instead */
        void *parameters,           /** User pointer */
        ode_function_ptr function,  /** ODE function */
        ode_jacobian_ptr jacobian,  /** Jacobian */
        pre_post_step_ptr pre_step, /** Pre-step function */
        pre_post_step_ptr post_step,/** Post-step function */
        size_t sz                   /** If sz > sizeof(FMIGo_model) then allocate sz bytes instead.
                                        Useful for the my_model case described earlier in this file */
);


/** Default destructor. Frees model->x and the model itself */
void FMIGo_model_default_free(FMIGo_model *model);

/**
 * Finally we can put everything in a bag.  The spefic model only has to
 * fill in these fields.
 */
typedef struct FMIGo_simulation {

    void* model;
    void* sim;
    int lib;
    int integrator;
    void (*save_data)(void*);
    void (*set_fixed_step)(void *,double);
    void (*free_simulation)(void *);
    static void (*default_free)(void*);


} FMIGo_simulation;

/*****************************
 * Enum  definitions
 *****************************
 */

/**
 * List of available time integration methods in the gsl_odeiv2 library
 */
enum FMIGo_lib_ids
{
  gsl,                          /* 0 */
  sundial,                      /* 1 */
};

/**************
 * Function declarations.
 **************
 */

/**
 * Essentially the constructor for the FMIGo_simulation object.
 * The dynamical model is defined by a function, its Jacobian, an
 * opaque parameter struct, and a set of initial values.
 * All variables are assumed to be continuous.
 *
 *  \TODO: fix semantics for saving.  Use a filename
 *   instead of descriptor?  Open and close file automatically?
 */
FMIGo_simulation FMIGo_init_simulation(
  FMIGo_model * model, /** the model we work on */
        enum FMIGo_integrator_ids integrator, /** Integrator ID   */
        double h,                    /** Initial time-step: must be non-zero, even  with variable step*/
        int fixed_step,		     /** Boolean */
        int save,		     /** Boolean */
        int print,		     /** Boolean  */
        FILE *f		             /** File descriptor if 'save' is enabled  */
  );

/**
 *  Memory deallocation.
 */
void  FMIGo_free_simulation( FMIGo_simulation sim );

/**  Step from current time to next communication point. */
int FMIGo_step_to(void * _s,  double comm_point, double comm_step ) ;

/** Accessor */
const gsl_odeiv2_step_type * FMIGo_get_integrator( int  i ) ;

/**  Commit to file. */
void FMIGo_save_data( struct FMIGo_simulation * sim );


/** Mutators for fixed step*/
/** \TODO: make this a toggle */
void FMIGo_simulation_set_fixed_step( FMIGo_simulation * s, double h );
void FMIGo_simulation_set_variable_step( FMIGo_simulation * s );

/** Get/set FMU state */
void FMIGo_simulation_get( FMIGo_simulation *s );
void FMIGo_simulation_set( FMIGo_simulation *s );

typedef int (*epce_post_step_ptr) (
    int n,                          /** Number of variables */
    const double outputs[],         /** Outputs. Filtered or not, depending on filter_length */
    void * params                   /** User pointer */
);

FMIGo_model * FMIGo_epce_model_init( FMIGo_model  *m, FMIGo_model *f,
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
FMIGo_model * FMIGo_epce_default_model_init(
        FMIGo_model  *m,
        int filter_length,
        epce_post_step_ptr epce_post_step,
        void *epce_post_step_params
);

#endif
