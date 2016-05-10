#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "lumped_rod.h"

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

/** WARNING:  no error checking */

lumped_rod  lumped_rod_alloc( int n, double mass, double compliance ) {
  lumped_rod  rod;

  rod.n = n;			
  rod.rod_mass = mass;
  rod.mass = rod.rod_mass / ( double ) rod.n ;
  rod.compliance = compliance;

  rod.x            = ( double * ) calloc(    n    ,    sizeof( double ) );		
  rod.v            = ( double * ) calloc(    n    ,    sizeof( double ) );		
  rod.a            = ( double * ) calloc(    n    ,    sizeof( double ) );		
  rod.torsion      = ( double * ) malloc( ( n - 1 )  * sizeof( double ) );		

  return rod;
}

/** WARNING:  no error checking */
void lumped_rod_free( lumped_rod rod ){

  free( rod.x           );
  free( rod.v           );
  free( rod.a           );
  free( rod.torsion     );

  return;

}


/** WARNING:  no error checking */
lumped_rod_sim lumped_rod_sim_alloc( int n ) {

  lumped_rod_sim sim;
  
  sim.z       = (double * ) malloc( ( 2 * n + 1 ) * sizeof( double ) );

  return sim;

}

void lumped_rod_sim_free( lumped_rod_sim * sim ) {
  free ( sim->z );
  free ( sim );
  return; 
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
*/

tri_matrix build_rod_matrix( lumped_rod  rod, double step, double tau ) { 

  int n = rod.n;
  tri_matrix m; 		/* matrix  */
  double gamma = 1.0 /  ( 1.0 + 4.0 * tau  );
  double compliance = - ( 4.0  * gamma / step / step ) * 
    rod.compliance  / ( double ) ( rod.n - 1 );
  int i;

  
  if ( n == 0  ){ 
    fprintf(stderr, "N = 0!\n" );
    abort();
  }

  m = tri_matrix_alloc( 2 * n - 1 );


  for( i = 0; i < m.n; i+= 2 ){
    m.diag[ i ] =  rod.mass;
  }

  for( i = 1; i < m.n; i+= 2 ){
    m.diag[ i ] =   compliance;
  }
    

  for( i = 0; i < n - 1; ++i ) {
    m.sub[ 2 * i     ] =   ( double ) 1.0;
    m.sub[ 2 * i + 1 ] = - ( double ) 1.0;
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
  lumped_rod_sim sim = lumped_rod_sim_alloc( p.N );
  sim.rod      = lumped_rod_alloc( p.N, p.mass, p.compliance ); 
  sim.rod_back = lumped_rod_alloc( p.N, p.mass, p.compliance ); /* for store / restore */
  sim.step = p.step;
  sim.tau  = p.tau;
  sim.gamma_v = ( double  ) 1.0  /  (  1.0  +  4.0  * sim.tau );
  sim.gamma_x = - (double ) 4.0  * sim.gamma_v   /  sim.step  ;

  sim.m = build_rod_matrix( sim.rod, sim.step, sim.tau ) ;
  lumped_rod_sim_mobility( &sim, sim.rod.mobility );
  sim.forces[ 0 ] = p.f1; 
  sim.forces[ 1 ] = p.fN; 
  
  lumped_rod_initialize(  & ( sim.rod ), p.x1, p.xN, p.v1, p.vN );

  lumped_rod_sim_sync_state_out( &sim );
  
  return sim;
}

void lumped_rod_sim_delete( lumped_rod_sim   sim ) {

  lumped_rod_free( sim.rod      );
  lumped_rod_free( sim.rod_back );
  tri_matrix_free( sim.m );
  free ( sim.z );

  return;
  
}

void lumped_rod_initialize( lumped_rod * rod, double x1, double xN, double v1, double vN){

  double dx =  ( xN - x1 )  / ( double )  rod->n;
  double dv =  ( vN - v1 )  / ( double )  rod->n;
  int i; 

  for ( i = 0; i < rod->n; ++i ){
    rod->x[ i ]  =  x1 + ( ( double ) i ) * dx ; 
    rod->v[ i ]  =  v1 + ( ( double ) i ) * dv ; 
    rod->a[ i ]  =  0;
  }

}


/** This is the standard for spook
 */
void build_rod_rhs( lumped_rod_sim * sim ){

  int i;
  
  for ( i = 0; i < sim->rod.n ; ++i ) {
    sim->z[ 2 * i ] = sim->rod.mass * sim->rod.v[ i ]; 
  }

  /* forces at the end */
  sim->z[  0          ] +=  sim->step * sim->forces[ 0 ]; 
  sim->z[ sim->m.n - 1 ] +=  sim->step * sim->forces[ 1 ]; 

  for ( i = 0; i <  sim->rod.n - 1 ;  ++i ) {

    sim->z[ 2 * i  + 1 ] =
      sim->gamma_x * ( sim->rod.x[  i  ] - sim->rod.x[ i + 1 ] )
      + sim->gamma_v * ( sim->rod.v[  i  ] - sim->rod.v[ i + 1 ] );

  }

}

/**  
     The rod can be forced at the first and last particle.  The mobility
     here is the 2x2 matrix of reaction forces given unit impulses at the
     two signals.  

     The output matrix is assumed to be row major
*/
void lumped_rod_sim_mobility( lumped_rod_sim * sim, double  * mob ){

  /* force on first mass */
  memset( sim->z, 0, sim->m.n * sizeof( double ) );
  sim->z[ 0 ] = ( double ) 1.0; 
  tri_solve( &sim->m, sim->z );

  /* collect results */ 
  mob[ 0 ] = sim->z[ 0 ];
  mob[ 1 ] = sim->z[ sim->m.n - 1 ];

  memset( sim->z, 0, sim->m.n * sizeof( double ) );
  sim->z[ sim->m.n - 1  ] = ( double ) 1.0; 
  tri_solve( &sim->m, sim->z );
  
  /* collect results */ 
  mob[ 2 ] = sim->z[ 0 ];
  mob[ 3 ] = sim->z[ sim->m.n - 1 ];

}

double *   lumped_rod_sim_get_state( lumped_rod_sim  * sim ){
  return ( double *) & ( sim->state ) ; 
}

void  lumped_rod_sim_sync_state_out( lumped_rod_sim * sim ){

  int last = sim->rod.n -1 ; 

  COPY_BCK(  sim->rod.x [    0  ]  ,  sim->state.x1 );
  COPY_BCK(  sim->rod.x [  last ]  , sim->state.xN  );
  COPY_BCK(  sim->rod.v [    0  ]  , sim->state.v1  );
  COPY_BCK(  sim->rod.v [  last ]  , sim->state.vN  );
  COPY_BCK(  sim->forces[    0  ]  , sim->state.f1  );
  COPY_BCK(  sim->forces[    1  ]  ,  sim->state.fN );
  return;
}


void  lumped_rod_sim_sync_state_in( lumped_rod_sim * sim ){

  int last = sim->rod.n -1 ; 

  COPY_FWD(  sim->rod.x [    0  ]  ,  sim->state.x1 );
  COPY_FWD(  sim->rod.x [  last ]  , sim->state.xN  );
  COPY_FWD(  sim->rod.v [    0  ]  , sim->state.v1  );
  COPY_FWD(  sim->rod.v [  last ]  , sim->state.vN  );
  COPY_FWD(  sim->forces[    0  ]  , sim->state.f1  );
  COPY_FWD(  sim->forces[    1  ]  ,  sim->state.fN );
  return;
}

/**
 *   Step forward in time.  
 */
void step_rod_sim( lumped_rod_sim * sim, int n ){
  
  double h_inv = 1.0  / sim->step;
  lumped_rod_sim_sync_state_in(  sim );
  int i, j ; 
  for ( j = 0; j < n; ++j  ){

    build_rod_rhs( sim );
    tri_solve( &sim->m, sim->z );
  
    for ( i = 0; i < sim->rod.n; ++i ){
      sim->rod.a[ i ]  = ( sim->z[ 2 * i ] - sim->rod.v[ i ] ) * h_inv; 
      sim->rod.v[ i ]  =   sim->z[ 2 * i ]; 
      sim->rod.x[ i ] +=   sim->step * sim->rod.v[ i ]; 
    }

    for ( i = 0; i < sim->rod.n - 1 ; ++i ){
      sim->rod.torsion[ i ] = sim->z[ 2 * i + 1 ] * h_inv ;
    }
  }
  lumped_rod_sim_sync_state_out(  sim );
}



/** set the positions at the ends */
void lumped_sim_set_velocity ( lumped_rod_sim * sim, double v, int j ){

  if ( j == 1 ) { 
    sim->rod.v[ sim->rod.n - 1  ] = v;
    sim->state.vN = v;
  }
  else {
    sim->rod.v[ 0 ] = v; 
    sim->state.v1 = v;
  }

}

/** set the positions at the ends */
void lumped_sim_set_position ( lumped_rod_sim * sim, double x, int j ){

  if ( j == 1 ) { 
    sim->rod.x[ sim->rod.n - 1  ] = x;
    sim->state.xN = x;
  }
  else {
    sim->rod.x[ 0 ] = x; 
    sim->state.x1 = x;
  }

}

/** set the forces at the ends */
void lumped_sim_set_force ( lumped_rod_sim * sim, double f, int j ){
  sim->forces[ j ] = f;
  if ( j == 0 )
    sim->state.f1 = f;
  else
    sim->state.fN = f;
}


/** get the positions at the end */
double lumped_sim_get_position ( lumped_rod_sim  * sim, int j ){

  if ( j ==  1 ){
    j = sim->rod.n - 1 ; 
  }

  return sim->rod.x[ j ];

}


/** get the velocities at the end */
double lumped_sim_get_velocity ( lumped_rod_sim * sim, int j ){

  if ( j ==  1 ){
    j = sim->rod.n - 1 ; 
  }

  return sim->rod.v[ j ];

}


/** get the accelerations at the end */
double lumped_sim_get_acceleration ( lumped_rod_sim * sim, int j ){

  if ( j ==  1 ){
    j = sim->rod.n - 1 ; 
  }

  return sim->rod.a[ j ];

}

/**
   only copy dynamic states
*/ 
void   lumped_rod_copy( lumped_rod * src, lumped_rod  * dest ){

  int N = src->n * sizeof( double ); 

  memcpy( dest->x, src->x, N);
  memcpy( dest->v, src->v, N);
  memcpy( dest->a, src->a, N);
  memcpy( dest->torsion, src->torsion, ( src->n - 1 ) * sizeof( double ) );

  return;
}

void lumped_rod_sim_store ( lumped_rod_sim  * sim ){
  lumped_rod_copy( & sim->rod, & sim->rod_back );
  return;
}

void lumped_rod_sim_restore ( lumped_rod_sim  * sim ){
  lumped_rod_copy( & ( sim->rod_back ), & ( sim->rod ) );
  return;
}
