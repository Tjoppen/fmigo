#ifndef GSL_INTERFACE_H
#define GSL_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif



#include <gsl/gsl_odeiv2.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_matrix.h>

/*****************************
 * Structure definitions
 *****************************
 */

/**
 * Encapsulation of the integrator.  
 * GSL is a very verbose API which allows for a variety of 
 * use pattern.  This uses the most fundamental components and wraps them 
 * in an idiot-proof API. 
*/
typedef struct cgsl_integrator{

  const gsl_odeiv2_step_type * step_type; /* Time stepping methods*/
  gsl_odeiv2_step      * step;		  /* time step allocation */
  gsl_odeiv2_control   * control;	  /* Time step control */
  gsl_odeiv2_system      system;	  /* Definition of the system */
  gsl_odeiv2_evolve    * evolution;	  /* Driver */
  gsl_odeiv2_driver    * driver;	  /* high level interface */

} cgsl_integrator;


/**  These are the function pointers used by the integrator */ 
typedef int (* ode_function_ptr ) (double t, const double y[], double dydt[], void * params);
typedef int (* ode_jacobian_ptr ) (double t, const double y[], double * dfdy, double dfdt[], void * params);

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

struct cgsl_model;
typedef  void (* free_model) (struct cgsl_model * model);

typedef struct cgsl_model{
  int n_variables;      /** number of variables  */
  double *x;  	        /** state variables */
  void * parameters;	/** user defined parameters */

  /** Definition of the dynamical system: this assumes an explicit ODE */
  ode_function_ptr function;
  /** Jacobian */
  ode_jacobian_ptr jacobian;

  /** Destructor */
  free_model free;

} cgsl_model;

/**
 * Useful function for allocating the most common type of model
 */
cgsl_model* cgsl_model_default_alloc(
        int n_variables,            /** Number of variables */
        const double *x0,           /** Initial values. If NULL, initialize model->x to all zeroes instead */
        void *parameters,           /** User owned pointer: not allocated nor
				     * free' d by this class */
        ode_function_ptr function,  /** ODE function */
        ode_jacobian_ptr jacobian,  /** Jacobian */
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
  cgsl_integrator i;	      /** GSL data structure as defined above */
  double t;		      /** current time */
  double t_end;		      /** stop time */
  double h;		      /** current stepsize value */
  FILE * file;		      /** where to store data  */
  double * data;              /** store variables as integration proceeds */
  int buffer_size;            /** size of data storage for state backup/restore */ 
  int n;		      /** number of time steps taken */
  int print;		      /** verbose on stderr if !=0  (stderr, NOT stdout)*/
  int steps;                  /** Number of steps taken during last call to cgsl_step_to() */
  int fixed_step;	      /** Whether we are using fixed step integration.  */
} cgsl_simulation;


/*****************************
 * Enum  definitions
 *****************************
 */

/**
 * List of available time integration methods in the gsl_odeiv2 library.  
 * See GSL documentation for their meaning if unclear.  
 */ 
typedef enum cgsl_integrator_ids
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
}cgsl_integrator_ids;

/** Utility type to allow testing different integrators on a given problem */
typedef struct cgsl_integrator_named_id{ 
  int id;
  char *name;
  char *short_name;
} cgsl_integrator_named_ids ;

/** segretated lists of integrator types */
extern cgsl_integrator_named_ids cgsl_integrator_list[]; /* everyone */

extern cgsl_integrator_named_ids cgsl_integrator_explicit_list[];
extern cgsl_integrator_named_ids cgsl_integrator_implicit_list[];
extern cgsl_integrator_named_ids cgsl_integrator_rk_explicit_list[];
extern cgsl_integrator_named_ids cgsl_integrator_rk_implicit_list[];
extern cgsl_integrator_named_ids cgsl_integrator_ms_explicit_list[];
extern cgsl_integrator_named_ids cgsl_integrator_ms_implicit_list[];



/**
 *  To choose one of the step control methods. 
 *  For details, see:
 *  
 * https://www.gnu.org/software/gsl/manual/html_node/Adaptive-Step_002dsize-Control.html#Adaptive-Step_002dsize-Control
 *
 */  
typedef enum cgsl_step_control_ids
{
  step_control_first =0,
  step_control_y_new=0,			/*0: Simple control: needs absolute and relative tolerances.  */
  step_control_yp_new,                       /*1: More sophisticated control: needs relative and absolute  tolerances.  */
  step_control_standard_new,			/*2: Even more sophisticated control which can scale derivatives and states differently: 
				*  Needs relative and absolute tolerance, as well as scaling factors for states and derivatives. */
  step_control_scaled_new,     /*3: Top of the line: needs all parameters of standard_new, but also an array of scaling factors, 
				 *  one for each component. */
  step_control_fixed, 
  invalid_step_control_id
} cgsl_step_control_ids;


/** 
 *  Contains the data needed to instantiate any of the step control objects. 
 *  eps_abs
 */
typedef struct cgsl_step_control_parameters{
  double eps_abs;		/*  absolute tolerance: needed for all*/
  double eps_rel;		/* relative tolerance: needed for all */
  enum cgsl_step_control_ids id; /* control id */
  double start;			 /* optional initial step */
  double a_y;			/* scaling factor for state variables: needed for `standard_new' and `scaled_new'  */
  double a_dydt;		/* scaling factor for derivatives: needed for `standard_new' and `scaled_new'  */
  const double * scale_abs;	/* individual scaling factors: needed for scaled_new*/
  size_t dim;			/* number of variables, dimension of scale_abs array: needed for scaled_new */
} cgsl_step_control_parameters;



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
  int print,		     /** Boolean  */
  FILE *f,		             /** File descriptor if 'print' is enabled: caller responsible to open file. */
  cgsl_step_control_parameters control_parameters /* determines step control method and parameters for tolerance etc. */
  );


/** turn writing to file on or off */
void cgsl_enable_write( cgsl_simulation * sim, int x );
void cgsl_set_file( cgsl_simulation * sim, FILE * f);

/**  Step from current time to next communication point. */
int cgsl_step_to(void * _s,  double t_start, double t_ened ) ; 

/** Mutators for fixed step*/ 
/** \TODO: make this a toggle */
void cgsl_simulation_set_fixed_step( cgsl_simulation * s, double h );
void cgsl_simulation_set_variable_step( cgsl_simulation * s );

/**
 *  Memory deallocation.
 */
void  cgsl_free_simulation( cgsl_simulation sim );

/** handling of step control parameters */


/**
 * This will initialize the step control after verifying that sensible parameters have been passed 
 * and otherwise use defaults.  
 */
void cgsl_get_step_control( cgsl_step_control_parameters control_parameters, cgsl_simulation * sim );


#ifdef __cplusplus
}
#endif


#endif
