#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include "lumped_rod.h"

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
  rod.torsion      = ( double * ) malloc( ( n - 1 )  * sizeof( double ) );		

  return rod;
}

/** WARNING:  no error checking */
void lumped_rod_free( lumped_rod rod ){

  free( rod.x           );
  free( rod.v           );
  free( rod.torsion     );

  return;

}


/** WARNING:  no error checking */
lumped_rod_sim lumped_rod_sim_alloc( int n ) {

  lumped_rod_sim  sim;
  
  sim.z       = (double * ) malloc( ( 2 * n + 1 ) * sizeof( double ) );

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
*/

tri_matrix build_rod_matrix( lumped_rod  rod, double step, double tau ) { 

  int n = rod.n;
  tri_matrix m; 		/* matrix  */
  double gamma = 1.0 /  ( 1.0 + 4.0 * tau  );
  double compliance = - ( 4.0  * gamma / step / step ) * 
    rod.compliance  / ( double ) ( rod.n - 1 );
  int i;

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
  

  tri_factor( &m );			/* this only has to be factored once */

  return m;
  
}


/**
   This does almost all the work.  The one assumption is that the lumped_rod
   struct has a valid value for rod_mass, N,   and compliance.  Everything
   else is derived from that. 
 */
lumped_rod_sim create_sim( int N, double mass, double compliance, double step, double tau  ){
  lumped_rod_sim sim = lumped_rod_sim_alloc( N );
  sim.rod = lumped_rod_alloc( N, mass, compliance ); 
  sim.step = step;
  sim.tau  = tau;
  sim.gamma_v = ( double  ) 1.0  /  (  1.0  +  4.0  * tau );
  sim.gamma_x = - (double ) 4.0  * sim.gamma_v   /  step  ;

  sim.m = build_rod_matrix( sim.rod, sim.step, sim.tau ) ;
  lumped_rod_sim_mobility( sim, sim.rod.mobility );
  
  return sim;
}


/** This is the standard for spook
 */
void build_rod_rhs( lumped_rod_sim sim ){
  int i;
  
  for ( i = 0; i < sim.rod.n ; ++i ) {
    sim.z[ 2 * i ] = sim.rod.mass * sim.rod.v[ i ]; 
  }

  /* forces at the end */
  sim.z[  0          ] +=  sim.step * sim.forces[ 0 ]; 
  sim.z[ sim.m.n - 1 ] +=  sim.step * sim.forces[ 1 ]; 

  for ( i = 0; i <  sim.rod.n - 1 ;  ++i ) {

    sim.z[ 2 * i  + 1 ] =
      sim.gamma_x * ( sim.rod.x[  i  ] - sim.rod.x[ i + 1 ] )
      + sim.gamma_v * ( sim.rod.v[  i  ] - sim.rod.v[ i + 1 ] );

  }

}

/**  
     The rod can be forced at the first and last particle.  The mobility
     here is the 2x2 matrix of reaction forces given unit impulses at the
     two signals.  

     The output matrix is assumed to be row major
 */
void lumped_rod_sim_mobility( lumped_rod_sim sim, double  * mob ){

  /* force on first mass */
  bzero( sim.z, sim.m.n * sizeof( double ) ); 
  sim.z[ 0 ] = ( double ) 1.0; 
  tri_solve( &sim.m, sim.z );

  /* collect results */ 
  mob[ 0 ] = sim.z[ 0 ];
  mob[ 1 ] = sim.z[ sim.m.n - 1 ];

  bzero( sim.z, sim.m.n * sizeof( double ) ); 
  sim.z[ sim.m.n - 1  ] = ( double ) 1.0; 
  tri_solve( &sim.m, sim.z );
  
  /* collect results */ 
  mob[ 2 ] = sim.z[ 0 ];
  mob[ 3 ] = sim.z[ sim.m.n - 1 ];

}

double *   lumped_rod_sim_get_state( lumped_rod_sim  * sim ){
  return ( double *) & ( sim->state ) ; 
}

void  lumped_rod_sim_sync_state( lumped_rod_sim * sim ){
  int last = sim->rod.n -1 ; 

  sim->state.x1 = sim->rod.x [    0  ];
  sim->state.xN = sim->rod.x [  last ];
  sim->state.v1 = sim->rod.v [    0  ];
  sim->state.vN = sim->rod.v [  last ];
  sim->state.f1 = sim->forces[    0  ];
  sim->state.fN = sim->forces[    1  ];

}

/**
 *   Step forward in time.  
 */
void step_rod_sim( lumped_rod_sim * sim, int n ){
  
  int i, j ; 
  for ( j = 0; j < n; ++j  ){

    build_rod_rhs( *sim );
    tri_solve( &sim->m, sim->z );
  
    for ( i = 0; i < sim->rod.n; ++i ){
      sim->rod.v[ i ]  = sim->z[ 2 * i ]; 
      sim->rod.x[ i ] += sim->step * sim->rod.v[ i ]; 
    }

    for ( i = 0; i < sim->rod.n - 1 ; ++i ){
      sim->rod.torsion[ i ] = sim->z[ 2 * i + 1 ] /  sim->step ;
    }
  }
  lumped_rod_sim_sync_state(  sim );
}


int lumped_rod_get_space_size( lumped_rod rod ){
  return  ( 3 * rod.n - 1 )  *  sizeof( double ) + sizeof( lumped_rod );
}

int lumped_rod_save_to_buffer( lumped_rod rod, void *buffer ) {

  int pos = 0;
  int n = sizeof( lumped_rod );
  memcpy( buffer, &rod, n );
  pos += n;
  
  n = rod.n * sizeof( double ) ;
  memcpy( buffer + pos,  rod.x, n );
  pos += n;

  n = rod.n * sizeof( double ) ;
  memcpy( buffer + pos,  rod.v, n );
  pos += n;

  n = ( rod.n - 1 ) * sizeof( double ) ;
  memcpy( buffer + pos,  rod.torsion, n );
  pos += n;

  return pos;
}


int lumped_rod_read_from_buffer( lumped_rod * rod, void *buffer ) {

  int pos = 0;
  int n = sizeof( lumped_rod );
  memcpy( rod, buffer, n );
  pos += n;
  
  n = rod->n * sizeof( double ) ;
  memcpy( rod->x, buffer + pos, n );
  pos += n;

  n = rod->n * sizeof( double ) ;
  memcpy( rod->v, buffer + pos, n );
  pos += n;

  n = ( rod->n - 1 ) * sizeof( double ) ;
  memcpy( rod->torsion, buffer + pos, n );
  pos += n;

  return pos;
}


/** set the forces at the ends */
void lumped_sim_set_force ( lumped_rod_sim * sim, double f, int j ){
  sim->forces[ j ] = f;
}


/** get the positions at the end */
double lumped_sim_get_position ( lumped_rod_sim  sim, int j ){

  if ( j ==  1 ){
    j = sim.rod.n - 1 ; 
  }

  return sim.rod.x[ j ];

}


/** get the velocities at the end */
double lumped_sim_get_velocity ( lumped_rod_sim sim, int j ){

  if ( j ==  1 ){
    j = sim.rod.n - 1 ; 
  }

  return sim.rod.v[ j ];

}


void   lumped_rod_copy( lumped_rod src, lumped_rod * dest ){

  int N;
  if ( dest->x == NULL  ) {
    *dest = lumped_rod_alloc( src.n, src.mass, src.compliance ) ;
  }
  if ( dest->n !=  src.n ){
    lumped_rod_free( *dest ) ;
  }
  N = src.n * sizeof( double ); 

  memcpy( dest->x, src.x, N);
  memcpy( dest->v, src.v, N);
  memcpy( dest->torsion, src.torsion, N - 1);

  dest->rod_mass   = src.rod_mass;
  dest->mass       = src.mass;
  dest->compliance = src.compliance;

  memcpy( dest->mobility, src.mobility,  sizeof( src.mobility) );

}
