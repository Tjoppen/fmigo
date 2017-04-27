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
    if( s->model->pre_step(comm_point, comm_step, s->model->x, s->model->parameters) == CGSL_RESTART ){
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
    s->model->post_step(comm_point, comm_step, s->model->x, s->model->parameters);
  }

  return 0;

}


void * cgsl_default_get_model_param(const cgsl_model *m){
    return m->parameters;
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
  sim.model->get_model_parameters = cgsl_default_get_model_param;
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

void cgsl_model_default_free(cgsl_model *model) {
    free(model->x);
    free(model->x_backup);
    free(model);
}

static void cgsl_model_default_get_state(cgsl_model *model) {
  fprintf(stderr, "Getting states\n");
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




typedef struct cgsl_epce_model {
  cgsl_model  e_model;          /** this is what the cgsl_simulation and cgsl_integrator see */
  cgsl_model  *model;           /** actual model */
  cgsl_model  *filter;          /** filtered variables */

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

} cgsl_epce_model;

static int cgsl_epce_model_eval (double t, const double y[], double dydt[], void * params){

  int x;
  int status =  GSL_SUCCESS;
  cgsl_epce_model * p = (cgsl_epce_model *) params;

    p->model->function (t, y, dydt, p->model->parameters);
    //the semantics of filters is that they have read-only access to the model's
    //variables, from which it computes its derivatives

  p->filter->function(t, y, dydt + p->model->n_variables, p);

  return status;
}

static int cgsl_epce_model_jacobian (double t, const double y[], double * dfdy, double dfdt[], void * params){

  int i, j;
  cgsl_epce_model  * p = (cgsl_epce_model *)params;

  double *dfdy_f = dfdy + p->model->n_variables*p->model->n_variables;

  gsl_matrix_view dfdy_mat_e = gsl_matrix_view_array (dfdy,   p->e_model.n_variables, p->e_model.n_variables);
  gsl_matrix_view dfdy_mat_m = gsl_matrix_view_array (dfdy,   p->model->n_variables,   p->model->n_variables);
  gsl_matrix_view dfdy_mat_f = gsl_matrix_view_array (dfdy_f, p->filter->n_variables,  p->model->n_variables);    /** non-square */

  //grab Jacobians and time derivatives
  //the Jacobian will have to be rearranged since its values will end up smushed together at the start of dfdy
  //the time derivatives on the other hand will already be in the proper order
  p->model->jacobian (t, y, dfdy,   dfdt,                         p->model->parameters);
  p->filter->jacobian(t, y, dfdy_f, dfdt + p->model->n_variables, p->filter->parameters);

  //rearrange entries. start with the filter entries
  for (i = p->filter->n_variables-1; i >= 0; i--) {
    for (j = p->model->n_variables-1; j >= 0; j--) {
      gsl_matrix_set(&dfdy_mat_e.matrix, i + p->model->n_variables, j, gsl_matrix_get(&dfdy_mat_f.matrix, i, j));
    }
  }

  //now the model itself
  for (i = p->model->n_variables-1; i >= 0; i--) {
    for (j = p->model->n_variables-1; j >= 0; j--) {
      gsl_matrix_set(&dfdy_mat_e.matrix, i, j, gsl_matrix_get(&dfdy_mat_m.matrix, i, j));
    }
  }

  //finally, zero fill the entire right side of the matrix
  for (i = 0; i < p->e_model.n_variables; i++) {
    for (j = p->model->n_variables; j < p->e_model.n_variables; j++) {
      gsl_matrix_set(&dfdy_mat_e.matrix, i, j, 0);
    }
  }

  return GSL_SUCCESS;

}

static int cgsl_epce_model_pre_step  (double t, double dt, const double y[], void * params){

  cgsl_epce_model  * p = (cgsl_epce_model *)params;

  p->dt = dt;
  int x; 
  //memorize timestep and z values
  //TODO: circular buffer
  if ( p->filter_length != 0 ){ 
    memset( p->e_model.x + p->model->n_variables, 0,   p->filter->n_variables * sizeof(p->z_prev[0]) );
  }

  if (p->model->pre_step) {
    p->model->pre_step(t, dt, y, p->model->parameters);
  }

  if (p->filter->pre_step) {
    p->filter->pre_step(t, dt, y + p->model->n_variables, p->filter->parameters);
  }

  return CGSL_RESTART;

}


static int cgsl_epce_model_post_step (double t, double dt, const double y[], void * params){

  cgsl_epce_model  * p = (cgsl_epce_model *)params;

  if (p->model->post_step) {
  }

  if (p->filter->post_step) {
    p->filter->post_step(t, dt, y + p->model->n_variables, p->filter->parameters);
  }

  if (p->epce_post_step) {
    int x;
    double diff = 0;
    if (p->filter_length == 2) {
      //y = (z + z_prev) / 2.0 / dt )   : time step accounted for already
      //TODO: circular buffer
      // this will use the filtered variables to generate the ouputs. 
      //p->epce_post_step(t, y, p->epce_post_step_params);
      
      p->epce_post_step(p->filter->n_variables, p->filtered_outputs,       p->epce_post_step_params);

    } else if (p->filter_length == 1) {
        //y = z
        p->epce_post_step(p->filter->n_variables, &y[p->model->n_variables], p->epce_post_step_params);
    } else {
        //y = g(x)
      p->epce_post_step(t, y, p->epce_post_step_params);
    }
  }

  return GSL_SUCCESS;

}

static void cgsl_epce_model_free(cgsl_model *m) {
    cgsl_epce_model *model = (cgsl_epce_model*)m;

    if (model->model->free) {
        model->model->free(model->model);
    }

    if (model->filter->free) {
        model->filter->free(model->filter);
    }

    free(model->z_prev);
    free(model->z_prev_backup);
    free(model->outputs);
    free(model->filtered_outputs);
    cgsl_model_default_free(m);
}

static void cgsl_epce_model_get_state(cgsl_model *model) {
  cgsl_epce_model *m = (cgsl_epce_model*)model;

  if (m->model->get_state) {
    m->model->get_state(m->model);
  } else {
    cgsl_model_default_get_state(model);
  }

  if (m->filter->get_state) {
    m->filter->get_state(m->filter);
  }

  memcpy(m->z_prev_backup, m->z_prev, m->filter->n_variables * sizeof(m->z_prev[0]));
}

static void cgsl_epce_model_set_state(cgsl_model *model) {
  cgsl_epce_model *m = (cgsl_epce_model*)model;

  if (m->model->set_state) {
    m->model->set_state(m->model);
  } else { 
    cgsl_model_default_set_state(model);
  }

  if (m->filter->set_state) {
    m->filter->set_state(m->filter);
  }

  memcpy(m->z_prev, m->z_prev_backup, m->filter->n_variables * sizeof(m->z_prev[0]));
}

void * cgsl_epce_get_model_param(const cgsl_model *m){
    return ((cgsl_epce_model*)m)->model->parameters;
}

cgsl_model * cgsl_epce_model_init( cgsl_model  *m, cgsl_model *f,
        int filter_length,
        epce_post_step_ptr epce_post_step,
        void *epce_post_step_params){

  cgsl_epce_model *model = (cgsl_epce_model*)cgsl_model_default_alloc(
          m->n_variables + f->n_variables,
          NULL,
          NULL,
          cgsl_epce_model_eval,
          cgsl_epce_model_jacobian,
          cgsl_epce_model_pre_step,
          cgsl_epce_model_post_step,
          sizeof( cgsl_epce_model )
  );

  model->e_model.parameters     = model;
  model->model                  = m;
  model->filter                 = f;
  model->z_prev                 = (double*)calloc(f->n_variables, sizeof(double));
  model->z_prev_backup          = (double*)calloc(f->n_variables, sizeof(double));
  model->e_model.free           = cgsl_epce_model_free;
  model->e_model.get_state      = cgsl_epce_model_get_state;
  model->e_model.set_state      = cgsl_epce_model_set_state;
  model->e_model.get_model_parameters = cgsl_epce_get_model_param;
  model->filter_length          = filter_length;
  model->epce_post_step         = epce_post_step;
  model->epce_post_step_params  = epce_post_step_params;
  model->outputs                = (double*)calloc(f->n_variables, sizeof(double));
  model->filtered_outputs       = (double*)calloc(f->n_variables, sizeof(double));

  //copy initial values
  memcpy(model->e_model.x,                  m->x, m->n_variables * sizeof(m->x[0]));
  memcpy(model->e_model.x + m->n_variables, f->x, f->n_variables * sizeof(f->x[0])); /** should this not simply be initialized to zero */
  memcpy(model->z_prev,                     f->x, f->n_variables * sizeof(f->x[0]));


  return (cgsl_model*)model;
}

/*
 * the logic is easier if we integrate  dot_z = y/H 
 */
static int cgsl_automatic_filter_function (double t, const double y[], double *dydt, void * params) {

    cgsl_epce_model * p = (cgsl_epce_model *) params;
    int x;
    double idt = 1.0 / p->dt;
    
    for( x = 0; x < p->filter->n_variables; ++x ){
      dydt[ x ] = y[ x ]  * idt;
    }

    return GSL_SUCCESS;
}

static int cgsl_automatic_filter_jacobian (double t, const double y[], double * dfdy, double dfdt[], void * params) {

    cgsl_model *m = (cgsl_model*)params;
    int x;

    gsl_matrix_view dfdy_mat = gsl_matrix_view_array (dfdy, m->n_variables, m->n_variables);
    gsl_matrix_set_identity(&dfdy_mat.matrix);

    for (x = 0; x < m->n_variables; x++) {
        dfdt[x] = 0.0;
    }

    return GSL_SUCCESS;
}

cgsl_model * cgsl_epce_default_model_init(
        cgsl_model  *m,
        int filter_length,
        epce_post_step_ptr epce_post_step,
        void *epce_post_step_params) {

    cgsl_model *f = cgsl_model_default_alloc(m->n_variables, NULL, m,
        cgsl_automatic_filter_function, cgsl_automatic_filter_jacobian,
        NULL, NULL, 0
    );

    return cgsl_epce_model_init(m, f, filter_length, epce_post_step, epce_post_step_params);
}
