#ifndef SHO_H
#define SHO_H
#include <string.h>
#include "gsl-interface.h"



/** 
 * Declaration of the simple harmonic oscillator subject to an external
 * foce.
 */


typedef struct sho_params{

/** 
    first all parameters needed for initialization so we can use brace initializers  
*/ 
  double m; 			/* mass */
  double k; 			/* spring constant */
  double d; 			/* damping constant */

  double force; 		/* externally applied force */
  
} sho_params;



int  jac_sho (double t, const double x[], double *dfdx, double dfdt[], void *params); /* Jacobian */
int  sho (double t, const double x[], double dxdt[], void * params); /* computes derivatives */


#endif


typedef cgsl_model sho_model;


/** 
 */
cgsl_model  *  init_sho_model(sho_params pp, double  x0 []  ){

  cgsl_model      * m = (cgsl_model * ) malloc( sizeof( sho_model ) );
  m->parameters  = ( void * ) malloc( sizeof( sho_params ) );
  *( sho_params *) m->parameters =  pp;
  m->n_variables = 2;
  m->x = (double * ) malloc( sizeof( double ) * m->n_variables );
  /** initial conditions */
  memcpy( m->x, x0, m->n_variables * sizeof( x0[ 0 ] ) );
  
  m->function  = sho;
  m->jacobian  = jac_sho;
  m->pre_step  = NULL;
  m->post_step = NULL;

  return  m;

}

void  free_sho_model(sho_model * m){

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

int sho (double t, const double x[], double dxdt[], void * params){

  /* make local variables */
  sho_params    * p  = (sho_params *)params;

  dxdt[ 0 ]  = x[ 1 ];
  dxdt[ 1 ] = - ( 1.0 / p->m )  * (  p->k * x[ 0 ] + p->d * x[ 1 ] );
  dxdt[ 1 ] += p->force;

  return GSL_SUCCESS;

}


int
jac_sho (double t, const double x[], double *dfdx, double dfdt[], void *params)
{
  sho_params  * p = (sho_params *)params;
  gsl_matrix_view dfdx_mat = gsl_matrix_view_array (dfdx, 2, 2);
  gsl_matrix * J = &dfdx_mat.matrix; 

  gsl_matrix_set (J, 0, 0, 0.0);
  gsl_matrix_set (J, 0, 1, 1.0);
  gsl_matrix_set (J, 1, 0, - ( 1.0 / p-> m) * p->k ) ;
  gsl_matrix_set (J, 1, 1, - ( 1.0 / p-> m) * p->d ) ;
  dfdt[0] = 0.0;		
  dfdt[1] = 0.0; /* would have a term here if the force is
		    some polynomial interpolation */

  return GSL_SUCCESS;
}

int main()
{

  sho_params p = { 2.0,			/* mass */
		   2.0,			/* spring constant */
		   0.01,		/* damping constant */
		   2			/* driving force */
  };


  FILE *file = fopen("s.m", "w+");
  cgsl_simulation  sim = cgsl_init_simulation( init_sho_model( (sho_params ){2.0, 2.0, 0.01,30.0}, (double []) {2.0, 0.0}) ,
    rkf45,
    1e-6,
    0,
    1,
    1,
    file
    );
  
  int status =  cgsl_step_to( &sim, 0, 10 );

  free_sho_model( (sho_model * ) sim.model );

  cgsl_free_simulation( &sim );

  return status;

}

/** what we want is this: 

  cgsl_simulation  sim = cgsl_init_simulation(

  cgsl_init_epcesim( init_sho_model( (sho_params ){2.0, 2.0, 0.01,30.0}, (double []) {2.0, 0.0}) ,
		     init_sho_filter( ... ) ),
 rkf45,
    1e-6,
    0,
    1,
    1,
    file
    );
    

The issue is how to get a reference to the parameter pointer of the
original model.

typedef struct filter_parameters{ 
 cgsl_model * model;
 
} filter_parameters;

*/
