#include "nonsmooth_clutch.h"
#include <stdlib.h>
#include <iostream>
using namespace std;
#include "banded.h"
#include "qp_solver.h"


static const double infinity  = std::numeric_limits<Real>::infinity() ;


using namespace std;



///
/// We store the matrix in a vector of arrays so that the first array
/// contains the diagonal, the second contains the first subdiagonal, etc. 
///

typedef double Real;

struct clutch_sim: public nonsmooth_clutch_params {
  
  nonsmooth_clutch_params saved_parameters;
  
  
  banded_matrix M; // 9x9, bandwidth of 3, bisymmetric
  valarray<double> lower;       /// lower bounds for the complementarity problem
  valarray<double> upper;       /// upper bounds for the complementarity problem
  valarray<double> rhs;         /// buffer to store the RHS
  valarray<double> z;           /// buffer for the QP solver to work with
  double gamma;                 /// Infamous damping factor 
  valarray<double> g;           /// constraint violations
  valarray<double> gdot;        /// constraint velocity
  
  qp_solver QP;                    /// An instance of the QP solver.  


  /// straight copy of a parameter struct and initialization
  clutch_sim( nonsmooth_clutch_params & p) : clutch_sim() {

    * ( ( nonsmooth_clutch_params * ) this ) = p;
    
    init();
    
  }

  /// contorted way to explicitly initialize all parameters
  clutch_sim( double _M[4], double K1, double K2, double F, double MU,
              double TIN, double TOUT, double H, double TAU,
              double LO[2], double UP[2], 
              double COMP1 = 1e-8, double COMP2 = 1e-8,
              double COMP3 = 1e-8  ) : clutch_sim()
  {
    nonsmooth_clutch_params p = {
      { _M[0], _M[1], _M[2], _M[3]},
      K1 ,
      K2 ,
      F ,
      MU ,
      TIN,
      TOUT,
      H ,  
      TAU ,
      { LO[ 0 ],LO[ 1 ] },
      { UP[ 0 ], UP[ 1 ] },
      COMP1 ,  COMP2 ,  COMP3};
    * ( ( nonsmooth_clutch_params * ) this ) = p;

    init();
    
  }

  /// Default initializer, but without setting any parameters.  This is meant to be chained. 
  clutch_sim() : M( 9, 4, true ),
                 lower( -infinity, 9 ),
                 upper(  infinity, 9 ),
                 rhs ( 0.0, 9 ), z( 0.0, 9), QP( &M )
  {
    init();
  }

  /// Construct the matrix according to current parameters.
  inline void init(){

    set_limits();
    reset();
    
    M.negated[ 0 ] =  1.0;     // body 1
    M.negated[ 1 ] = -1.0;      // low range
    M.negated[ 2 ] = -1.0;      // up range
    M.negated[ 3 ] =  1.0;     // body 2
    M.negated[ 4 ] = -1.0;      // low range
    M.negated[ 5 ] = -1.0;      // up range
    M.negated[ 6 ] =  1.0;     // body 3
    M.negated[ 7 ] = -1.0;      // friction
    M.negated[ 8 ] =  1.0;     // body 4 

  }

  /// This will rebuild the matrix according to current time step, and
  /// update the bounds corresponding to plate friction
  inline void reset(){
    
    set_timestep( step );       
    set_pressure( plate_pressure );

  }

  inline void set_limits(){

    lower[ 1 ] = 0.0;           // first lower limit
    upper[ 1 ] = infinity;         
    
    lower[ 2 ] = 0.0;           // first upper limit
    upper[ 2 ] = infinity;      // inequality for first upper limit

    lower[ 4 ] = 0.0;              // inequality for second lower limit
    upper[ 4 ] = infinity;         // inequality for second lower limit

    lower[ 5 ] = 0.0;              // inequality for second upper limit
    upper[ 5 ] = infinity;         // inequality for second upper limit
    
  }

  inline void set_timestep( double new_step ){

    step = new_step;
    gamma =  Real( 1.0 ) / ( 1.0 + 4 * tau / step  );
    build_matrix();

  }

  /// A method is needed here so the bounds are updated.
  inline void set_pressure( double p  ){

    plate_pressure = p;
    lower[ 7 ] = - step * friction_coefficient * plate_pressure;     
    upper[ 7 ] =   step * friction_coefficient * plate_pressure;
    
  }

  /// This sets all the coefficients according to time step, 
  inline void build_matrix(){

    double EE =  ( gamma *  4.0  /  ( step * step ) );
    double kk1 = first_spring / EE;
    double kk2 = second_spring / EE;
   
    double damping = 0;         // TODO: this would be an additional damping on
                                // each body and is probably a bad idea.
    
    
/// Build the matrix
    M[ 0 ][ 0 ] =  masses[ 0 ] + kk1;       // first mass including spring
    M[ 0 ][ 0 ] +=  step * damping;
    
    M[ 0 ][ 1 ] = -compliance_1 * EE;       // lower bound for first spring
    M[ 0 ][ 2 ] = -compliance_1 * EE;       // upper bound for first spring

    M[ 0 ][ 3 ] =  masses[ 1 ] + kk1 + kk2; // second mass including both springs
    M[ 0 ][ 3 ] +=  step * damping;

    M[ 0 ][ 4 ] = -compliance_2 * EE;       // lower bound for second spring
    M[ 0 ][ 5 ] = -compliance_2 * EE;       // upper bound for second spring

    M[ 0 ][ 6 ] =  masses[ 2 ] +  kk2;      // third mass including spring
    M[ 0 ][ 6 ] +=  step * damping;

    M[ 0 ][ 7 ] = -plate_slip / step;       // dry friction constraint

    M[ 0 ][ 8 ] =  masses[ 3 ];             // fourth mass: no spring
    M[ 0 ][ 8 ] +=  step * damping;

    /// first two masses bound constraints
    M[ 1 ][ 0 ] =  Real( 1.0 );
    M[ 1 ][ 2 ] =  Real( 1.0 );

    M[ 2 ][ 0 ] =  Real(-1.0 );
    M[ 2 ][ 1 ] =  Real(-1.0 );

    /// this connects the second and third mass via bound constraints
    M[ 1 ][ 3 ] =  Real( 1.0 );
    M[ 1 ][ 5 ] =  Real( 1.0 );
    
    M[ 2 ][ 3 ] =  Real(-1.0 );
    M[ 2 ][ 4 ] =  Real(-1.0 );
    
    /// this connects the third and fourth mass via dry friction
    M[ 1 ][ 6 ] =  Real( 1.0 );
    M[ 1 ][ 7 ] =  Real(-1.0 );

    /// spring constant between the first two masses
    M[ 3 ][ 0 ] = -kk1;
    /// spring constant between the second and third
    M[ 3 ][ 3 ] = -kk2;
    
/**

The by bisymmetric version of this is: 
   m  -1   1  -K1  0   0   0   0   0
   1   e   0  -1   0   0   0   0   0    -- row for  lower bound
  -1   0   e   1   0   0   0   0   0    -- duplicated row for upper bound
  -K1  1  -1   m  -1   0   K2  0   0
   0   0   0   1   e   0  -1   0   0     -- row for  lower bound
   0   0   0  -1   0   e   1   0   0     -- duplicated row for upper bound
   0   0   0  -K2  1  -1   m  -1   0
   0   0   0   0   0   0   1   e  -1
   0   0   0   0   0   0   0   1   m

The symmetrized version, lower triangle only
   m   *   *   *   *   *   *   *   *
   1  -e   *   *   *   *   *   *   *    -- row for  lower bound
  -1   0  -e   *   *   *   *   *   *    -- duplicated row for upper bound
  -K1 -1   1   m   *   *   *   *   *
   0   0   0   1  -e   *   *   *   *     -- row for  lower bound
   0   0   0  -1   0  -e   *   *   *     -- duplicated row for upper bound
   0   0   0  -K2 -1   1   m   *   *
   0   0   0   0   0   0   1  -e   *
   0   0   0   0   0   0   0  -1   m

*/

  }


  /// Compute constraint violations and their speeds as well
  inline void get_constraints(){
 
    /// displacements for the two springs
    double dx1 = x[0] - x[1];
    double dx2 = x[1] - x[2];
    double dv1 = v[0] - v[1];
    double dv2 = v[1] - v[2];
    double dv3 = v[2] - v[3];

    if ( g.size() != 5 ) { g.resize( 5 ) ; }
    if ( gdot.size() != 5 ) { gdot.resize( 5 ) ; }
    /// constraint violations for contacts
    g   [ 0 ]  =  dx1  - spring_lo[ 0 ];
    gdot[ 0 ]  =  dv1;
    
    g   [ 1 ]  = -dx1  + spring_up[ 0 ];
    gdot[ 1 ]  = -dv1;

    
    g    [ 2 ] =  dx2  - spring_lo[ 1 ];
    gdot [ 2 ] =  dv2;
    
    g    [ 3 ] = -dx2  + spring_up[ 1 ];
    gdot [ 3 ] = -dv2;
    
  }


  /// Check if there is an impact.  
  inline bool  impact(){
    bool i(false);

    double threshold_g(1e-3);
    double threshold_v(1e-3);

    /// Everything has been rigged so that all bound constraints have
    /// multipliers bounded *below*

    i =  i || ( g[ 0 ] < threshold_g  &&  gdot[ 0 ] < threshold_v );
    i =  i || ( g[ 1 ] < threshold_g  &&  gdot[ 1 ] < threshold_v );
    i =  i || ( g[ 2 ] < threshold_g  &&  gdot[ 2 ] < threshold_v );
    i =  i || ( g[ 3 ] < threshold_g  &&  gdot[ 3 ] < threshold_v );

    /// Here we need to cover both positive and negative bounds. 
    i =  i ||  abs( g[ 4 ]   >   threshold_g  &&  gdot[ 4 ] > threshold_v );
    i =  i ||  abs( g[ 4 ]   <  -threshold_g  &&  gdot[ 4 ] < -threshold_v );

    return i;
    
  }
  

  /// Right hand side for impacts.  This implements a completely inelastic model. 
  inline void  get_rhs_impact(){
    

    rhs[ 0 ] = - ( masses[ 0 ] * v[ 0 ] );

    rhs[ 1 ] = 0.0;
    
    rhs[ 2 ] = 0.0;
    
    rhs[ 3 ] = - ( masses[ 1 ] * v[ 1 ] );
    
    rhs[ 4 ] = 0.0;

    rhs[ 5 ] = 0.0;

    rhs[ 6 ] = - ( masses[ 2 ] * v[ 2 ] );

    rhs[ 7 ] = 0.0;

    rhs[ 8 ] = - ( masses[ 3 ] * v[ 3 ] );
    
  }
  

  /// Here we compute the right hand size so the problem to solve is
  ///
  /// M * z  + rhs = w
  ///
  /// which means that it is the *negative* of the standard one we would
  /// use if there were no inequalities
  ///
  inline void  get_rhs(){
    
    /// displacements for the two springs
    double dx1 = x[0] - x[1];
    double dv1 = v[0] - v[1];
    double dx2 = x[1] - x[2];
    double dv2 = v[1] - v[2];   

    /// scaling of spring constants
    double step4 = step / double( 4.0 );
     
    /// spring forces
    double fk1 = -  step * first_spring  * ( dx1  - step4 * dv1 );
    double fk2 = -  step * second_spring * ( dx2  - step4 * dv2 );
    
    
    /// start with the masses
    rhs[ 0 ] = - ( masses[ 0 ] * v[ 0 ] + fk1         +  step * torque_in  );
    
    rhs[ 3 ] = - ( masses[ 1 ] * v[ 1 ] - fk1  + fk2                       );
    
    rhs[ 6 ] = - ( masses[ 2 ] * v[ 2 ]        - fk2                      );

    rhs[ 8 ] = - ( masses[ 3 ] * v[ 3 ]               + step * torque_out );

    /// Now constraints;

    get_constraints();
    
    rhs[ 1 ] =  - ( gamma * ( ( -4.0 / step ) * g[ 0 ] + gdot[ 0 ] ) );
    rhs[ 2 ] =  - ( gamma * ( ( -4.0 / step ) * g[ 1 ] + gdot[ 1 ] ) );

    rhs[ 4 ] =  - ( gamma * ( ( -4.0 / step ) * g[ 2 ] + gdot[ 2 ] ) );
    rhs[ 5 ] =  - ( gamma * ( ( -4.0 / step ) * g[ 3 ] + gdot[ 3 ]) );
    
    /// and the friction constraint
    rhs[ 7 ] =  0;

    return;
    
  }

  ///
  /// Allows multiple steps to be taken to reach a sync point.  If the
  /// nominal step is 0, the current step is used and we do our best to get
  /// past the final time.  But that might actually be too far.
  ///
  void step_to( double now, double final, double nominal_step = 0.0){

    if ( nominal_step ) {
      step = nominal_step;
      reset();
    }

    size_t N = ( size_t ) ceil( ( final - now ) / step );
    
    do_step( N );

  }

  /// Move forward in time with current step. 
  void do_step( size_t n ){

    M.set_active();           // Activate everything and let the solver
                              // figure out what to do

    for ( size_t t = 0; t < n; ++t ){

      
      get_constraints();        // TODO: this is duplicated if there is no impact!
      has_impact = impact();
      
      if (   has_impact  ){

        get_rhs_impact();
        
        QP.solve( rhs, lower, upper, z);
        
        v[ 0 ] = z[ 0 ];
        v[ 1 ] = z[ 3 ];
        v[ 2 ] = z[ 6 ];
        v[ 3 ] = z[ 8 ];     
      }
      
      get_rhs();      
      M.set_active();           // let the solver figure out what to do
      QP.solve( rhs, lower, upper, z);
      //QP.report_stats( std::cerr , z, lower, upper);
      
      v[ 0 ] = z[ 0 ];
      v[ 1 ] = z[ 3 ];
      v[ 2 ] = z[ 6 ];
      v[ 3 ] = z[ 8 ];

      /// The for-loop here is because we don't use valarray for x, and
      /// that's to help with initialization from C
      for ( size_t i = 0; i < sizeof( x ) / sizeof( x[ 0 ] ); ++i ){ 
        x[ i ] += step * v[ i ];
      }

      lambda[ 0 ] = z[ 1 ] / step ;
      lambda[ 1 ] = z[ 2 ] / step;
      lambda[ 2 ] = z[ 4 ] / step;
      lambda[ 3 ] = z[ 5 ] / step;
      lambda[ 4 ] = z[ 7 ] / step;
      
      constraint_state[ 0 ]  = QP.idx[ 1 ];
      constraint_state[ 1 ]  = QP.idx[ 2 ];
      constraint_state[ 2 ]  = QP.idx[ 4 ];
      constraint_state[ 3 ]  = QP.idx[ 5 ];
      constraint_state[ 4 ]  = QP.idx[ 7 ];
       
      constraints[ 0 ]  = g[ 0 ];
      constraints[ 1 ]  = g[ 1 ];
      constraints[ 2 ]  = g[ 2 ];
      constraints[ 3 ]  = g[ 3 ];
      constraints[ 4 ]  = g[ 4 ];
       
      
      drive_torque = z[ 7 ] / step ;
      
    }

  }
  ///
  /// Blind copy of everything.
  /// 
  inline void sync_state_out( void * p ){

    *( ( nonsmooth_clutch_params * ) p )  = * ( ( nonsmooth_clutch_params * ) this );

  }

  
  ///
  /// Blind copy of everything.
  /// 
  inline void sync_state_in( void * p ){

    nonsmooth_clutch_params * q = ( nonsmooth_clutch_params * ) p;
    bool go = ( q->step != step );
    
    * ( ( nonsmooth_clutch_params * ) this ) = * q;
    
    /// Reset if time steps differ
    if ( go ) { 
      reset();
    }
    

  }

  ///
  /// None of the data in the class other than the parameters have to be
  /// saved.
  /// 
  inline void save_state(){

    saved_parameters =   * ( ( nonsmooth_clutch_params *) this );
    
  }

  ///
  ///  The reset is needed since the time step in the saved state might be different than current.  A check could be performed. 
  /// 
  inline void restore_state(){
    bool go = ( step != saved_parameters.step );
    
    * ( ( nonsmooth_clutch_params *) this ) = saved_parameters;

    if ( go ){
      reset();
    }
    
  }
};













/** Now the C API for all this
 *
 */













void * nonsmooth_clutch_init            ( nonsmooth_clutch_params params){

  clutch_sim *sim = (clutch_sim * )  new clutch_sim( params );
  return sim;
  
}

void   nonsmooth_clutch_free            ( void * sim){

  delete  ( clutch_sim * ) sim;
}


///  Return value should be a diagnostic flag but no such thing yet
int    nonsmooth_clutch_step            ( void * sim, size_t n){

  clutch_sim *s = ( clutch_sim * ) sim;
  s->do_step( n );
  return 0;
  
}



int    nonsmooth_clutch_step_to         ( void * sim, double now, double comm_step, double nominal_step){


  clutch_sim *s = ( clutch_sim * ) sim;
  s->step_to( now, comm_step, nominal_step);
  return 0;
  
};


void   nonsmooth_clutch_save_data       ( void * sim){

  clutch_sim *s = ( clutch_sim * ) sim;

  cerr << "Nonsmooth_clutch_save_data not implemented.  Crashing impolitely. " << endl;
  abort();
  
}

void   nonsmooth_clutch_flush_data      ( void * sim, char * file){

  clutch_sim *s = ( clutch_sim * ) sim;

  cerr << "Nonsmooth_clutch_flush_data not implemented.  Crashing impolitely. " << endl;
  abort();

}

void   nonsmooth_clutch_save_state      ( void * sim) {

  clutch_sim *s = ( clutch_sim * ) sim;
  s->save_state();
  

}


void   nonsmooth_clutch_restore_state      ( void * sim) {

  clutch_sim *s = ( clutch_sim * ) sim;
  s->save_state();
  

}

void   nonsmooth_clutch_sync_in         ( void * sim, void * params ){

  clutch_sim *s = ( clutch_sim * ) sim;
  s->sync_state_in( params );
  
}

void   nonsmooth_clutch_sync_out         ( void * sim, void * params ){

  clutch_sim *s = ( clutch_sim * ) sim;
  s->sync_state_out(  params );
  
}


double nonsmooth_clutch_get_partial     ( void * sim, void * params, int x, int wrt ){

  cerr << "nonsmooth_clutch_get_partial is not implemented.  Crashing impolitely. " << endl;
  cerr << "Because this is a nonsmooth model, we probably need a numerical derivative anyway." << endl;
  abort();

}




