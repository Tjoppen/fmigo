#ifndef FMIGO_INTERFACE_H
#define FMIGO_INTERFACE_H

#include "gsl-interface.h"
#include "sundial-interface.h"

#include <gsl/gsl_odeiv2.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_matrix.h>

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

/*****************************
 * Structure definitions
 *****************************
 */

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

typedef void* FMIGo_model;
typedef struct FMIGo_params{
    void* params;
    int n_variables;
    N_Vector y;
    N_Vector ydot;
    N_Vector dfdy;
    N_Vector dfdt;
    ode_function_ptr gsl_f;
    ode_jacobian_ptr gsl_j;
    CVRhsFn sundial_f;
    CVDlsDenseJacFn sundial_j;

}FMIGo_params;

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

/**
 * Finally we can put everything in a bag.  The spefic model only has to
 * fill in these fields.
 */
typedef struct FMIGo_simulation {
    FMIGo_model model;
    void* sim;
    enum FMIGo_lib_ids lib;
    int integrator;
} FMIGo_simulation;


/**************
 * Function declarations.
 **************
 */

/** Default destructor. Frees model->x and the model itself */
void FMIGo_model_default_free(FMIGo_simulation &sim);

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
    enum FMIGo_lib_ids lib,
    int integrator,             /** Integrator ID   */
    double h, /** Initial time-step: must be non-zero, even  with variable step*/
    int fixed_step,       /** Boolean */
    int save,             /** Boolean */
    int print,            /** Boolean  */
    FILE *f               /** File descriptor if 'save' is enabled  */
);

/**
 *  Memory deallocation.
 */
void  FMIGo_free_simulation( FMIGo_simulation sim );

/**  Step from current time to next communication point. */
int FMIGo_step_to(FMIGo_simulation &sim,  double comm_point, double comm_step ) ;

/**  Commit to file. */
void FMIGo_save_data( FMIGo_simulation &sim );


/** Mutators for fixed step*/
/** \TODO: make this a toggle */
void FMIGo_simulation_set_fixed_step( FMIGo_simulation &s, double h );
void FMIGo_simulation_set_variable_step( FMIGo_simulation &s );

/** Get/set FMU state */
void FMIGo_simulation_get( FMIGo_simulation &s );
void FMIGo_simulation_set( FMIGo_simulation &s );
#endif
