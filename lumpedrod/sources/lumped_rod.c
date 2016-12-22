#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "lumped_rod.h"
#include "safealloc.h"

//#define ROD_WRITE_OUTPUT

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


static void lumped_rod_alloc( lumped_rod * rod ) {
  
  assert( rod );
  assert( rod->n );
  
  rod->mass  = rod->rod_mass  / ( double ) rod->n;
  
  CALLOC( double, rod->state.x       , rod->n);
  CALLOC( double, rod->state.v       , rod->n);
  CALLOC( double, rod->state.a       , rod->n);
  CALLOC( double, rod->state.torsion, rod->n);

  return ;

}

/** WARNING:  no error checking */
static void lumped_rod_free( lumped_rod rod ){

  FREE( rod.state.x           );
  FREE( rod.state.v           );
  FREE( rod.state.a           );
  FREE( rod.state.torsion     );

  return;

}

/**  
     The rod can be forced at the first and last particle.  The mobility
     here is the 2x2 matrix of reaction forces given unit impulses at the
     two signals.  

     The output matrix is assumed to be row major
*/
static void lumped_rod_sim_get_mobility( lumped_rod_sim * sim, double  * mob ){

  /* unit force on first mass */
  memset( sim->z, 0, sim->matrix.n * sizeof( double ) );
  sim->z[ 0 ] = ( double ) 1.0; 
  tri_solve( &sim->matrix, sim->z );

  /* collect results */ 
  mob[ 0 ] = sim->z[ 0 ];
  mob[ 1 ] = sim->z[ sim->matrix.n - 1 ];

  /* unit force on the last mass */
  memset( sim->z, 0, sim->matrix.n * sizeof( double ) );
  sim->z[ sim->matrix.n - 1  ] = ( double ) 1.0; 
  tri_solve( &sim->matrix, sim->z );
  
  /* collect results */ 
  mob[ 2 ] = sim->z[ 0 ];
  mob[ 3 ] = sim->z[ sim->matrix.n - 1 ];

}


static void lumped_rod_initialize( lumped_rod rod, lumped_rod_init_conditions  initial){

  /** no need to initialize acceleration or torsion */
  double dx =  ( initial.xN - initial.x1 )  / ( double )  ( rod.n - 1 );
  double dv =  ( initial.vN - initial.v1 )  / ( double )  ( rod.n - 1 );
  int i; 

  for ( i = 0; i < rod.n; ++i ){
    rod.state.x[ i ]  =  initial.x1 + ( ( double ) i ) * dx ; 
    rod.state.v[ i ]  =  initial.v1 + ( ( double ) i ) * dv ; 
  }

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
static tri_matrix build_rod_matrix( lumped_rod  rod, lumped_rod_coupling_parameters p, double step) { 

  int n = rod.n;
  tri_matrix m; 		/* matrix  */
  double gamma = 1.0 /  ( 1.0 + 4.0 * rod.relaxation_rate );
  double compliance = - rod.compliance * ( 4.0  * gamma / step / step ) /  ( double ) rod.n  ; 
  double stiffness1 = 0; 
  double stiffnessN = 0; 

  if ( p.coupling_stiffness1 > 0 )
    stiffness1 =  p.coupling_stiffness1 * ( 1.0  + 4.0 * p.coupling_damping1 / p.coupling_stiffness1 / step ) * step * step /  4.0 ;
  if ( p.coupling_stiffnessN > 0 )
    stiffnessN =  p.coupling_stiffnessN * ( 1.0  + 4.0 * p.coupling_dampingN / p.coupling_stiffnessN / step ) * step * step /  4.0 ;

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
					 *  once unless we change the time step.
					 */
  return m;
  
}



/** 
 *  This is the standard for spook. 
 * 
 *
 */
static void build_rod_rhs( lumped_rod_sim * sim ){

  int i;
  double tmp;
  for ( i = 0; i < sim->rod.n - 1 ; ++i ) {

    sim->z[ 2 * i ] = sim->rod.mass * sim->rod.state.v[ i ]; /* mass term */

    sim->z[ 2 * i  + 1 ] =
      + sim->gamma_x * ( sim->rod.state.x[  i  ] - sim->rod.state.x[ i + 1 ] ) /* constraint violation */
      + sim->gamma_v * ( sim->rod.state.v[  i  ] - sim->rod.state.v[ i + 1 ] ); /* relative velocity */
  }

  sim->z[ sim->matrix.n  - 1 ] = sim->rod.mass * sim->rod.state.v[ sim->rod.n -1 ]; /* deal with the last mass */

  /* forces at the end */
  sim->z[  0               ]  +=  sim->step * sim->coupling_states.f1;
  sim->z[ sim->matrix.n - 1]  +=  sim->step * sim->coupling_states.fN;

  /* drivers with force-velocity couplings */

  /** Driver on first particle */
  if ( sim->coupling_parameters.integrate_dx1 ) {
    sim->coupling_states.dx1 += sim->step * ( sim->rod.state.v[ 0 ] -  sim->coupling_states.coupling_v1 );
    tmp = sim->coupling_states.dx1;
  } else {
    tmp = sim->rod.state.x[ 0 ] - sim->coupling_states.coupling_x1;
  }

  /** compute the output force */ 
  sim->coupling_states.coupling_f1 = sim->coupling_parameters.coupling_stiffness1  * tmp  +
    sim->coupling_parameters.coupling_damping1 * ( sim->rod.state.v[ 0 ] - sim->coupling_states.coupling_v1 ) ;
  
  /** RHS contribution: not same as output force because we are doing implicit integration here. */
  sim->z[ 0 ] +=  ( 
    - sim->step * sim->coupling_parameters.coupling_stiffness1
    * ( tmp - sim->step * ( sim->rod.state.v[ 0 ] - sim->coupling_states.coupling_v1 ) / 4.0 )
    ) ;

  /** Driver on last particle */
  
  if ( sim->coupling_parameters.integrate_dxN ) {
    sim->coupling_states.dxN += sim->step * ( sim->rod.state.v[ sim->rod.n - 1 ] -  sim->coupling_states.coupling_vN );
    tmp = sim->coupling_states.dxN;
  } else {
    tmp = sim->rod.state.x[ sim->rod.n - 1 ] - sim->coupling_states.coupling_xN;
  }

  /** compute the output */ 

  sim->coupling_states.coupling_fN = sim->coupling_parameters.coupling_stiffnessN  * tmp  +
    sim->coupling_parameters.coupling_dampingN * ( sim->rod.state.v[ sim->rod.n - 1 ] - sim->coupling_states.coupling_vN ) ;
  
  /** RHS contribution: not same as output force because we are doing implicit integration here. */
  sim->z[ sim->matrix.n -1 ] +=  ( 
    - sim->step * sim->coupling_parameters.coupling_stiffnessN
    * ( tmp - sim->step * ( sim->rod.state.v[ sim->rod.n - 1 ] - sim->coupling_states.coupling_vN ) / 4.0 )
    );
  
}

/**
 *   Step forward in time.  
 */
void rod_sim_do_step( lumped_rod_sim * sim, int n ){

  assert( sim );
  
  
  double h_inv = 1.0  / sim->step;
  int i, j ; 

  for ( j = 0; j < n; ++j  ){

    sim->time  += sim->step;
    build_rod_rhs( sim );

    tri_solve( &sim->matrix, sim->z );
  
    for ( i = 0; i < sim->rod.n; ++i ){
      sim->rod.state.a[ i ]  = ( sim->z[ 2 * i ] - sim->rod.state.v[ i ] ) * h_inv; 
      sim->rod.state.v[ i ]  =   sim->z[ 2 * i ]; 
      sim->rod.state.x[ i ] +=   sim->step * sim->rod.state.v[ i ]; 
    }

    
    for ( i = 0; i < sim->rod.n -1 ; ++i ){
      sim->rod.state.torsion[ i ] = sim->z[ 2 * i + 1 ] * h_inv ;
    }
  }
#ifdef ROD_WRITE_OUTPUT
  {
    FILE *f = (FILE *) sim->file;
    int i = 0;
    fprintf(f, "%g ", sim->time);
    for( i = 0; i < sim->rod.n; ++ i )
      fprintf(f, " %g ", sim->rod.state.x[ i ] );
    for( i = 0; i < sim->rod.n; ++ i )
      fprintf(f, " %g ", sim->rod.state.v[ i ] );
  fputs("\n", f);
  }
#endif

  /** 
      Publish variables.
  */

  /* the internal torsion should be the reaction force but we're reporting
   * the coupling spring force instead */
  
  //  sim->coupling_states.coupling_f1 = sim->rod.state.torsion[ 0              ];
  //  sim->coupling_states.coupling_fN = sim->rod.state.torsion[ sim->rod.n - 2 ];
  sim->coupling_states.x1 = sim->rod.state.x      [ 0              ];
  sim->coupling_states.xN = sim->rod.state.x      [ sim->rod.n - 2 ];
  sim->coupling_states.v1 = sim->rod.state.v      [ 0              ];
  sim->coupling_states.vN = sim->rod.state.v      [ sim->rod.n - 2 ];
  sim->coupling_states.a1 = sim->rod.state.a      [ 0              ];
  sim->coupling_states.aN = sim->rod.state.a      [ sim->rod.n - 2 ];

  sim->coupling_states.dx1 += sim->step * ( sim->rod.state.v[ 0              ] - sim->coupling_states.coupling_v1 );
  sim->coupling_states.dxN += sim->step * ( sim->rod.state.v[ sim->rod.n - 1 ] - sim->coupling_states.coupling_vN );

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
  lumped_rod_copy(  & sim->rod,  & sim->rod_backup );
  return;
}

void lumped_rod_sim_restore ( lumped_rod_sim  * sim ){
  lumped_rod_copy( & ( sim->rod_backup ),  & ( sim->rod ) );
  return;
}


/**
   This does almost all the work.  The one assumption is that the lumped_rod
   struct has a valid value for rod_mass, N,   and compliance.  Everything
   else is derived from that. 
*/
lumped_rod_sim lumped_rod_sim_initialize( lumped_rod_sim sim, lumped_rod_init_conditions initial) {
  sim.time = 0;
  MALLOC( double, sim.z, 2 * sim.rod.n - 1 ); 
  sim.rod_backup = sim.rod;
  lumped_rod_alloc( &sim.rod );
  lumped_rod_alloc( & sim.rod_backup ); /* for store / restore */

  if ( sim.coupling_parameters.coupling_sign1 >= 0 ){
    sim.coupling_parameters.coupling_sign1 =  1.0;
  } else {
    sim.coupling_parameters.coupling_sign1 = -1.0;
  }

  if ( sim.coupling_parameters.coupling_signN >= 0 ){
    sim.coupling_parameters.coupling_signN =  1.0;
  } else {
    sim.coupling_parameters.coupling_signN = -1.0;
  }

  lumped_rod_coupling_states c = sim.coupling_states;
  
  sim.coupling_states_backup = c;
  lumped_sim_set_timestep( &sim, sim.step );
  lumped_rod_initialize(   sim.rod , initial );

#ifdef ROD_WRITE_OUTPUT
  sim.file = ( void * ) fopen("./rod_data.mat", "w+");
#endif
  return sim;
}



void lumped_sim_set_timestep ( lumped_rod_sim * sim, double step){

  sim->step = step;
  sim->gamma_v =   ( double ) 1.0  /  (  1.0  +  4.0  * sim->rod.relaxation_rate );
  sim->gamma_x = - ( double ) 4.0  * sim->gamma_v   /  sim->step  ;

  sim->matrix = build_rod_matrix( sim->rod, sim->coupling_parameters, sim->step) ;
  lumped_rod_sim_get_mobility( sim, sim->rod.mobility );

}

void lumped_rod_sim_free( lumped_rod_sim   sim ) {

  lumped_rod_free( sim.rod      );
  lumped_rod_free( sim.rod_backup );
  tri_matrix_free( sim.matrix );
  free ( sim.z );
#ifdef ROD_WRITE_OUTPUT
  fclose( ( FILE * ) sim.file );
#endif

  return;
  
}




#ifdef CONSOLE

int main(){

  lumped_rod_sim sim = {
    1.0 / 10.0, 		/* step */
    {				/* rod */
      25, 			/* num elements */
      5, 			/* mass */
      1e-3,			/* global compliance */
      0				/* relaxation */
    }, 
    {				/* coupling parameters */
      0,			/* stiffness */
      0,			/* damping */
      0,			/* stiffness */
      0,			/* damping */
      1,			/* coupling sign */
      1,			/* coupling sign */
      0,			/* integrate dx */
      0				/* integrate dx */
    }, 
    {				/* coupling states: ouputs */
      0,			/* x */
      0,			/* v */
      0,			/* a */
      0,			/* dx */
      0,			/* x */
      0,			/* v */
      0,			/* a */
      0,			/* dx */
      /* force output*/
      0, 			/* coupling_f1 */
      /* force output*/
      0, 			/* coupling_fN */
      /* force inputs */ 
      0,			/* force in*/
      0,			/* force in */
      /* velocity displacement couplings */
      0, 			/* coupling_x1 */
      0, 			/* coupling_v1 */
      0, 			/* coupling_xN */
      0 			/* coupling_vN */
    }, 
  };

  lumped_rod_init_conditions init = {
    -1.0,
    0.000,
    1.000,
    0.0
  };

  int print = 1;
  int N = 2000;
  
  sim = lumped_rod_sim_initialize(sim, init );
  
  for ( int j = 0; j < N; ++j ){
    rod_sim_do_step( &sim, 1);
    if ( print ){
      for ( int i = 0; i < sim.rod.n; ++i ) {
	fprintf( stderr, "%6.4f " , i, sim.rod.state.x[ i ] );
      }
      fprintf(stderr, "\n");
    }
  }
  
  lumped_rod_sim_store( & sim );
  lumped_rod_sim_restore( & sim );
  
  lumped_rod_sim_free( sim ) ;


  return 0;
}

#endif
