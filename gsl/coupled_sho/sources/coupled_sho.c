#include "modelDescription.h"
#include "gsl-interface.h"

#define SIMULATION_TYPE cgsl_simulation
#define SIMULATION_INIT coupled_sho_init
#define SIMULATION_FREE cgsl_free

#include "fmuTemplate.h"

/*  The simple forced harmonic oscillator with an external coupling: 

    x'' + zeta * x'  + x = - mu * omega2 * ( dx ) - mu * omega * zeta_c * ( x' - x0' )
    dx' = x' - x0';
    
    dx is the estimated angle difference. 
    
    Also, 
    omega2 = omega * omega;

 */

static int coupled_sho (double t, const double x[], double dxdt[], void * params){

  modelDescription_t    * p  = (modelDescription_t *)params;
  double momega2 = p->mu *p->omega * p->omega;
  double mzeta_cw = p->zeta_c * p->omega;

  /** compute the coupling force: NOTE THE SIGN!
   *  This is the force *applied* to the coupled system
   */
  p->force_c =   mzeta_cw * ( x[ 1 ] - p->v0 )  + momega2 * x[ 2 ];

  dxdt[ 0 ]  = x[ 1 ];
  /** internal dynamics */ 
  dxdt[ 1 ] = - ( x[ 0 ] - p->x0 ) - p->zeta * x[ 1 ];
  /** coupling */ 
  dxdt[ 1 ] -= p->force_c; 
  /** additional driver */ 
  dxdt[ 1 ] += p->force;

  dxdt[ 2 ] = x[ 1 ] - p->v0;

  return GSL_SUCCESS;

}



static int jac_coupled_sho (double t, const double x[], double *dfdx, double dfdt[], void *params)
{
  modelDescription_t  * p = (modelDescription_t *)params;
  double momega2 = p->mu *p->omega * p->omega;
  double mzeta_cw = p->zeta_c * p->omega;

  gsl_matrix_view dfdx_mat = gsl_matrix_view_array (dfdx, 3, 3);
  gsl_matrix * J = &dfdx_mat.matrix; 

  /** first row */
  gsl_matrix_set (J, 0, 0, 0.0); 
  gsl_matrix_set (J, 0, 1, 1.0 ); /* position/velocity */
  gsl_matrix_set (J, 0, 2, 0.0 ); 

  /** second row */
  gsl_matrix_set (J, 1, 0, -1 );
  gsl_matrix_set (J, 1, 1, - ( p->zeta + mzeta_cw ) );
  gsl_matrix_set (J, 1, 2, - momega2);

  /** third row */
  gsl_matrix_set (J, 2, 0, 0.0 );
  gsl_matrix_set (J, 2, 1, 1.0 ); /* angle difference */
  gsl_matrix_set (J, 2, 2, 0.0 ); 

  dfdt[0] = 0.0;		
  dfdt[1] = 0.0;		
  dfdt[2] = 0.0; /* would have a term here if the force is
		    some polynomial interpolation */

  return GSL_SUCCESS;
}

static void coupled_sho_init(state_t *s) {
    const double initials[3] = {s->md.x, s->md.v, s->md.dx};
    //TODO: see https://mimmi.math.umu.se/vtb/umit-fmus/issues/5
    //p->momega2 = p->mu *p->omega * p->omega;
    //p->mzeta_cw = p->zeta_c * p->omega; 
    s->simulation = cgsl_init_simulation( 3, initials, &s->md, coupled_sho, jac_coupled_sho, rkf45, 1e-5, 0, 0, 0, NULL );
}

//returns partial derivative of vr with respect to wrt
static fmi2Status getPartial(state_t *s, fmi2ValueReference vr, fmi2ValueReference wrt, fmi2Real *partial) {
    return fmi2Error;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {
    cgsl_step_to( &s->simulation, currentCommunicationPoint, communicationStepSize );
}

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"
