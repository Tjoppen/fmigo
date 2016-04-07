#include "lumped_rod.h"
#include <stdio.h>
#include <stdlib.h>

enum {
  
    THETA_IN,			//angle (output, state)
    THETA_OUT,			//angle (output, state)
    OMEGA_IN,			//angle (output, state)
    OMEGA_OUT,			//angle (output, state)
    TAU_IN,			//coupling torque (input, state)
    TAU_OUT,			//coupling torque (input, state)
    J,				// Inertia [1/(kg*m^2)] (parameter)
    STEP,			// internal time step
    D,				// damping parameter 

    NUMBER_OF_REALS
};

#define NUMBER_OF_INTEGERS 0
#define NUMBER_OF_BOOLEANS 0
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0
#define FMI_COSIMULATION




int main(){

  int N = 10; 			/* number of elements */
  double mass = 20;		/* rod mass  */
  double compliance = 1e-8;	/* inverse stiffness */
  double step = ( double ) 1.0 / ( double ) 10.0;
  double tau  = ( double ) 2.0 ;
  lumped_rod_sim sim;

  sim = create_sim( N, mass, compliance, step, tau  );
  lumped_sim_set_force ( &sim,  1e6, 0);
  lumped_sim_set_force ( &sim, -1e6, 1);

  step_rod_sim( &sim, 40 );

  for ( int i = 0; i < sim.rod.n; ++i ){ 
    fprintf(stderr, "X[ %-2d ] = %2.3f\n", i, sim.rod.x[i]); 
  } 

  double * s = lumped_rod_sim_get_state( & sim ); 

  for ( int i = 0; i < 6; ++i ){ 
    fprintf(stderr, "S[ %-2d ] = %2.3f\n", i, s[i]); 
  } 

  return 0;

}
