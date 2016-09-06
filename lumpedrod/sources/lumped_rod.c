#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "lumped_rod.h"
#include "safealloc.h"

/** to allow simpler copy paste in store/restore functions */ 
#define COPY_FWD( a , b )  a  = b
#define COPY_BCK( a , b )  b  = a
#define MEMCP_FWD( a , b , c)  memcpy(a, b, c)
#define MEMCP_BCK( b , a , c)  memcpy(a, b, c)


/**
   This code makes explicit use of the fact that struct arguments are
   copied, so are return values.  This means that the addresses of the
   points contained in the structs remain valid, even though the addresses
   of the structs aren't. 
*/

/** WARNING:  no error checking 
 *  No initialization is done here.  
*/

void lumped_rod_alloc( lumped_rod * rod ) {
  
  assert( rod );
  assert( rod->n );
  
  rod->mass  = rod->rod_mass  / ( double ) rod->n;
  
  CALLOC( double, rod->state.x       , rod->n);
  CALLOC( double, rod->state.v       , rod->n);
  CALLOC( double, rod->state.a       , rod->n);
  CALLOC( double, rod->state.torsion, rod->n);
//  rod->state.v            = ( double * ) calloc(    rod->n    ,    sizeof( double ) );		
 // rod->state.a            = ( double * ) malloc(    rod->n    *    sizeof( double ) );		
 // rod->state.torsion      = ( double * ) malloc( ( rod->n - 1 )  * sizeof( double ) );		

  return ;

}

/** WARNING:  no error checking */
void lumped_rod_free( lumped_rod rod ){

  FREE( rod.state.x           );
  FREE( rod.state.v           );
  FREE( rod.state.a           );
  FREE( rod.state.torsion     );

  return;

}


/** WARNING:  no error checking 
    We need space for n masses and n-1 constraints.
*/
lumped_rod_sim lumped_rod_sim_alloc( int n ) {

  lumped_rod_sim sim;
  
  MALLOC( double, sim.z, 2 * n - 1 ); 

  return sim;

}


/**

   This implements the matrix for a string or a chain of particle in the
   form of a permutation of 
   [ M  -G' ]
   [ G   T  ]

   Of course, we work with 
   [ M   G' ]
   [ G  -T  ]
   which is symmetric and indefinite

   The permutation is such that we have 

   [  m  -1   0  0  0 ...
   [  1   e  -1  0  0 ...
   [  0   1   m -1  0 ...      ]
   [  0   0   1  e -1 ... 

   .
   .
   .
   [  0   0  ...   1  m ]


   I.e., every odd (even when starting from 0) row is a 'mass row'   with 

   ...  0  1  m -1  0  ... 

   and every even row (odd when starting from 0) 
   has 
   ...  0  1  e -1  0

   What this means is that we are imposing constraints

   v[ k   ]  -  v[ k + 1 ] = 0

   There are `n'  mass rows and `n-1'  constraint rows


   The parameter tau is related to damping.  With a value of 2, the
   dynamics is nearly critically damped independently of the time step.
   Higher values introduce overdamping.  

   To allow for a velocity input at either end, we introduce a stiff
   spring-damper based on the estimated integral of the difference between
   actual velocities v1 and vN and the driver v_in and v_out.

   What this does is to modify the first and last masses according to 
   m <-   m +  stiffness * h * h * ( 1 + relaxation ) / 4;

   The RHS is then modified to include   h * K * ( dx1 - h * dv / 4 );

*/

tri_matrix build_rod_matrix( lumped_rod  rod, double step) { 

  int n = rod.n;
  tri_matrix m; 		/* matrix  */
  double gamma = 1.0 /  ( 1.0 + 4.0 * rod.relaxation_rate );
  double compliance = - rod.compliance * ( 4.0  * gamma / step / step ) /  ( double ) rod.n  ; 

  double stiffness1 =  rod.driver_stiffness1 * ( 1  + 4 * rod.driver_relaxation1 ) * step * step /  4 ;
  double stiffnessN =  rod.driver_stiffnessN * ( 1  + 4 * rod.driver_relaxationN ) * step * step /  4 ;


  int i;

  
  assert( n );

  m = tri_matrix_alloc( 2 * n - 1 );


  for( i = 0; i < m.n; i+= 2 ){
    m.diag[ i ] =  rod.mass;
  }

  for( i = 1; i < m.n; i+= 2 ){
    m.diag[ i ] =   compliance;
  }
    
  /** this accounts for stiff springs coupled to drivers */
  m.diag[     0 ]   += stiffness1;
  m.diag[ m.n-1 ]   += stiffnessN;

  /** place a -1 on each row with a mass starting from second */
  for( i = 1; i < n; ++i ) {
    m.sub[   2 * i - 2   ] =     1.0;
    m.sub[   2 * i - 1   ] =   - 1.0;
  }
  

  tri_factor( &m );			/** this only has to be factored
					 *  once unless we introduce
					 *  nonlinear elasticity */

  return m;
  
}


/**
   This does almost all the work.  The one assumption is that the lumped_rod
   struct has a valid value for rod_mass, N,   and compliance.  Everything
   else is derived from that. 
*/
lumped_rod_sim lumped_rod_sim_create( lumped_rod_sim_parameters p) {
  lumped_rod_sim sim = lumped_rod_sim_alloc( p.rod.n );
  sim.state = p;
  sim.rod      =  sim.state.rod;

  if ( sim.rod.driver_sign1 >= 0 ){
    sim.rod.driver_sign1 =  1.0;
  } else {
    sim.rod.driver_sign1 = -1.0;
  }

  if ( sim.rod.driver_signN >= 0 ){
    sim.rod.driver_signN =  1.0;
  } else {
    sim.rod.driver_signN = -1.0;
  }

  lumped_rod_alloc( &sim.rod );
  
  sim.state_backup = p;

  lumped_rod_alloc( & sim.state_backup.rod ); /* for store / restore */

  sim.gamma_v =   ( double ) 1.0  /  (  1.0  +  4.0  * sim.rod.relaxation_rate );
  sim.gamma_x = - ( double ) 4.0  * sim.gamma_v   /  sim.state.step  ;

  sim.gamma_driver_v1 =   ( double ) 1.0  /  (  1.0  +  4.0  * sim.rod.driver_relaxation1 );
  sim.gamma_driver_x1 = - ( double ) 4.0  * sim.gamma_driver_v1   /  sim.state.step  ;

  sim.gamma_driver_vN =   ( double ) 1.0  /  (  1.0  +  4.0  * sim.rod.driver_relaxationN );
  sim.gamma_driver_xN = - ( double ) 4.0  * sim.gamma_driver_vN   /  sim.state.step  ;

  sim.m = build_rod_matrix( sim.rod, sim.state.step) ;
  lumped_rod_sim_mobility( &sim, sim.rod.mobility );

  lumped_rod_initialize(   sim.rod , sim.state.state.x1, sim.state.state.xN, sim.state.state.v1, sim.state.state.vN );

  
  return sim;
}



void lumped_sim_set_timestep ( lumped_rod_sim * sim, double step){

  sim->state.step = step;
  sim->gamma_v =   ( double ) 1.0  /  (  1.0  +  4.0  * sim->rod.relaxation_rate );
  sim->gamma_x = - ( double ) 4.0  * sim->gamma_v   /  sim->state.step  ;

  sim->gamma_driver_v1 =   ( double ) 1.0  /  (  1.0  +  4.0  * sim->rod.driver_relaxation1 );
  sim->gamma_driver_x1 = - ( double ) 4.0  * sim->gamma_driver_v1   /  sim->state.step  ;

  sim->gamma_driver_vN =   ( double ) 1.0  /  (  1.0  +  4.0  * sim->rod.driver_relaxationN );
  sim->gamma_driver_xN = - ( double ) 4.0  * sim->gamma_driver_vN   /  sim->state.step  ;

  sim->m = build_rod_matrix( sim->rod, sim->state.step) ;
  lumped_rod_sim_mobility( sim, sim->rod.mobility );

}




void lumped_rod_sim_free( lumped_rod_sim   sim ) {

   lumped_rod_free( sim.rod      );
  lumped_rod_free( sim.state_backup.rod );
  tri_matrix_free( sim.m );
  free ( sim.z );

  return;
  
}

void lumped_rod_initialize( lumped_rod rod, double x1, double xN, double v1, double vN){

  /** no need to initialize acceleration or torsion */
  double dx =  ( xN - x1 )  / ( double )  rod.n;
  double dv =  ( vN - v1 )  / ( double )  rod.n;
  int i; 

  for ( i = 0; i < rod.n; ++i ){
    rod.state.x[ i ]  =  x1 + ( ( double ) i ) * dx ; 
    rod.state.v[ i ]  =  v1 + ( ( double ) i ) * dv ; 
  }

}


/** This is the standard for spook
 */
void build_rod_rhs( lumped_rod_sim * sim ){

  int i;
  for ( i = 0; i < sim->rod.n - 1 ; ++i ) {

    sim->z[ 2 * i ] = sim->rod.mass * sim->rod.state.v[ i ]; 

    sim->z[ 2 * i  + 1 ] =
      sim->gamma_x * ( sim->rod.state.x[  i  ] - sim->rod.state.x[ i + 1 ] )
      + sim->gamma_v * ( sim->rod.state.v[  i  ] - sim->rod.state.v[ i + 1 ] );
  }

  sim->z[ sim->m.n  - 1 ] = sim->rod.mass * sim->rod.state.v[ sim->rod.n -1 ]; 

  /* forces at the end */
  sim->z[  0          ]  +=  sim->state.step * sim->state.state.driver_f1;
  sim->z[ sim->m.n - 1]  +=  sim->state.step * sim->state.state.driver_fN;

  /* drivers */
  sim->z[ 0 ]           += - sim->state.step * sim->rod.driver_stiffness1  *
    ( sim->state.state.dx1 - sim->state.step * sim->state.state.driver_v1 / 4.0 );

  sim->z[ sim->m.n -1 ] += - sim->state.step * sim->rod.driver_stiffnessN  *
    ( sim->state.state.dxN - sim->state.step * sim->state.state.driver_vN / 4.0 );

}

/**  
     The rod can be forced at the first and last particle.  The mobility
     here is the 2x2 matrix of reaction forces given unit impulses at the
     two signals.  

     The output matrix is assumed to be row major
*/
void lumped_rod_sim_mobility( lumped_rod_sim * sim, double  * mob ){

  /* unit force on first mass */
  memset( sim->z, 0, sim->m.n * sizeof( double ) );
  sim->z[ 0 ] = ( double ) 1.0; 
  tri_solve( &sim->m, sim->z );

  /* collect results */ 
  mob[ 0 ] = sim->z[ 0 ];
  mob[ 1 ] = sim->z[ sim->m.n - 1 ];

  /* unit force on the last mass */
  memset( sim->z, 0, sim->m.n * sizeof( double ) );
  sim->z[ sim->m.n - 1  ] = ( double ) 1.0; 
  tri_solve( &sim->m, sim->z );
  
  /* collect results */ 
  mob[ 2 ] = sim->z[ 0 ];
  mob[ 3 ] = sim->z[ sim->m.n - 1 ];

}


/**
 *   Step forward in time.  
 */
void rod_sim_do_step( lumped_rod_sim * sim, int n ){

  assert( sim );
  
  double h_inv = 1.0  / sim->state.step;
  int i, j ; 

  for ( j = 0; j < n; ++j  ){

    build_rod_rhs( sim );

    tri_solve( &sim->m, sim->z );
  
    for ( i = 0; i < sim->rod.n; ++i ){
      sim->rod.state.a[ i ]  = ( sim->z[ 2 * i ] - sim->rod.state.v[ i ] ) * h_inv; 
      sim->rod.state.v[ i ]  =   sim->z[ 2 * i ]; 
      sim->rod.state.x[ i ] +=   sim->state.step * sim->rod.state.v[ i ]; 
    }

    
    for ( i = 0; i < sim->rod.n -1 ; ++i ){
      sim->rod.state.torsion[ i ] = sim->z[ 2 * i + 1 ] * h_inv ;
    }
  }

  /** 
      Publish variables.
  */

  sim->state.state.f1 = sim->rod.state.torsion[ 0     ];
  sim->state.state.fN = sim->rod.state.torsion[ n - 1 ];
  sim->state.state.x1 = sim->rod.state.x      [ 0     ];
  sim->state.state.xN = sim->rod.state.x      [ n - 1 ];
  sim->state.state.v1 = sim->rod.state.v      [ 0     ];
  sim->state.state.vN = sim->rod.state.v      [ n - 1 ];
  sim->state.state.a1 = sim->rod.state.a      [ 0     ];
  sim->state.state.aN = sim->rod.state.a      [ n - 1 ];

  sim->state.state.dx1 += sim->state.step * ( sim->rod.state.v[ 0 ] - sim->state.state.driver_v1 );
  sim->state.state.dxN += sim->state.step * ( sim->rod.state.v[ n - 1 ] - sim->state.state.driver_vN );

}



/** set the positions at the ends */
void lumped_sim_set_velocity ( lumped_rod_sim * sim, double v, int j ){

  if ( j == 1 ) { 
    sim->rod.state.v[ sim->rod.n - 1  ] = v;
    sim->state.state.vN = v;
  }
  else {
    sim->rod.state.v[ 0 ] = v; 
    sim->state.state.v1 = v;
  }

}

/** set the positions at the ends */
void lumped_sim_set_position ( lumped_rod_sim * sim, double x, int j ){

  if ( j == 1 ) { 
    sim->rod.state.x[ sim->rod.n - 1  ] = x;
    sim->state.state.xN = x;
  }
  else {
    sim->rod.state.x[ 0 ] = x; 
    sim->state.state.x1 = x;
  }

}


/**
   We don't want to copy the ponters!  
*/ 
void   lumped_rod_copy( lumped_rod * src, lumped_rod  * dest ){

  lumped_rod_kinematics tmp  = dest->state;
  
  *dest = *src;

  int N = src->n * sizeof( double ); 

  memcpy( dest->state.x, tmp.x, N);
  memcpy( dest->state.v, tmp.v, N);
  memcpy( dest->state.a, tmp.a, N);
  memcpy( dest->state.torsion, tmp.torsion, ( src->n - 1 ) * sizeof( double ) );
  dest->state = tmp;

  return;
}

void lumped_rod_sim_store ( lumped_rod_sim  * sim ){
  lumped_rod_copy(  & sim->rod,  & sim->state_backup.rod );
  return;
}

void lumped_rod_sim_restore ( lumped_rod_sim  * sim ){
  lumped_rod_copy( & ( sim->state_backup.rod ),  & ( sim->rod ) );
  return;
}
