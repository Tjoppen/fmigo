#ifndef LUMPED_ROD
#define LUMPED_ROD

/**
   This is to simulate a lumped-element rod using the spook stepper, based
   on a tridiagonal LDLT factorization. 
 */

#include "tridiag_ldlt.h"


/**
   The lumped rod has position, velocity and masses for all elements, as
   well as compliance for all connections.  

   Damping is left to the stepper though so that we can maintain stability.
*/
typedef struct lumped_rod{
  int n;			/* number of elements */
  double * x;			/* positions */
  double * v;			/* velocities */
  double * torsion;		/* constraint forces along the rod */
  double   rod_mass;		/* total mass: elements have mass mass/n */
  double   mass;		/* element mass */
  double   compliance;		/* global compliance: the compliance used in the constraints is compliance/n  */
  double mobility[ 4 ];		/* rod's mobility wrt forcing at each end */
   
} lumped_rod;


int lumped_rod_get_space_size( lumped_rod rod );
int lumped_rod_save_to_buffer( lumped_rod rod, void *buffer ) ;
int lumped_rod_read_from_buffer( lumped_rod * rod, void *buffer ) ;

/**
 * This is the state that is visible from the outside. 
 */
typedef struct lumped_rod_state {
  double x1;
  double xN;
  double v1;
  double vN;
  double f1;
  double fN;
} lumped_rod_state;


/**
   The full simulation struct which includes global parameters.
*/
typedef struct lumped_rod_sim{
  tri_matrix m;			/* system matrix */
  lumped_rod rod;               /* the rod being simulated */
  double  forces[2];		/* forces at each end: nothing in between */
  double * z;			/* buffer for solution: contains velocities and multipliers*/
  double step;			/* time step */
  double tau;			/* relaxation */
  double gamma_x;               /* stabilization factor for constraint violations */
  double gamma_v;               /* stabilization factor for constraint * velocities */
  lumped_rod_state state;
  
} lumped_rod_sim; 


/** straight forward allocation */
lumped_rod  lumped_rod_alloc( int n, double mass, double compliance ) ;
void        lumped_rod_free( lumped_rod rod );

/** copy data between two rods */
void   lumped_rod_copy( lumped_rod src, lumped_rod * dest );
/** computes mobility wrt the input forces at each end */
void        lumped_rod_sim_mobility( lumped_rod_sim sim, double * mob );

/** get the velocities at the end: j is either 0 or 1 */
double lumped_sim_get_velocity ( lumped_rod_sim sim, int j );
/** get the positions at the end: j is either 0 or 1  */
double lumped_sim_get_position ( lumped_rod_sim  sim, int j );
/** put forces at the ends.   j is either 0 or 1 */
void lumped_sim_set_force ( lumped_rod_sim * sim, double f, int j );

lumped_rod_sim lumped_rod_sim_alloc( int n ) ;
lumped_rod_sim lumped_rod_sim_free ( lumped_rod_sim sim ) ;

void  lumped_rod_sim_sync_state( lumped_rod_sim * sim );
double *   lumped_rod_sim_get_state( lumped_rod_sim  * sim );


/** build a tridiagonal matrix for simulating a lumped element rod using
 * relaxed constraints, i.e., arbitrarily stiff. 
 */
tri_matrix build_rod_matrix( lumped_rod  rod, double step, double tau ) ; 
/** allocate data, prepare and factorize matrices and setup all parameters
 * for the simulation */ 
lumped_rod_sim create_sim( int N, double mass, double compliance, double step, double tau  );
/** Called at each step */ 
void build_rod_rhs( lumped_rod_sim sim );

/** Integrate forward in time using spook */
void step_rod_sim( lumped_rod_sim * sim, int n );
  



#endif
