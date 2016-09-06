#include <lumped_rod.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



int main(){

  double x1[] = {
    0.00000,
    0.11111,
    0.22222,
    0.33333,
    0.44444,
    0.55556,
    0.66667,
    0.77778,
    0.88889,
    1.00000,
    0.90000,
    0.80000,
    0.70000,
    0.60000,
    0.50000,
    0.40000,
    0.30000,
    0.20000,
    0.10000,
    0.00000
  };
  double x[ size( x1 )  / sizeof( x1[ 0 ] ) ]

  int N = sizeof( x ) / sizeof( x[ 0 ] ) ;

  double v[ N ]; 

  memset( v, 0,  sizeof( x ) );
  memset( x, 0,  sizeof( x ) );


  lumped_rod_sim_parameters p = {
    1.0/100.0,			/* time step */
    {
      /* these are normally outputs, but used at init condition */
      0.0,			/* position of first point */
      0.0,			/* position of last point */
      0.0,			/* vel of first */
      0.0, 			/* vel of last */
      0.0,			/* accel */
      0.0,
      0.0,			/* dx */
      0.0,			
      0.0,			/* force */
      0.0,
      /* here come inputs*/
      0.0,			/* driving forces */
      0.0, 
      0.0,			/* velocity driver */
      0.0,
    },
    /* now the rod physical parameters */
    {
      N, 			/* number of elements */
      100,			/* total mass*/
      1e-3,			/* compliance */
      1,			/* relaxation rate in unit of steps */
      0,
      0,
      0,
      0
    }
  };
  
  lumped_rod_sim sim = lumped_rod_sim_create( p ) ; 
  memcpy( sim.rod.state.x, x, sizeof( x ) );

  rod_sim_do_step( &sim, 1 );

  for ( int i = 0; i < N; ++i ) {
    fprintf( stderr, "x[  %-2d  ] = %6.4f\n" , i, sim.rod.state.x[ i ] );
  }

  lumped_rod_sim_free( sim ) ;

#if 0
  /** execute the result of your life's work */
  /** this is where we would communicate with the FMU API */
  double * s = lumped_rod_sim_get_state(  &sim ); 

  lumped_rod_sim_store(  &sim ) ;
  
  lumped_rod_sim_restore(  &sim ) ;

#endif

  return 0;

}
