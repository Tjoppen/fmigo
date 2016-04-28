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
  double * a;			/* accelerations */
  double * torsion;		/* constraint forces along the rod */
  double   rod_mass;		/* total mass: elements have mass mass/n */
  double   mass;		/* element mass */
  double   compliance;		/* global compliance: the compliance used in the constraints is compliance/n  */
  double mobility[ 4 ];		/* rod's mobility wrt forcing at each end */
   
} lumped_rod;


void lumped_rod_initialize( lumped_rod * rod, double x1, double xN, double v1, double vN);


/** 
 * This to interface with FMI
 */ 
typedef struct lumped_rod_state {
  double x1;
  double xN;
  double v1;
  double vN;
  double a1;
  double aN;
  double f1;
  double fN;
  double mass;
  double compliance;
  double tau;
  double step;
  
} lumped_rod_state;


/**
   The full simulation struct which includes global parameters.
*/
typedef struct lumped_rod_sim{
  tri_matrix m;			/** system matrix */
  lumped_rod rod;               /** the rod being simulated */
  lumped_rod rod_back;          /** storage space for store/restore */
  double  forces[2];		/** forces at each end: nothing in between */
  double * z;			/** buffer for solution: contains velocities and multipliers*/
  double step;			/** time step */
  double tau;			/** relaxation */
  double gamma_x;               /** stabilization factor for constraint violations */
  double gamma_v;               /** stabilization factor for constraint * velocities */
  lumped_rod_state state;	/** what is exposed to the outside in terms
				 * of I/O states*/
  
} lumped_rod_sim; 


typedef struct lumped_rod_sim_parameters{

  double mass;			/** Total mass of the rod, or moment of inertia.  */
  int N;			/** Number of elements */
  double compliance;		/** inverse of stiffness of *whole* rod */
  double step;                  /** time step */
  double tau;			/** damping: this should be between 0 and 4
				 * and is dimensionless.  It's essentially
				 *  a relaxation rate in "units"  of time step. 
				*/

  double x1;			/** initial displacement at first end */
  double xN;			/** initial displacement at other end: interpolation in between */
  double v1;			/** initial velocity at first end */
  double vN;			/** initial velocity at other end: interpolation in between */
  double f1;			/** initial force (torsion) at first end */
  double fN;			/** initial force (torsion) at other end */

} lumped_rod_sim_parameters ;



/** straight forward allocation */
lumped_rod  lumped_rod_alloc( int n, double mass, double compliance ) ;
void        lumped_rod_free( lumped_rod rod );

/** copy data between two rods */
void   lumped_rod_copy( lumped_rod * src, lumped_rod  * dest );
/** computes mobility wrt the input forces at each end */
void        lumped_rod_sim_mobility( lumped_rod_sim * sim, double * mob );

/** get the velocities at the end: j is either 0 or 1 */
double lumped_sim_get_velocity ( lumped_rod_sim *sim, int j );
/** get the positions at the end: j is either 0 or 1  */
double lumped_sim_get_position ( lumped_rod_sim  *sim, int j );
/** put forces at the ends.   j is either 0 or 1 */
void lumped_sim_set_force ( lumped_rod_sim * sim, double f, int j );
/** Assign velocities at the ends.   j is either 0 or 1 */
void lumped_sim_set_velocity ( lumped_rod_sim * sim, double v, int j );
/** Assign positions at the ends.   j is either 0 or 1 */
void lumped_sim_set_position ( lumped_rod_sim * sim, double x, int j );

/** This will store the dynamic states in a preallocated buffer */
void lumped_rod_sim_store ( lumped_rod_sim  * sim );
/** This will restore the dynamic states from a preallocated buffer */
void lumped_rod_sim_restore ( lumped_rod_sim *  sim );

/** Very basic allocation for the sates belonging to the string itself */
lumped_rod_sim * lumped_rod_sim_alloc( int n ) ;
/** Shallow free */
void lumped_rod_sim_free ( lumped_rod_sim *sim ) ;

/** Map states of inputs to inside variables */
void  lumped_rod_sim_sync_state_in( lumped_rod_sim * sim );
/** Map states of published variables to an array readable from outside */
void  lumped_rod_sim_sync_state_out( lumped_rod_sim * sim );
double *   lumped_rod_sim_get_state( lumped_rod_sim  * sim );


/** Build a tridiagonal matrix for simulating a lumped element rod using
 * relaxed constraints, i.e., arbitrarily stiff. 
 */
tri_matrix build_rod_matrix( lumped_rod  rod, double step, double tau ) ; 
/** Allocate data, prepare and factorize matrices and setup all parameters
 * for the simulation */ 
lumped_rod_sim * lumped_rod_sim_create( lumped_rod_sim_parameters p);
/** Deep deallocation of all included objects */
void lumped_rod_sim_delete( lumped_rod_sim  * sim ) ;
 
/** Called at each step */ 
void build_rod_rhs( lumped_rod_sim * sim );

/** Integrate forward in time using spook */
void step_rod_sim( lumped_rod_sim * sim, int n );
  

  
#endif
