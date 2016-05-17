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
    double   stiffness;	/** global stiffness: the stiffness used
				 * in the constraints is  n * stiffness  */
    double  relaxation_rate;	/** even though this is timestep dependent,
				    it logically belongs here since it is
				    part of physical properties and could in fact be set to sensible
				    physical values.  It will be set to 2
				    by default however */
    double driver_stiffness1;	/** This is for the case when we have a
				    driver at point 1 */
    double driver_relaxation1;	
    double driver_stiffnessN;	/** This is for the case when we have a
				    driver at point N */
    double driver_relaxationN;	
    double driver_sign1;
    double driver_signN;
    double mass;		/* element mass: this is computed during
                                 * initialization */
  
    double mobility[ 4 ];	/** rod's mobility wrt forcing at each
				 * end: this is derived from parameters
				 * above, but might be step dependent. */
    /** kinematic state  */
    lumped_rod_kinematics state; 
   
  } lumped_rod;




  
/** Sets initial conditions: eventual drivers are left out of this here
  because they belong to outside influences like forces.  */
  void lumped_rod_initialize( lumped_rod  rod, double x1, double xN, double v1, double vN);

/** 
 * This is the kinematic state relevant to the outside, as well as dynamic inputs.
 */ 
  typedef struct lumped_rod_state {

    /** kinematics: outputs */
    double x1;                  /* position/angle of first point */
    double xN;			/* position/angle of last point */
    double v1;			/* velocity of first point */
    double vN;			/* velocity of last point */
    double a1;			/* acceleration of first point */
    double aN;			/* acceleration of last point */
    double dx1;			/* integrated angle difference on first point */
    double dxN;			/* integrated angle difference between last
				 * point and external driver. */
    double f1;			/* force/torque on first point */
    double fN;			/* force/torque on last point */


    
    /** Driving forces and velocities */
    double driver_f1;		/* force/torque on first point */
    double driver_fN;		/* force/torque on last point */
    double driver_v1;		/* driving velocity on first point */
    double driver_vN;		/* driving velocity on last point */

  } lumped_rod_state;


/** Parameters needed by the simulation.  

    There is some twisted logic here because of the rod structure.  Yet
    this makes it relatively easy to initialize as the rod's physical
    parameters such as mass and damping can be set using brace
    initializers.
*/
  typedef struct lumped_rod_sim_parameters{

    double step;	           /** time step */

    lumped_rod_state state;

    lumped_rod rod;

  } lumped_rod_sim_parameters ;


/**
   The full simulation struct which includes global parameters.
*/
  typedef struct lumped_rod_sim{
    /** twisted logic here: we call this a state though that's also the
	parameters for initial conditions. */
    lumped_rod_sim_parameters state; 

    lumped_rod  rod;		   
    tri_matrix m;	           /** system matrix */
    lumped_rod rod_back;           /** storage space for store/restore.
				       The rod itself is already in the state. */
    double * z;	                   /** buffer for solution: contains velocities and multipliers*/
    double gamma_x;                  /** stabilization factor for constraint violations */
    double gamma_v;                  /** stabilization factor for velocities */
    double gamma_driver_x1;          /** stabilization factor for constraint * velocities */
    double gamma_driver_v1;          
    double gamma_driver_xN;         
    double gamma_driver_vN;        
    /** backup for store/restore */
    lumped_rod_sim_parameters state_backup; 
  
  } lumped_rod_sim; 



/** straight forward allocation */
  void lumped_rod_alloc( lumped_rod * rod ) ;
  void        lumped_rod_free( lumped_rod rod );

/** copy data between two rods */
  void   lumped_rod_copy( lumped_rod * src, lumped_rod  * dest );
/** Computes mobility wrt the input forces at each end. We need the
    simulation object here because that's where the matrix is*/
  void   lumped_rod_sim_mobility( lumped_rod_sim * sim, double * mob );

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
/** Assign *driving* velocity at ends.   j is either 0 or 1.
    This will not impose a velocity directly, but via a very stiff spring. 
*/
  void lumped_sim_set_driving_velocity ( lumped_rod_sim * sim, double v, int j );

  void lumped_sim_set_timestep ( lumped_rod_sim * sim, double step);

/** This will store the dynamic state in a preallocated buffer */
  void lumped_rod_sim_store ( lumped_rod_sim  * sim );
/** This will restore the dynamic state from a preallocated buffer */
  void lumped_rod_sim_restore ( lumped_rod_sim *  sim );

/** Very basic allocation for the sates belonging to the string itself */
  lumped_rod_sim lumped_rod_sim_alloc( int n ) ;


/** Build a tridiagonal matrix for simulating a lumped element rod using
 * relaxed constraints, i.e., arbitrarily stiff. 
 */
  tri_matrix build_rod_matrix( lumped_rod  rod, double step) ; 
/** Allocate data, prepare and factorize matrices and setup all parameters
 * for the simulation */ 
  lumped_rod_sim lumped_rod_sim_create( lumped_rod_sim_parameters p);
/** Deep deallocation of all included objects */
  void lumped_rod_sim_free( lumped_rod_sim  sim ) ;
 
/** Called at each step */ 
  void build_rod_rhs( lumped_rod_sim * sim );

/** Integrate forward in time using spook */
  void rod_sim_do_step( lumped_rod_sim * sim, int n );
  

#ifdef __cplusplus
}
#endif
  
#endif
