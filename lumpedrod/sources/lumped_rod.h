#ifndef LUMPED_ROD
#define LUMPED_ROD

/**
   This is to simulate a lumped-element rod using the spook stepper, based
   on a tridiagonal LDLT factorization. 
*/

#include "tridiag_ldlt.h"

#ifdef __cplusplus
extern "C" {
#endif

  struct lumped_rod_kinematics; // only kinematics
  struct lumped_rod;            // physical properties and one instance of kinematic states
  struct lumped_rod_coupling_states; // this is the state visible to the outside
  struct lumped_rod_coupling_sim_parameters; // bucket for the simulation: contains a rod and a rod_state
  struct lumped_rod_sim;		     /* The simulation itself: contains rod, coupling states and parameters */
  struct lumped_rod_init_conditions;         // convenient packaging


  /** kinematic state of the rod */
  typedef struct lumped_rod_kinematics{

    double * x;			/** positions */
    double * v;			/** velocities */
    double * a;			/** accelerations */
    double * torsion;		/** constraint forces along the rod */

  } lumped_rod_kinematics ;



/**
   
   Definition of physical structure and internal states of the rod in
   complete isolation, i.e., external influences aren't included here.
   
   How the rod is simulated is left out of this. 
   
*/
  
  typedef struct lumped_rod{

    /** fixed physical parameters */
    int n;			/** number of elements */
    double   rod_mass;		/** total mass */
    double   compliance;	/** global stiffness: the stiffness used in the constraints is  n * stiffness  */
    double  relaxation_rate;	/** even though this is timestep dependent,
				    it logically belongs here since it is
				    part of physical properties and could in fact be set to sensible
				    physical values.  It will be set to 2
				    by default however. */

    double mass;		/* element mass: this is computed during initialization */
    double mobility[ 4 ];	/** rod's mobility wrt forcing at each
				 * end: this is derived from parameters
				 * above, but might be step dependent. */
    /** kinematic state  */
    lumped_rod_kinematics state; 
   
  } lumped_rod;

/** 
 * This is the kinematic state relevant to the outside, as well as dynamic inputs.
 */ 
  typedef struct lumped_rod_coupling_states{

    /** kinematics: OUTPUTS */
    double x1;                  /* position/angle of first point */
    double v1;			/* velocity of first point */
    double a1;			/* acceleration of last point */
    double dx1;			/* integrated angle difference on first point */

    double xN;			/* position/angle of last point */
    double vN;			/* velocity of last point */
    double aN;			/* acceleration of first point */
    double dxN;			/* integrated angle difference between last
				 * point and external driver. */
    /** for force velocity couplings: output force */
    double coupling_f1;		/* force/torque on first point */
    double coupling_fN;		/* force/torque on last point */

    /** INPUTS */ 
    double f1;			/* force/torque on first point */
    double fN;			/* force/torque on last point */
    
    /** Force-velocity coupling velocities and displacements */
    double coupling_x1;		/* driving displacement on first point */
    double coupling_v1;		/* driving velocity on first point */

    double coupling_xN;		/* driving displacement on last point */
    double coupling_vN;		/* driving velocity on last point */

  } lumped_rod_coupling_states;



/**
 *  Coupling parameters for cosimulation
 */

  typedef struct lumped_rod_coupling_parameters{

    double coupling_stiffness1;
    double coupling_damping1;
    double coupling_stiffnessN;
    double coupling_dampingN;

    double coupling_sign1;
    double coupling_signN;
    int    integrate_dx1;
    int    integrate_dxN;
    
  } lumped_rod_coupling_parameters;


  /**
   * One of multiple ways to initialize the rod.  
   */
  typedef struct lumped_rod_init_conditions{

    double x1;
    double v1;
    double xN;
    double vN;

  } lumped_rod_init_conditions;
  

/**
 * The full simulation struct which includes global parameters 
 * and influence from the outside.
 */
  typedef struct lumped_rod_sim{
    double step;
    lumped_rod                     rod;		/** primary copy of the rod.  */
    lumped_rod_coupling_parameters coupling_parameters; 
    lumped_rod_coupling_states     coupling_states;
    
    tri_matrix  matrix;		/** system matrix */
    double *    z; /** buffer for solution: contains velocities and multipliers*/

    /** These parameters are derived from the timestep and other parameters. */
    double gamma_x;
    double gamma_v;
    double gamma_coupling_x1;
    double gamma_coupling_xN;
    double gamma_coupling_v1;
    double gamma_coupling_vN;

    

    /** backup for store/restore */
    lumped_rod_coupling_states coupling_states_backup; 
    lumped_rod                 rod_backup; /** storage space for store/restore. */
  
  } lumped_rod_sim; 


/*****************************************
 ** External interface to the lumped rod itself.
 ***************************************/

/** copy data between two rods */
  void   lumped_rod_copy( lumped_rod * src, lumped_rod  * dest );

/** Guess. This will reset the matrix and other parameters which are derived from the time step. */
  void lumped_sim_set_timestep ( lumped_rod_sim * sim, double step);

/** Store all dynamic states in a preallocated buffer */
  void lumped_rod_sim_store ( lumped_rod_sim  * sim );

/** Restore all dynamic states from a preallocated buffer */
  void lumped_rod_sim_restore ( lumped_rod_sim *  sim );

/** Allocate data, prepare and factorize matrices and setup all parameters
 * for the simulation */ 
  lumped_rod_sim lumped_rod_sim_initialize( lumped_rod_sim sim, lumped_rod_init_conditions init );

/** Deep deallocation of all included objects */
  void lumped_rod_sim_free( lumped_rod_sim  sim ) ;
 
/** Integrate forward in time using Spook by n fixed step. */
  void rod_sim_do_step( lumped_rod_sim * sim, int n );
  

#ifdef __cplusplus
}
#endif
  
#endif
