#ifndef NONSMOOTH_CLUTCH
#define NONSMOOTH_CLUTCH
#include <stddef.h>

/** 
 *  This defines both the C and C++ interfaces to the nonsmooth clutch
 *  model. 
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

  /**
   * Note that the nonsmooth clutch class will inherit from the parameter struct. 
   * This reduces errors since copying is now just a structure assignment.
   */

  /**
     API
   */
  typedef struct nonsmooth_clutch_params nonsmooth_clutch_params;
  void * nonsmooth_clutch_init            ( nonsmooth_clutch_params params);
  void   nonsmooth_clutch_free            ( void * sim);
  int    nonsmooth_clutch_step            ( void * sim, size_t n);
  int    nonsmooth_clutch_step_to         ( void * sim, double now, double comm_step, double nominal_step);
  void   nonsmooth_clutch_save_data       ( void * sim);
  void   nonsmooth_clutch_flush_data      ( void * sim, char * file);
  void   nonsmooth_clutch_save_state      ( void * sim);
  void   nonsmooth_clutch_restore_state   ( void * sim);
  void   nonsmooth_clutch_sync_in         ( void * sim, void * params );
  void   nonsmooth_clutch_sync_out        ( void * sim, void * params );
  double nonsmooth_clutch_get_partial     ( void * sim, void * params, int x, int wrt );
  
  
struct nonsmooth_clutch_params {

  double masses[ 4 ];           /* masses for the 4 bodies: first three on engine side, last one is second plate */
  double first_spring;          /* spring constant for the first set of soft springs */
  double second_spring;         /* spring constant for the second set of stiff springs */
  double plate_pressure;        /* force between the clutch plates */
  double friction_coefficient;  /* friction of the plates */
  double torque_in;		/* applied torque on the input shaft */
  double torque_out;		/* applied torque on the output shaft */
  double step;			/* time step: this moves at fixed step and a change of time step */
  double tau;                   /* Damping rate: this should be approximately twice the time step. */
  double spring_lo[ 2 ];        /* Lower bounds for the motion range of the  two first bodies. */
  double spring_up[ 2 ];        /* Upper bounds for motion range of same. */
  double compliance_1;          /* compliance for first limit: both up and down. */
  double compliance_2;		/* compliance for second limit: both up and down. */
  double plate_slip;		/* clutch slip */
  double x[ 4 ];		/* positions (angles) */
  double v[ 4 ];		/* (angular) velocity */
  double drive_torque;          /* the torque pushing the output shaft:
                                 * output value */
  double constraint_state[ 5 ];              /* diagnostic ouput: state of constraints * */
  double lambda[ 5 ];           /* diagnostic output: constraint forces */
  double constraints[ 5 ];           /* diagnostic output: constraint forces */
  int has_impact;			/* diagnostic output: whether the step had
  an impact */
  
};
  

  
#ifdef __cplusplus
 }
#endif

#endif
