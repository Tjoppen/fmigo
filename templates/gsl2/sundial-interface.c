#include "sundial-interface.h"
#include <string.h>
#include <stdlib.h>

#ifdef WIN32
//http://stackoverflow.com/questions/6809275/unresolved-external-symbol-hypot-when-using-static-library#10051898
double hypot(double x, double y) {return _hypot(x, y);}
#endif

static int check_flag(void *flagvalue, const char *funcname, int opt);


void create(csundial_simulation &sim){
    switch (sim.integrator){
    case cvode: {
        sim.ode_mem = CVodeCreate(CV_BDF, CV_NEWTON);
        if (check_flag((void *)sim.ode_mem, "CVodeCreate", 0)) exit(1);
        break;
    }
    case arkode:{
        sim.ode_mem = ARKodeCreate();     /* Create the solver memory */
        if (check_flag((void *)sim.ode_mem, "ARKodeCreate", 0)) exit(1);
    }
    }
}
void init(csundial_simulation &sim){
    int flag;
    switch (sim.integrator){
    case cvode:{
            flag = CVodeInit(sim.ode_mem, sim.model->function, sim.t, sim.model->x);
            if (check_flag(&flag, "CVodeInit", 1)) exit(1);
            break;
    }
    case arkode:{
        flag = ARKodeInit(sim.ode_mem, NULL, sim.model->function, sim.t, sim.model->x);
        if (check_flag(&flag, "ARKodeInit", 1)) exit(1);
        break;
    }
    }
}

void tolerance(csundial_simulation &sim){
    int flag;
    switch (sim.integrator){
    case cvode:{
            flag = CVodeSVtolerances(sim.ode_mem, sim.model->reltol, sim.model->abstol);
            if (check_flag(&flag, "CVodeSVtolerances", 1)) exit(1);
            break;
    }
    case arkode:{
        flag = ARKodeSStolerances(sim.ode_mem, sim.model->reltol, Ith(sim.model->abstol,0));  /* Specify tolerances */
        if (check_flag(&flag, "ARKodeSStolerances", 1)) exit(1);
        break;
    }
    }
}

void rootInit(csundial_simulation &sim){
    int flag;
    switch (sim.integrator){
    case cvode:{
            flag = CVodeRootInit(sim.ode_mem, sim.model->n_roots, sim.model->rootfinding);
            if (check_flag(&flag, "CVodeRootInit", 1)) exit(1);
            break;
    }
    case arkode:{
        break;
    }
    }
}

void CVDense(csundial_simulation &sim){
    int flag;
    switch (sim.integrator){
    case cvode:{
        flag = CVDense(sim.ode_mem, sim.model->neq);
        if (check_flag(&flag, "CVDense", 1)) exit(1);
        break;
    }
    case arkode:{
        flag = ARKDense(sim.ode_mem, sim.model->neq);   /* Specify dense linear solver */
        if (check_flag(&flag, "ARKDense", 1)) exit(1);
        break;
    }
    }
}

void setJac(csundial_simulation &sim){
    int flag;
    switch (sim.integrator){
    case cvode:{
        flag = CVDlsSetDenseJacFn(sim.ode_mem, sim.model->jacobian);
        if (check_flag(&flag, "CVDlsSetDenseJacFn", 1)) exit(1);
        break;
    }
    case arkode:{
        flag = ARKDlsSetDenseJacFn(sim.ode_mem, sim.model->jacobian);       /* Set Jacobian routine */
        if (check_flag(&flag, "ARKDlsSetDenseJacFn", 1)) exit(1);
        break;
    }
    }
}

void setUserData(csundial_simulation &sim){
    int flag;
    switch (sim.integrator){
    case cvode:{
        break;
    }
    case arkode:{
        flag = ARKodeSetUserData(sim.ode_mem, (void *) &sim.model->parameters);  /* Pass lamda to user functions */
        if (check_flag(&flag, "ARKodeSetUserData", 1)) exit(1);
        break;
    }
    }
}

void setLinear(csundial_simulation &sim){
    int flag;
    switch (sim.integrator){
    case cvode:{
        break;
    }
    case arkode:{
        flag = ARKodeSetLinear(sim.ode_mem, sim.model->linear);
        if (check_flag(&flag, "ARKodeSetLinear", 1)) exit(1);
        break;
    }
    }
}

int stepTo(csundial_simulation *sim){
    int flag;
    switch (sim->integrator){
    case cvode:{
        flag = CVode(sim->ode_mem, sim->t1, sim->model->x, &sim->t, CV_NORMAL);
        break;
    }
    case arkode:{
        flag = ARKode(sim->ode_mem, sim->t1, sim->model->x, &sim->t, ARK_NORMAL);      /* call integrator */
        break;
    }
    }
}

/* Free integrator memory */
void freeSimMem(csundial_simulation *sim){
    int flag;
    switch (sim->integrator){
    case cvode:{
        CVodeFree(&sim->ode_mem);
        break;
    }
    case arkode:{
        ARKodeFree(&sim->ode_mem);
        break;
    }
    }
}

  // if (check_flag(&flag, "ARKodeSetLinear", 1)) return 1;
const csundial_integrator csundial_ode={
    .create    = create,
    .init      = init,
    .tolerance = tolerance,
    .rootInit  = rootInit,
    .CVDense   = CVDense,
    .setJac    = setJac,
    .setUserData = setUserData,
    .setLinear = setLinear,
    .stepTo    = stepTo,
    .free    = freeSimMem,
};

const int csundial_get_roots(csundial_simulation &sim, int *roots){
      int flagr = CVodeGetRootInfo(sim.ode_mem, roots);
      if (check_flag(&flagr, "CVodeGetRootInfo", 1)) exit(1);
      return flagr;
}

/**
 * Integrate the equations up to a given end point.
 */
int csundial_step_to(void * _s,  double comm_point, double comm_step ) {

    csundial_simulation * sim = ( csundial_simulation * ) _s;

    sim->t  = comm_point;		/* start point */
    sim->t1 = comm_point + comm_step; /* end point */
    int flag = sim->i.stepTo(sim);

    int i;
    if ( sim->file && sim->save) {
#if defined(SUNDIALS_EXTENDED_PRECISION)
#define PRINT_TYPE "%.5Le "
#elif defined(SUNDIALS_DOUBLE_PRECISION)
#define PRINT_TYPE "%.5e "
#else
#define PRINT_TYPE "%.5e "
#endif
      fprintf (sim->file, PRINT_TYPE, sim->t );
      for ( i = 0; i < NV_LENGTH_S(sim->model->x); ++i )
          fprintf (sim->file, PRINT_TYPE , Ith(sim->model->x,i));
      fprintf (sim->file, "\n");
    }
    return flag;

}

/**
 * Note: we use pass-by-value semantics for all structs anywhere possible.
 */
csundial_simulation csundial_init_simulation(
    csundial_model * model, /** the model we work on */
    enum csundial_integrator_ids integrator, /** Integrator ID   */
    int save,
    FILE* f)
{
  csundial_simulation sim;
  int flag;

  sim.model                  = model;
  sim.integrator             = integrator;
  sim.i                      = csundial_ode;
  sim.t                      = 0.0;
  sim.t1                     = 0.0;
  sim.save                   = save;
  sim.file                   = f;

  sim.i.create(sim);

  /* Call CVodeInit to initialize the integrator memory and specify the
   * user's right hand side function in y'=f(t,y), the inital time T0, and
   * the initial dependent variable vector y. */
  sim.i.init(sim);

  /* /\* Call CVodeSVtolerances to specify the scalar relative tolerance */
  /*  * and vector absolute tolerances *\/ */
  sim.i.tolerance(sim);
  sim.i.setUserData(sim);

  /* Call CVodeRootInit to specify the root function g with 2 components */
  if(sim.model->rootfinding != NULL){
      sim.i.rootInit(sim);
  }

  /* Call CVDense to specify the CVDENSE dense linear solver */
  sim.i.CVDense(sim);

  sim.i.setLinear(sim);
  /* Set the Jacobian routine to Jac (user-supplied) */
  sim.i.setJac(sim);

  return sim;

}

void csundial_model_default_free(csundial_model *model) {
  N_VDestroy_Serial(model->x);
  N_VDestroy_Serial(model->abstol);
}

static void csundial_model_default_get_state(csundial_model *model) {
}

static void csundial_model_default_set_state(csundial_model *model) {
}

csundial_model* csundial_model_default_alloc(int n_variables, const N_Vector x0,
                                             void *parameters, CVRhsFn function,
                                             CVDlsDenseJacFn jacobian, CVRootFn rootfinding,
                                             pre_post_step_ptr pre_step, pre_post_step_ptr post_step, size_t sz) {

    csundial_model*m = (csundial_model*)calloc(1,sizeof(csundial_model));
    return m;

}

void  csundial_free_simulation( csundial_simulation sim ) {

  if (sim.file) {
    fclose(sim.file);
  }

  if (sim.model->free) {
    sim.model->free(sim.model);
  }

  /* Free integrator memory */
  sim.i.free(&sim);

  return;
}

/** Resize an array by copying data, deallocate previous memory.  Return
 * new size */
static int gsl_hungry_alloc( int  n, double ** x ){

  return 0;

}

void csundial_save_data( struct csundial_simulation * sim ){

  return;

}

void csundial_simulation_set_fixed_step( csundial_simulation * s, double h){

  return;
}

void csundial_simulation_set_variable_step( csundial_simulation * s ) {

  return;
}

void csundial_simulation_get( csundial_simulation *s ) {
}

void csundial_simulation_set( csundial_simulation *s ) {
}




typedef struct csundial_epce_model {
  csundial_model  e_model;          /** this is what the csundial_simulation and csundial_integrator see */
  csundial_model  *model;           /** actual model */
  csundial_model  *filter;          /** filtered variables */

  double *z_prev;               /** z values of previous step, copied in pre_step
                                 * TODO: replace with circular buffer for longer averaging
                                 */
  double *z_prev_backup;        /** for get/set FMU state */
  double dt;                    /** current timestep */

  int filter_length;            /** EPCE filter length.
                                 * 0: No filtering applied (y = g(x))
                                 * 1: Use latest z only
                                 * 2: Use average of z and z_prev
                                 */

  epce_post_step_ptr epce_post_step;
  void *epce_post_step_params;
  double *outputs;
  double *filtered_outputs;

} csundial_epce_model;


static int csundial_epce_model_eval (double t, const double y[], double dydt[], void * params){
  return 0;
}

static int csundial_epce_model_jacobian (double t, const double y[], double * dfdy, double dfdt[], void * params){

    return 0;

}

static int csundial_epce_model_pre_step  (double t, double dt, const double y[], void * params){
    return 0;
}

static int csundial_epce_model_post_step (double t, double dt, const double y[], void * params){
    return 0;

}

static void csundial_epce_model_free(csundial_model *m) {
}

static void csundial_epce_model_get_state(csundial_model *model) {
}

static void csundial_epce_model_set_state(csundial_model *model) {
}

csundial_model * csundial_epce_model_init( csundial_model  *m, csundial_model *f,
        int filter_length,
        epce_post_step_ptr epce_post_step,
        void *epce_post_step_params){
  return f;
}

static int csundial_automatic_filter_function (double t, const double y[], double dydt[], void * params) {

    return 0;
}

static int csundial_automatic_filter_jacobian (double t, const double y[], double * dfdy, double dfdt[], void * params) {

    return 0;
}

csundial_model * csundial_epce_default_model_init(
        csundial_model  *m,
        int filter_length,
        epce_post_step_ptr epce_post_step,
        void *epce_post_step_params) {
    csundial_model* f = (csundial_model*)calloc(1,sizeof(csundial_model));
    return csundial_epce_model_init(m, f, filter_length, epce_post_step, epce_post_step_params);
}

csundial_model* csundial_model_default_alloc(int n_variables, int n_roots, N_Vector x0, N_Vector abstol,
                                             realtype reltol, void *parameters,
                                             CVRhsFn function, CVDlsDenseJacFn jacobian, CVRootFn rootfinding,
                                             pre_post_step_ptr pre_step, pre_post_step_ptr post_step,
                                             size_t sz) {
    //allocate at least the size of cgsl_model
    if ( sz < sizeof(csundial_model) ) {
      sz = sizeof(csundial_model);
    }
    csundial_model *model = (csundial_model*)calloc( 1, sz );

    /* Initialize x */
    model->x = N_VNew_Serial(n_variables);
    model->neq = n_variables;
    if (check_flag((void *)model->x, "N_VNew_Serial", 0)) exit(1);
    memcpy(NV_DATA_S(model->x),NV_DATA_S(x0),n_variables);

    model->abstol = N_VNew_Serial(n_variables);
    if (check_flag((void *)model->abstol, "N_VNew_Serial", 0)) exit(1);
    /* Set the scalar relative tolerance */
    model->reltol = reltol;
    /* Set the vector absolute tolerance */
    memcpy(NV_DATA_S(model->abstol),NV_DATA_S(abstol),n_variables);

    model->function = function;
    model->jacobian = jacobian;
    model->rootfinding = rootfinding;
    model->n_roots = n_roots;
    model->free        = csundial_model_default_free;
    //model->get_state   = csundial_model_default_get_state;
    //model->set_state   = csundial_model_default_set_state;
    return model;
}

/*
 * Check function return value...
 *   opt == 0 means SUNDIALS function allocates memory so check if
 *            returned NULL pointer
 *   opt == 1 means SUNDIALS function returns a flag so check if
 *            flag >= 0
 *   opt == 2 means function allocates memory so check if returned
 *            NULL pointer
 */

static int check_flag(void *flagvalue, const char *funcname, int opt)
{
  int *errflag;

  /* Check if SUNDIALS function returned NULL pointer - no memory allocated */
  if (opt == 0 && flagvalue == NULL) {
    fprintf(stderr, "\nSUNDIALS_ERROR: %s() failed - returned NULL pointer\n\n",
	    funcname);
    return(1); }

  /* Check if flag < 0 */
  else if (opt == 1) {
    errflag = (int *) flagvalue;
    if (*errflag < 0) {
      fprintf(stderr, "\nSUNDIALS_ERROR: %s() failed with flag = %d\n\n",
	      funcname, *errflag);
      return(1); }}

  /* Check if function returned NULL pointer - no memory allocated */
  else if (opt == 2 && flagvalue == NULL) {
    fprintf(stderr, "\nMEMORY_ERROR: %s() failed - returned NULL pointer\n\n",
	    funcname);
    return(1); }

  return(0);
}
