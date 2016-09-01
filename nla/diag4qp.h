#ifndef DIAG4_QP
#define DIAG4_QP

#include "diag4.h"
#include <vector>
#include <limits>
#include <valarray>
#include <algorithm>
#include <assert.h>
#include <types.h>

#define DO_TIME_MEASURE 0
///
///  The greater of two numbers in absolute value.
///
inline bool abs_greater( Real x, Real y){
  return fabs( y ) >= fabs( x );
}

///
///  The lesser of two numbers in absolute value.
///
inline bool abs_lesser( Real x, Real y){
  return fabs( x ) < fabs( y );
}

///
///  Utility.
///  Keeps the four possible steps and indicates which one is chosen.
///  The reason to have a whole class for this is that the logic is
///  confusing.  This keeps the qp class simpler and cleaner.
///
///  The computation of the min steps is done in the qp solver
///
///  TODO: too little precaution is taken against getting an invalid step.
///
struct min_step {
  enum  {INVALID = -1, NATURAL=0, Z_BOUND=1, LOWER=2, UPPER=3 };
  Real steps  [4];              // the four candidates
  size_t  indices[4];           // index of variables. May be (size_t)-1 to signal none. (Well, it can be -1. Not sure what it would mean.)
  int step_type;                // which of the four alternatives
  bool ready;                   // flag telling that computation is over

  /// Ready to go!
  min_step( int _driving, Real theta0, Real theta1 ) { init( _driving, theta0,  theta1 ); }

  /// Initialize in a silly state
  min_step() : min_step( -1, std::numeric_limits<Real>::infinity(), std::numeric_limits<Real>::infinity() ){}

  ///
  /// Prepare the computation: set the two natural steps, and reset the
  /// other two options to infinity.
  ///
  inline void init( size_t _driving, Real theta0, Real theta1 ) {
    ready = false;
    for( size_t i = 0; i < sizeof( steps ) / sizeof( steps[0] ); ++i) {
      indices[ i ] = -1;
    }
    indices[ ( int ) NATURAL ] = _driving;
    indices[ ( int ) Z_BOUND ] = _driving;
    step_type = ( int ) INVALID;
    steps[ ( int ) NATURAL ] = theta0;
    steps[ ( int ) Z_BOUND ] = theta1;
    steps[ ( int ) LOWER ] = std::numeric_limits<Real>::infinity();
    steps[ ( int ) UPPER ] = std::numeric_limits<Real>::infinity();
  }

  ///
  ///  Used as we perform the minimum ratio step.   This is just a running
  ///  'min'  computation, catching the idex of the min as we go along
  ///  TODO:  I cannot see how to use the standard library for this.
  inline void update( Real x, int i, int var  ){
    if ( x < steps[ var ] ) {
      steps[ var ] = x;
      indices[ var ] = i;
    }
  }

  ///
  /// Returns which of the four alternative steps we are taking.
  ///
  inline int get_step_type( ) const {
    return ( int ) step_type;
  }

  ///
  /// This will return the `driving'  variable when we take a natural
  /// pivot, i.e., one which 'frees' a variable and makes its slack vanish.
  ///
  /// Otherwise we get the index of the variable which prevents taking a
  /// full step.
  ///
  inline int get_blocking( ) const {
    return ( int ) indices[ (int) step_type ];
  }

  ///
  /// Return the min step.
  /// The computation  is not done here, though it could be a good idea to
  /// do it as needed.
  inline Real get_min_step( Real pivot ) {
    /// avoid recomputing
    if ( ! ready ){
      step_type = int( std::distance( steps, std::min_element( steps,  steps + sizeof(steps)/sizeof(steps[0] ), abs_greater ) ) );

      ready = true;
    }

    return  pivot * steps[ ( int ) step_type ];
  }
};

///
/// A QP solver using Keller's algorithm
///
///
struct diag4qp {
  enum STATES { EQ = -1, FREE=0, LOWER = 2, UPPER = 4 };
  const Real inf = std::numeric_limits<Real>::infinity();

  qp_diag4 * M;                  /// System matrix
  std::valarray<int> idx;            ///  index set
  std::valarray<Real> w;                    /// slacks
  std::valarray<Real> v;                    /// search direction.


  /// these are variables for the solve which are shared between the
  /// different methods.
  Real pivot;                   /// Whether we are working on a lower bound
                                /// or upper bound: the latter case
                                /// involves a -1 sign
  min_step step;                /// The step
  size_t driving;               /// The driving variable, i.e., the one for
                                /// which are are trying to find a feasible
                                /// slack
  size_t blocking;              /// A blocking variable, i.e., one which
                                /// prevents making the slack of the
                                /// driving variable feasible.
  Real mtt;                     /// The rate at which the slack changes
                                /// with respect to the corresponding
                                /// variable.
  int iterations;               /// Number of iterations for main loop.
  int projection_iterations;       /// Iterations of projections for feasible
                                /// point.
  Real zero_tolerance;          /// How much slack we give for exact zeros.
  bool done;                    /// Done.

  std::chrono::duration<double> solve_time;
  std::chrono::duration<double> projection_time;


  /// Default constructor takes references to dummy variables.  This is a
  /// bit silly since we do not have a resize functionality now.
  diag4qp() :
    M(0), idx(),
    w(), v( 0 ), pivot(0), step(), driving(-1), blocking( -1 ),
    mtt(0.0), zero_tolerance(1e-9) {}


  ///
  /// Standard constructor.
  /// TODO: the `const'  has to be dropped if we want to reuse an instance
  /// of this class, both here and in declarations.
  /// Also, the 'z' vector should be a reference to avoid copy going in and
  /// out.
  diag4qp(qp_diag4 * _m) :
    M( _m ), idx( _m->size() ),
    w( _m->size() ), v( _m->size() ), pivot( 0 ), step(), driving( -1 ), blocking( -1 ), mtt( 0.0 ),
    zero_tolerance(1e-9), done(false) {
  }

  inline void init( ){
    driving   = (size_t ) -1; // driving is used as an index. Would be bad to index with -1.
    blocking  = (size_t ) -1;
    mtt = 0.0;
    pivot = 0;
  }


  ///
  ///  Get the number of variables
  ///
  size_t size() const {
    return idx.size();
  }

  ///
  /// Compute how the z vector changes for a unit change in the driving
  /// variable
  ///
  bool get_search_direction( const std::valarray<Real> & q ){
    if ( v.size() != q.size() )
      v.resize( q.size() );

    M->get_search_direction( driving, v, mtt);
    return true;
  }

  size_t get_free(){
    size_t n = 0;
    for ( size_t i = 0; i < idx.size(); ++i )
      n += ( idx[ i ] <= FREE );
    return n;
  }
  ///
  ///  Rearrange the index set to be consistent with the step just taken.
  ///
  bool do_pivot(){
    blocking = step.get_blocking();
    if ( step.get_step_type() == min_step::NATURAL ){
      idx[ driving ] = FREE;
      blocking = driving;

      M->toggle_active( driving );

    }else if( step.get_step_type() == min_step::Z_BOUND ){
      idx[ blocking ] = ( pivot > 0.0 ) ? UPPER : LOWER;
      blocking = driving;

    }
    else {
      assert( blocking != (size_t) -1 );
      
      idx[ blocking ] = step.get_step_type();
      M->toggle_active( blocking );
    }
    return true;
  }


  ///
  ///  Just compute   w = M*z + q
  ///
  bool get_complementarity_point( const std::valarray<Real>& q,
                                  const std::valarray<Real> & lo, const std::valarray<Real> & up,
                                  std::valarray<Real> & z ){


    /// this might be an overkill: should check first if needed.
    for (size_t i = 0; i< z.size();++i)
    {
      if (idx[i] == (int)UPPER )
        z[i] = up[i];
      if (idx[i] == (int)LOWER)
        z[i] = lo[i];
    }
    //z[ idx==(int)UPPER ] = up[ idx == (int) UPPER ];
    //z[ idx==(int)LOWER ] = lo[ idx == (int) LOWER ];
    w = q;
    M->multiply(z, w, 1.0, 1.0);
    return true;
  }

  ///
  /// This will find the largest slack and determine whether or not this
  /// corresponds to a lower or upper bound.
  ///
  bool get_driving_variable(){

    /// fine the most violated slack
    Real lower_max = 0;
    size_t lower_index = (size_t)-1;
    Real upper_max = 0;
    size_t upper_index = (size_t)-1;
    for ( size_t i = 0;  i < w.size(); ++i ){
      if ( idx[ i ] == LOWER ){
        if ( w[ i ] < lower_max ){
          lower_max = w[ i ];
          lower_index = i ;
        }
      }
      if ( idx[ i ] == UPPER ){
        if ( w[ i ] > upper_max ){
            upper_max = w[ i ];
            upper_index = i ;
        }
      }
    }

    if ( std::abs( upper_max ) > std::abs( lower_max ) ){
      driving = upper_index;
    }
    else {
      driving = lower_index;
    }

//    assert( driving >= 0 &&  driving < idx.size() );

    done = true;
    pivot = 1;

    /// Now check what we have found a variable that's worth chasing
    if  ( (driving != (size_t) -1 ) ) {
      done =
        ( idx[ driving ] <= FREE ) ||
        ( idx[ driving ] == LOWER && ( w[ driving ] > - zero_tolerance ) )  ||
        ( idx[ driving ] == UPPER && ( w[ driving ] <   zero_tolerance ) ) ;

      pivot = ( idx[ driving ] == LOWER ) ? Real( 1.0 ) : Real( -1.0 );
    }


    return done;

  }

  ///
  ///  Figure out how far we can push the driving variable without breaking
  ///  other bounds.
  ///
  bool get_step_length( const std::valarray<Real> & lo, const std::valarray<Real> & up, const std::valarray<Real> & z ){

    /// TODO: mtt could be zero for the case of a degenerate matrix.  This
    /// calls for 2x2 pivots but for the matrices considered, I doubt this
    /// can happen since they have full rank.
    step.init(driving,
              ( mtt   > 0 )? -pivot * w[ driving ] / mtt : std::numeric_limits<Real>::infinity(),
              ( pivot > 0 )?  up[ driving ] - z[ driving ] : z[ driving ] - lo[ driving ] );

    for ( size_t i = 1; i < idx.size(); i+= 2 ){

      if ( idx[ i ] == FREE ) {

        if      (  pivot * v[ i ]  < -  zero_tolerance  ){
          /// these are the variables which are brought closer to a lower
          ///  bound, if any, with curent search direction
          step.update( fabs( ( z[ i ]  - lo[ i ] ) / v[ i ] ), (int)i, min_step::LOWER);
        }
        else if (  pivot * v[ i ]  >   zero_tolerance  ){
          /// these are the variables which are brought closer to an upper
          /// bound, if any, with the current search direction.
          step.update( fabs( ( up[ i ] -  z[ i ]   ) / v[ i ] ), (int)i, min_step::UPPER);
        }
      }
    }

    step.get_min_step( pivot );

    return true;

  }

  inline int project_feasible(  const std::valarray<Real> & q ,
                                const std::valarray<Real> & lo, const std::valarray<Real> & up,
                                std::valarray<Real> &  z ){

    
    //idx = FREE;
    //idx[ (lo == -inf ) && ( up == inf ) ] = EQ;

    for ( size_t i = 0; i < idx.size(); ++i){
      if ( ( lo[ i ] == -inf ) && ( up[ i ] == inf )  )
        idx[ i ] = EQ;
      else
        idx[ i ] = FREE;
    }

    M->active = true;
    M->dirty = true;

    projection_iterations = 0;

#if DO_TIME_MEASURE
    std::chrono::time_point<std::chrono::system_clock> f_start = std::chrono::system_clock::now();
#endif
    do {
      ++projection_iterations;
      M->solve_subproblem(q, lo, up, idx, z);
    } while ( active_set_switch( lo, up,  z ) );
    get_complementarity_point( q, lo, up, z );
#if DO_TIME_MEASURE
     projection_time = std::chrono::system_clock::now() - f_start;
#endif
#if 0
     std::cerr << " ----------------------------------- "  << std::endl;
     std::cerr << "Projection iterations:  " << projection_iterations << "  time: " << (size_t) ( 1e6 * projection_time.count()) << " us"  << std::endl;
    std::cerr << "Active variables: " << active_count() << ", feasibility error: " << get_feasibility_error(lo, up, z  )
              << " complementarity error: "  << get_complementarity_point( q, lo, up,  z ) << std::endl;

    std::cerr << " ----------------------------------- "  << std::endl;
#endif
    return iterations;
  }

  inline int active_set_switch( const std::valarray<Real> & lo, const std::valarray<Real> & up, const std::valarray<Real> &  z ){
    /// this rule will do:
    /// 1  : keep all bounded variables at their bound
    /// 2  : remove active variables which are outside the bounds
    /// 3  : keep everything else as it is

    int changes = 0;


    for ( size_t i = 0; i < idx.size(); ++i ){

      int x = idx[ i ];

      idx[ i ] =
        ( int ) EQ * ( int )( x == EQ ) // first case: EQ variables stay
                                        // where they are
         + ( int ) ( x == FREE ) *      // here a free variable will turn
                                        // either to LOWER or UPPER as
                                        // needed
        (
          ( int ) LOWER  * ( int ) ( z[ i ] <   ( lo[ i ]  - zero_tolerance ) )
         +( int ) UPPER  * ( int ) ( z[ i ] >   ( up[ i ]  + zero_tolerance ) )
          )
        + ( int ) LOWER * ( int ) ( x == LOWER ) /// a variable at lower
                                                 /// bound cannot have been
                                                 /// caught already: keep
                                                 /// it's state 
        + ( int ) UPPER * ( int ) ( x == UPPER ); /// same as above for
                                                  /// variables at lower
                                                  /// bounds
      if ( x != idx[ i ] ){                       /// keep track of the
                                                  /// number of variables
                                                  /// which have changed
                                                  /// states 
        M->toggle_active( i );
        changes +=  1;
      }
    }

    return changes;             /// If there were state changes, they the
                                /// projection process will iterate once
                                /// more. 

  }

  size_t get_at_bound() {
    size_t at_bound = 0;
    
    for (size_t i = 0; i < idx.size(); i += 2) {
      at_bound +=  ( size_t )  ( idx[ i ] >  FREE );
    }
    return at_bound;
  }

  size_t get_inequalities() {
    return ( size_t ) idx.size() /  2 ;
  }

  Real get_percent_stick() {
    return ( Real )  100 * ( int ) get_at_bound()   /  ( Real ) get_inequalities();
    
  }

///
/// Simple feasible point. No need to do something more complicated since
/// factorization update and downdate is very cheap.
///
  inline void get_feasible_point( const std::valarray<Real> & q, const std::valarray<Real> & lo,   std::valarray<Real> & up,  std::valarray<Real> & z  ){

      idx[ (lo == -inf ) && ( up == inf ) ] = EQ;
      idx[ (lo != -inf ) && ( up != inf)  ] = LOWER;
      idx[ (lo != -inf ) && ( up == inf)  ] = LOWER;
      idx[ (lo == -inf ) && ( up != inf)  ] = UPPER;

      M->set_active( idx );
      M->solve_subproblem(q, lo, up, idx, z);

      return;

    }

    inline size_t active_count( ) const {
      size_t count = 0;
      for ( size_t i = 0 ; i < idx.size(); ++i ){
        count += (size_t) ( idx[ i ] <= 0 );
      }
      return count;

    }

  inline bool is_feasible( const std::valarray<Real> & lo, const std::valarray<Real> & up,  std::valarray<Real> & z  )  const {
      bool ret = true;
      for ( size_t i = 0; i < z.size() && ret; ++i ){
        ret = ret && ( z[ i ] >= lo[ i ]-zero_tolerance  && z[ i ] <= up[ i ] + zero_tolerance );
      }
      return ret;
    }

  inline Real get_feasibility_error( const std::valarray<Real> & lo, const  std::valarray<Real> & up, std::valarray<Real> & z  ) const {
      Real err = Real( 0.0 );

      for ( size_t i = 0; i < z.size(); ++i ){
        err +=
          std::min( Real(0.0),  z[ i ] -  lo[ i ] )
          * std::min( Real(0.0),  z[ i ] -  lo[ i ] )
          +
          std::min( Real( 0.0 ) , up[ i ] -  z[ i ] ) * std::min( Real( 0.0 ) , up[ i ] -  z[ i ] );
      }

      return sqrt( err );
    }


  inline bool feasible( const std::valarray<Real> & lo, const std::valarray<Real> & up, const std::valarray<Real> & z  ) const {
      bool ret = true;

      for ( size_t i = 1; i < z.size(); i += 2 )
        ret = ret &&  ( z[ i ] >= lo[ i ] &&  z[ i ] <= up[ i ] );

      return ret;
  }

    ///
    ///  This could of course take arguments so the same class could be
    ///  references as class members.
    ///
  inline bool solve( const std::valarray<Real>& q,   const std::valarray<Real>& lo,
                     const std::valarray<Real>& up, std::valarray<Real> & z )
    {
        blocking = -1;               // safe starting point
        driving = -1;               // safe starting point
        iterations = 0;
        z = -q;
        project_feasible(q, lo, up, z );
        get_complementarity_point( q, lo, up,  z );

#if DO_TIME_MEASURE
      auto f_start = std::chrono::system_clock::now();
#endif

        ///  We get a new pivot index each time the pivot is not successful.
        ///  This avoid having two nested while-loops
        ///  And look at this!  Here's the solver, and it took hundreds of line
        ///  to construct that little for-loop.
        /// \todo Should have a guarding stop criterion here with either
        ///  complementarity error or max iterations
        while ( ( driving != blocking ) ||  ! get_driving_variable() ) {
          iterations++;
          if ( iterations >  q.size() ){
            std::cerr << "All gone south " << std::endl;
            break;
            
          }
          get_search_direction( q );
          get_step_length( lo, up, z );
          M->do_step( z, w, v, step.get_min_step( pivot ), driving);
          do_pivot();
        }
#if DO_TIME_MEASURE
      solve_time = std::chrono::system_clock::now() - f_start;

      std::cerr << "Solve time.  Size: " <<  z.size() << " time: " <<  (size_t) ( 1e6 * solve_time.count()) << " us"   << "  iterations " << iterations << std::endl;
      std::cerr << "Number of free variables:  " << get_free() << " of " << z.size() << std::endl;
#endif

      return true;
    }

    ///
    ///  See how well the solver did
    ///
    inline int get_iterations() const { return iterations;}


    ///
    ///  This will check if all complementarity conditions are satisfied.
    ///
    Real get_complementarity_error ( const std::valarray<Real> & z ){

      Real err;

      for ( size_t i = 0; i < z.size(); ++i ){
        if ( idx[ i ] == FREE ) {
          Real x = w[ i ];
          err += x * x;
        }
        else if ( idx[ i ] == LOWER ){
          Real x = - std::min( w[ i ], zero_tolerance );
          err += x * x;
        } else if ( idx[ i ] == UPPER ){
          Real x = std::max( w[ i ], -zero_tolerance );
          err += x * x;
        }
      }
      return sqrt( err  / ( Real ) z.size() );
    }

  std::ostream &   report_stats( std::ostream & io,
                                 const std::valarray<Real> & z,
                                 const std::valarray<Real> & lo,
                                 const std::valarray<Real> & up){
    std::ios::fmtflags o = io.flags();
    io.precision( 3 );
    
    
    io  << "QP report: " << std::endl
      << "Iterations:      " << iterations << std::endl
        << "Complementarity error:" <<  get_complementarity_error (  z ) << std::endl
        <<  "IDX   W    LOWER    Z    UPPER " << std::endl;
    for ( size_t i = 0; i < z.size(); ++i ) {
      io  <<  idx[ i ] << "   " << w[ i ] << "   " <<  lo[ i ] << "   "  << z[ i ]  << "   "  << up[ i ] << std::endl;
      
    }
    
    return io;
    
  }
};


//  There you go!  Read all the way to the end, now, didn't you?
//  Congratulations!  Or maybe you skipped everything?



#endif
