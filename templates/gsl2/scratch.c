

typedef struct {
  cgsl_model * model;

} filtered_model;



/**
   Here, the z variables are the integrated values of the state variables x
   and v.
 */
int sho_filter(double t, double dzdt [], const double x[], void *params ){

  dzdt[ 0 ] = x[ 0 ]; 
  dzdt[ 1 ] = x[ 1 ]; 

  return GSL_SUCCESS;
  
}

/**
   Here we assume a system with the structure
   
   x'   =  f( x, u )
   y    =  g(x, u)

   And we want to compute z such that 

   z'  = y

 */


int step_filter (double t, const double x[], double dxdt[], void * params){

  /* make local variables */
  sho_params    * p  = (sho_params *)params;

  sho(t, x, dxdt, params);
  double * z    =    x + p->n_variables;
  double * dzdt = dxdt + p->n_variables;

  sho_filter(t, z, dzdt, x, params);

  return GSL_SUCCESS;

}


/** 
 * Entry point into the model being filtered: simple case has no parameters.
 * The state variables of `sho'  are the "parameters"  for the filtered
 * variables.  
 * We have
 z'  = g( x )
 and `x'  are the state variables of 
 */
typedef struct {
  sho_model  * sho;
} sho_output_params ;


typedef cgsl_model sho_output_model;


/** 
 */
cgsl_model  *  init_sho_output_model(sho_output_params pp, double  x0 []  ){

  cgsl_model      * m = (cgsl_model * ) malloc( sizeof( sho_output_model ) );
  m->parameters  = ( void * ) malloc( sizeof( sho_output_params ) );
  *( sho_output_params *) m->parameters =  pp;
  m->n_variables = 2;
  m->x = (double * ) malloc( sizeof( double ) * m->n_variables );
  /** initial conditions */
  memcpy( m->x, x0, m->n_variables * sizeof( x0[ 0 ] ) );
  
  m->function  = sho_filter;
  m->jacobian  = jac_sho_filter;
  m->pre_step  = NULL;
  m->post_step = NULL;

  return  m;

}

void  free_sho_output_model(sho_outout_model * m){

  if ( m ) { 

    if ( m->parameters )
      free ( m->parameters );
    m->parameters = ( void * ) NULL;
    
    if ( m->x )
      free ( m->x );
    m->x = ( void * ) NULL;
    
    free( m ) ;
  }
    
  return;

}


#if 0 
typedef struct cgsl_epce_model {
  cgsl_model  e_model;		/* this is what the cgsl_simulation and cgsl_integrator see */
  cgsl_model  model;		/* actual model */
  cgsl_model  outputs;	/* filtered variables */
} cgsl_epce_model;

cgsl_model * cgsl_epce_model_init( cgsl_model  m, cgsl_model o){
  
  cgsl_epce_model * model = ( cgsl_epce_model * )  malloc( sizeof( cgsl_epce_model ) );

  model->e_model->function = cgsl_epce_model_eval;
  model->e_model->jacobian = cgsl_epce_model_jacobian;
  model->model  = m;
  model->output = o;


}

static int cgsl_epce_model_eval (double t, const double y[], double dydt[], void * params){

  int status =  GSL_SUCCESS; 
  cgsl_epce_model_parameters * p = (cgsl_epce_model_parameters *) params;
  cgsl_model  * model  = p->model;
  cgsl_model  * filter = p->filter;

  p->model->function(t, y, dydt, p->model->parameters);
  
  p->filter->function(t, y, dydt + p->model->n_variables, p->filter->parameters);


  return status;
}

#endif
