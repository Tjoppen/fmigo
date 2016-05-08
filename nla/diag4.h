/*
  Copyright 2007-2015. Algoryx Simulation AB.

  All AgX source code, intellectual property, documentation, sample code,
  tutorials, scene files and technical white papers, are copyrighted, proprietary
  and confidential material of Algoryx Simulation AB. You may not download, read,
  store, distribute, publish, copy or otherwise disseminate, use or expose this
  material without having a written signed agreement with Algoryx Simulation AB.

  Algoryx Simulation AB disclaims all responsibilities for loss or damage caused
  from using this software, unless otherwise stated in written agreements with
  Algoryx Simulation AB.
*/


#ifndef DIAG4
#define DIAG4

#include <numeric>
/**
   \mainpage The mainpage documentation

   Wire contact solver.
*/


#include <iostream>
#include <vector>
#include <valarray>
#include <chrono>
#include <ctime>
#include <functional>
#include <assert.h>
#include <types.h>


/// needed for linking with octave
#ifdef OCTAVE
class ColumnVector;
class octave_value_list;
class octave_scalar_map;
#endif



/**
 *
 * Block LDLT decomposition.
 * All blocks are 2x2
 * Diagonal blocks have one of the two form before factorization:
 * [a 1]      or  [a 0]
 * [1 e]          [0 1]
 *
 *  These account for mass and friction and the Jacobian is just 1.
 *  a = mass + (h^2/4) * ( k1 + k2)
 *  where k1,k2 are the left and right spring constant, i.e.,
 *  tension / length
 *
 *  The off-diagonal blocks are block-subdiagonal and have the form
 *
 *  [b 0]  before and     [b f]  after factorization
 *  [0 0]                 [0 0]
 *
 *  These account for elasticity and b  = (h^2/4)k  where k is the spring
 *  constant from the coupling to the *previous* particle.
 *
 *  We compute and store the inverse of the diagonal blocks
 *  [e f]   =   ( 1 / (e*h - g*h) ) * [ h -g]
 *  [g h]                             [-f  e]
 *
 *  We don't factor in place since we need to update and downdate this
 *  factorization by changing between one of the two forms of the diagonal
 *  block.
 *
 *  In order to reset if needed, one must downdate and then update the
 *  first equation.
 *
 *
 * Assumption made here is that we are simulating a wire which is anchored
 * at both ends.  Each particle is linked to the previous and next one with
 * potentially different spring constants.  These are tension/length in
 * practice.
 *
 * NOTE: One could unhook the first and last particle but the code then
 * needs small revision to insert a switch to activate or deactivate the
 * anchor.
 */

struct qp_diag4 {
  ///
  /// Enumeration of the states of variables as needed in the lcp solver.
  /// This is intrusive but yet needed for various operations best
  /// encapsulated here than in the qp class itself, e.g., `solve principal
  /// subproblem'  for instance.
  ///
  /// EQ    is for variables which have both infinite upper and lower bounds
  /// FREE  is for variables which are not currently at their bound
  /// TIGHT is for variables currently clamped at one of their bounds
  /// UPPER is for variables currently clamped at their upper bound
  /// LOWER is for variables currently clamped at their lower bounds
  ///
  enum states:int {EQ = -1, FREE=0, LOWER = 2, TIGHT = 2,  UPPER = 4, ALL = 6 };
  bool bisymmetric;             /// \brief whether we solve the bisymmetric problem



  bool dirty;                   /// \brief whether data was changed and matrix
                                /// needs re-factorization.


                                ///
                                /// NOTE: we use symmetric representation of bisymmetric matrices since
                                /// that's all the LCP solver can handle, but symmetric representation is
                                /// more efficient.
  Real  flip_sign;              /// Negative for the bisymmetric case.
                                ///
                                /// Flipping signs is delegated to derived class since formats can vary:
                                /// variables corresponding to Lagrange multipliers have their signs
                                /// flipped.
                                ///

  inline virtual void flip( std::valarray<Real> & x ) = 0; //! \brief Flips the signs of all variables which need it

///
/// Whether or not a variable is active, i.e., the variable is free (solved
/// for) or not.  Lagrange multiplier variables are the only ones that
/// switch on and off as decided by the LCP solver.
///
  std::valarray<bool> active;

  std::valarray<Real> tmp_sub;    //! \brief a temporary array for multiply and solve
  std::valarray<Real> tmp_search; //! \brief  a temporary array for the search  direction method




///
/// Minimal constructor to the the bisymmetric state and allocate memory
/// for the active state as well as tmp arrays.
///
  qp_diag4(size_t _n, bool b  = false, bool d = true  ) :
    bisymmetric( b ), dirty( d ), flip_sign( ( b ) ? Real(-1.0) : Real(1.0) ),
    active(true, _n ), tmp_sub( _n ), tmp_search( _n ) { }


  virtual size_t size() const { return active.size() ; } //! \brief Number of variable

///
/// Self-explanatory: used by LCP solver, and is *variable* based, not
/// *block* based.  Subclasses have `blockwise'  version of these where
/// the index is the index of a Lagrange multiplier, not just a variable.
///
  inline bool toggle_active( size_t i ){
    dirty = true;
    active[ i ] = ! active[ i ] ;
    return active[ i ];
  }

///
/// self-explanatory: for LCP solver: variable based, not  block  based.
///
  inline size_t set_active( const std::valarray<int>& idx ){
    dirty = true;
    active[ idx <= 0 ] = true;
    return active.size();
  }

/// Factorize or update/downdate a factorization if  variable
///   is
/// specified.  If so, the variable will toggle between active and
/// inactive, and the factorization will restart there, updating what's
/// already done.
  virtual void factor( int variable = -1) = 0 ;

/// Fetch the column  variable
  virtual void get_column( std::valarray<Real> & v, size_t variable  ) = 0;
///
  virtual void multiply_add_column( std::valarray<Real> & v, Real alpha , size_t variable  ) = 0;

///
/// Solve a linear system of equations. The  start  argument
/// indicates the position of the first non-zero argument.
/// TODO: why is this 1 by default?
///
  virtual void solve( std::valarray<Real>  & x, size_t start = 1 ) = 0;

///
/// Standard multiply add:
/// x = alpha * x + beta * M * b
///
  virtual void multiply( const std::valarray<Real>& b,
                         std::valarray<Real>& x,
                         double alpha = 0.0,
                         double beta = 1.0,
                         int  left_set = ALL, int right_set = ALL ) = 0;
///
/// Needed by LCP solver
///
  virtual Real get_diagonal_element( size_t i ) const = 0;

///
/// Multiply add operations with submatrices, given a set of active and
/// inactive variables.  The following four cases are covered.
///
/// x( active ) = alpha * x( active ) + beta * M( active, active ) * b( active )
///
/// x( active ) = alpha * x( active ) + beta * M( active, ~active ) * b( ~active )
///
/// x(~active) = alpha * x(~active) + beta * M(~active, active ) b( active )
///
/// x(~active) = alpha * x(~active) + beta * M(~active, ~active ) b( ~active )
///
/// We use two bool's to control whether we use `active'  or `!active',
/// one on each side.  `true'  corresponds to `active', `false' to
/// `!active'
///
/// multiply-add is delegated to the derived class, but the outer logic
/// is independent of that.
///


  virtual void multiply_submatrix(  const std::valarray<Real>& b,   std::valarray<Real>& x,
                                    double alpha = 0.0, double beta = 1.0,
                                    int  left_set = ALL, int right_set = ALL ){

    tmp_sub = b;

    if ( right_set != ALL){
      for ( size_t i = 0;  i < b.size(); ++i ) {

        if ( right_set == FREE  && ! active[ i ] ){
          tmp_sub[ i ] =  Real(0.0) ;
        }
        else if ( right_set == TIGHT &&  active[ i ] ){
          tmp_sub[  i ] =  Real(0.0) ;
        }
      }
    }
    multiply(tmp_sub, x, alpha, beta, left_set, right_set);
    // filter variables according to active variables or the reverse
    if ( left_set == FREE ){
      for ( size_t i = 0; i < x.size(); ++i ) { 
        if ( ! active[ i ] )
          x[ i ] = 0.0;
      }
    }
    else if ( left_set == TIGHT ){
      for ( size_t i = 0; i < x.size(); ++i ) { 
        if (  active[ i ] )
          x[ i ] = 0.0;
      }
    }
  }



///
/// Here we solve for  v in M(active, active ) v = -M(active, variable)
/// and also compute the  mtt  variable needed by the qp solver
/// Because of the masks we need to apply, a temp copy is necessary.
/// This essentially computes the `variable'  column of the inverse of the
/// principal submatrix M(active, active)
///
  virtual bool get_search_direction(size_t variable, std::valarray<Real> & v, Real & mtt) {

    v = 0;

    if ( tmp_search.size() != v.size( ) )
      tmp_search.resize( v.size( ) );

    /// This is likely to be very very sparse
    /// The assumption is that we are getting the *negative* of the
    /// column. 
    get_column( v , variable );

    /// This is however dense
    /// TODO: there is an assumption about block structure here which
    /// shouldn't be there. 
    solve(v,   ( variable -1 ) / 2 );

    /// \todo : most time is spent here!
    multiply_submatrix(v, tmp_search, 0, 1.0, ALL, FREE );

    mtt = get_diagonal_element( variable ) + tmp_search[ variable ];

    return true;
  }

///
/// Given a search direction as previously computed and a steplength
/// determined by the LCP solver, we move variables according to rules
/// defined below.
///
///  For a given step length `step', the z( driving ) variable moves by
///  z( driving ) += step
///  and then w( driving ) moves in proportion with the search direction.
///
  virtual bool do_step(std::valarray<Real> & z, std::valarray<Real> & w,
                       std::valarray<Real> & v, Real step, size_t driving){

///
/// move free variables in the search direction
///
    for( size_t i = 0; i < z.size( ); ++i ) {
      if ( active[ i ] )
        z[ i ] += step * v[ i ];
    }

///
/// incrementing the driving variable, which is not active yet.
///
    z[ driving ] +=  step;

///
/// We want:
/// w(T) = w(T) + step * M(T, F) * v(F) + step *  M( T, s) );
/// first:
/// w(T) = w(T) + step * M(T, F) * v(F);
///
/// \todo a lot of time is spent here
    multiply_submatrix(v, w, 1.0, step, TIGHT, FREE );


///
/// This is the second time we fetch this column
///  \todo  TEST THE RESULTS OF THIS: speedup isn't great
#if 1

    multiply_add_column( w, -step, driving );

#else

    get_column( tmp_sub, driving );

    for ( size_t i = 0; i < tmp_sub.size(); ++i ){

      if ( ! active[ i ] ){
        w[ i ] -= step * tmp_sub[ i ];
      }

    }

#endif

    return true;
  }

///
/// Here we solve for z in
/// M(F, F)z(F) = -q(F) - M(F, T)z(T)
/// where z(T) are the variables at their bounds.  This is to support the
/// LCP solver
///
  void solve_subproblem(const std::valarray<Real>& q,
                        const std::valarray<Real>& l, const std::valarray<Real>& u,
                        const std::valarray<int> & idx, std::valarray<Real>& z){


    z = -q;

    if ( tmp_search.size( ) != z.size() )
      tmp_search.resize( z.size( ) );
    for ( size_t i = 0; i < tmp_search.size(); ++i ){
      if ( idx[ i ] ==  ( int ) LOWER  ){
        tmp_search[ i ] = -l[ i ];
        if ( active[ i ] ) toggle_active( i );
      }
      else if ( idx[ i ] ==  ( int ) UPPER  ){
        tmp_search[ i ] = -u[ i ];
        if ( active[ i ] ) toggle_active( i );
      }
      else{
        tmp_search[ i ] = Real(0.0);
        if ( ! active[ i ] ) toggle_active( i );
      }
    }

///
///z(i) = -q(i) - M(i, j) * z(j)
///
    multiply_submatrix( tmp_search,  z, 1.0, 1.0,FREE, TIGHT );
///
///z(i) = -M(i,i) \ ( q( i) +  M(i, j)*z(j) );
///
    solve( z );
    for ( size_t i = 0; i < z.size(); ++i ){
      if ( idx[ i ] == ( int ) LOWER){
        z[ i ] = l[ i ];
      }
      else if ( idx[ i ] == ( int ) UPPER ){
        z[ i ] = u[ i ];
      }
    }
  }


///
/// Utilities for octave.  These are defined in `diag4utils.h'
///
#ifdef OCTAVE
///
/// Reset the factor of matrix from data available in octave.
///
  virtual void read_factor(const octave_value_list& args, int arg) = 0;
///
/// Write the data in the matrix to an octave struct.
///
  virtual void convert_matrix_oct(octave_scalar_map & st) = 0;

#endif
};

///
/// This is the bare minimum data needed to store the original entries in a
/// tridiagonal matrix matrix.  This is needed to keep the original data as
/// needed when updating or downdating the matrix factors.
///
struct diag_data {
  std::valarray<Real> diag;    /// main diagonal: 2 entries per block
  std::valarray<Real> sub;     /// first subdiagonal: 1 entry per block
  std::valarray<Real> ssub;    /// second subdiagonal: even entries,
  diag_data( size_t _n ): diag( 2 * _n ), sub( 2 * _n ), ssub( 2 * _n ) {}
  diag_data(  ): diag(  ), sub(  ), ssub(  ) {}

};



///
/// Here we have a matrix which corresponds to a chain of particles
/// attached with spring and dampers, and each of the particles is subject
/// to dry friction.  This is permuted here to be a banded matrix of
/// bandwith 4 using a packed format:
///
/// Main diagonal where each other element is a mass, and each other
/// element is the compliance of the contact.
///
/// Sub-diagonal is all ones, one element for each mass.  This entry is on
/// the same row as the compliance
///
/// Sub-subdiagonal:  every other element: these are the spring constants
///
/// fill  : on sub-diagonal as well.  These become non-zero during
/// factorization
///

struct diag4 : public qp_diag4 {

  size_t n_blocks;             ///  number of blocks
  std::valarray<Real> diag;    /// main diagonal: 2 entries per block
  std::valarray<Real> sub;     /// first subdiagonal: 1 entry per block
  std::valarray<Real> ssub;    /// second subdiagonal: even entries,
/// one per block except for the last block.
  std::valarray<Real> fill;    /// second subdiagonal: odd entries, one per
/// block except for the last block.

  std::slice even;              /// slices for valarray. Avoids continuous
/// reallocation
  std::slice odd;               /// Same.

  diag_data original;          /// Hold the original data for matrix-vector
/// multiply and to simplify update/downdate


///
/// default constructor.
/// @TODO Find a way to get rid of this
///
  diag4() : qp_diag4(0), n_blocks(0), diag(0), sub(0), ssub(0), fill(0), original()  {  }

///
/// Allocate all arrays as well as the matrix to hold the original data.
/// Filling the arrays is left to the user.
/// This delegates to the next constructor.
///
  inline diag4(size_t _n, bool _bisymmetric = false) : diag4(_n, 2 * _n, _bisymmetric) {}

///
/// Here we allocate the diag4 matrix with the right size but also make
/// sure that the diag4qp class allocates the basic data with the right
/// sizes, as defined by inherited classes which can add rows and columns
/// to this.
/// The point here is that qp_diag4 might have a different size from diag4
/// This will be invoked by inherited classes in which case _n will not be
/// equal to _na
///
  inline diag4(size_t _n, size_t _na , bool _bisymmetric = false ) :
    qp_diag4( _na, _bisymmetric ),
    n_blocks(_n), diag( 2 * _n ), sub( _n ), ssub( _n - 1 ),
    fill( _n - 1 ), even(0, _n, 2), odd(1, _n, 2), original( _n ) {
  }


///
/// Delegate allocation to previous constructor and copy basic data. All
/// variables active by default.
///
  diag4( const std::valarray<Real>& _diag, const std::valarray<Real> & _sub,
         const std::valarray<Real>& _ssub,
         const bool _bisymmetric = false,
         const std::valarray<bool> _active = std::valarray<bool>()) :
    diag4(_diag.size()/2, _diag, _sub, _ssub, _bisymmetric, _active) {}

///
///  This does the real work in terms of allocation and storing data.
///
  diag4(size_t _na, const std::valarray<Real>& _diag, const std::valarray<Real> & _sub,
        const std::valarray<Real>& _ssub,
        const bool _bisymmetric,
        const std::valarray<bool> _active = std::valarray<bool>()) :
    diag4( _diag.size() / 2, _na,  _bisymmetric )
  {
    diag =  _diag;
    sub  =  _sub ;
    ssub =  _ssub;
    active= _active;
    sync();                     //! \brief this will cache the data in diag to a  safe spot.
  }




  inline virtual size_t size() const { return diag.size();}

///
/// This cache the original data for the matrix.
///
  void sync( ){
    original.diag = diag;
    original.sub  = sub;
    original.ssub = ssub;
  }

#ifdef OCTAVE
///
/// Utility to work from octave.  Here we take data which comes from a
/// script to construct the matrix.
///
  diag4(ColumnVector _diag, ColumnVector _sub, ColumnVector _ssub, ColumnVector _fill, ColumnVector _active, bool _bisymmetric = false);
  virtual void read_factor(const octave_value_list& args, int arg) ;
  virtual void convert_matrix_oct(octave_scalar_map & st) ;
#endif

///
///  Negate the signs of the Lagrange multipliers for bisymmetric mode.
///  These are the odd variables here, which are the friction forces.
///
  inline virtual void flip( std::valarray<Real> & x ){
    if ( bisymmetric )
      x[ odd ] = (Real) flip_sign * static_cast<std::valarray<double> >( x[ odd ] );
  }

///
/// Internal: the block is active if the second variable in that block is
/// active.
///
  inline bool active_block ( size_t i ) const {
    return active[ 2 * i  + 1 ];
  }

///
/// invert the diagonal block
/// Given a 2x2 matrix [ a b ]
///                    [ b c ]
/// the inverse is
///               (1/det)[  c -b ]
///                      [ -b  a ]
//  and det = (a * c - b *b ), the determinant.
//  For this particular problem, only "a" is updated by the operations
/// which precede this one and that is found in the factored matrix.
/// Since other values in the block haven't been touched yet, the only
/// valid data is in the original matrix.
//
  inline bool invert_active_block( size_t i ) {
//!  determinant: need diagnostic for the case where the matrix isn't P
    Real delta = Real( 1.0 ) / ( diag[ 2 * i ] * original.diag[ 2 * i + 1 ] -  original.sub[ i ] * original.sub[ i ] ) ;
//! swap diagonal entries using the original data for everything except the "a" value
    diag[ 2 * i + 1 ] =  delta * diag[ 2 * i ];
    diag[ 2 * i     ] =  delta * original.diag[ 2 * i + 1 ];
//! flip the sign of the off-diagonal entry (matrix is symmetric)
    sub [  i        ] = -delta * original.sub[ i ];
    return true;
  }

///
/// Inactive block invert.
///
/// No need for determinant for this case since the inactive block is
/// just
/// [ a  0 ]
/// [ 0  1 ]
///
  inline bool invert_inactive_block( size_t i ) {
//! @todo  Need diagnostic for the case where the matrix isn't P
    diag[ 2 * i     ] = (Real) 1.0  / diag[ 2 * i ];
    diag[ 2 * i + 1 ] = Real(1.0);
    sub[ i ]          = Real(0);
    return true;
  }

///
///  Wrapper for previous methods.
///
  inline bool invert_block(size_t i){
    ( active_block( i ) ) ? invert_active_block( i ) : invert_inactive_block( i );
    return true;
  };

///
/// Perform the work on the next subdiagonal using updated diagonal
/// block.  This cannot be performed on the last element.
///
//  What this will do is take care of the off diagonal elements below the
/// block that was just inverted, update the next diagonal block, and
/// create the fill.
///
///  This corresponds to the standard matrix factorization operation of
///  multiplying the current column with the inverse of the current,
///  factored, diagonal block.  The column in this case has only one entry:
///  the row after the block.
///
///   This, in turn, according to standard matrix factorization algorithm,
///   modifies the next diagonal block.  See Golub and van Loan for
///   details.
///
  inline bool push_forward( size_t i ) {
    ssub[ i ] = original.ssub[ i ] * diag[ 2 * i ];
    diag[ 2 * i + 2 ] = original.diag[ 2 * i + 2 ] - ssub[ i ] * original.ssub[ i ];
    fill[ i ] = original.ssub[ i ] * sub[ i ];
    return true;
  }

///
/// Factor the entire matrix as LDLT factors.
///
/// The special trick here is that `D'  is a 2x2 block diagonal matrix.
/// This leaves `L'  to have 1s on the diagonal, a sub-subdiagonal.
/// The sub-diagonal is contained  in the blocks.
///
/// The reason to put the push_forward function in the condition
/// preceeding the for-loop is to avoid an 'if' statement in the loop
/// itself.  'block' gets executed n times, but push_forward is executed
/// only n-1 times.  Note that `variable' here is blockwise, i.e., an odd
/// number only.
///
/// We stop short by 1 since `push_forward'  at block n-1 does all the work
/// on block n.
///
  virtual void factor( int variable = -1){
// std::chrono::time_point<std::chrono::system_clock> f_start, f_end;
//f_start = std::chrono::system_clock::now();

    if ( dirty ){
      if ( variable >= 0 ){
        update_block( variable );
      }
      else {
        variable = 0;
        diag[ 0 ] = original.diag[ 0 ];
      }

      assert( n_blocks > 0 );

      for ( size_t i = variable; invert_block( i ) &&  ( i < n_blocks - 1 );  ++ i  ){
        push_forward( i );
      }

//f_end = std::chrono::system_clock::now();
//std::chrono::duration<double> elapsed_seconds = f_end-f_start;
//std::cerr << "Factor time for size " << n << " :  " << elapsed_seconds.count() << std::endl;
      dirty = false;
    }
  }

///
/// Diagonal solve in the LDLT process.
/// This just multiplies the 2x2 block stored in diag and sub with the
/// RHS, since we store the inverse of D
///
  inline bool solve_block_forward(   std::valarray<Real> &x, size_t i ) {
    Real tmp0 = x[ 2 * i - 2 ];
    x[ 2 * i - 2 ] = diag[ 2 * i - 2 ]  * tmp0  + x[ 2 * i - 1 ] * sub[ i - 1 ];
    x[ 2 * i - 1 ] = sub[ i - 1 ]       * tmp0  + x[ 2 * i - 1 ] * diag[ 2 * i -  1 ];
    return true;
  }

///
/// All-in-one, lower triangular and diagonal solve: LDx = b.
/// Work in-place, and do the digonal solve as we go.
///
///  The lower triangle contains only ones on the sub-subdiagonal along
///  with the fills.   Everything else is contained in the D blocks.
///
///  This means in particular that there are only 1's in the first two
///  rows.
///
///  We move down block-wise and in all case encounter
///
///   [ ssub   0  1  0 ]
///   [   0  fill 0  1 ]
///


  inline bool forward_elimination(   std::valarray<Real> & x, size_t start = 1  ){
    start = 1;
    for( size_t i = start ; i <  n_blocks; ++i ){
      x[ 2 * i ] = x[ 2 * i ] - ssub[ i - 1 ] * x[ 2 * i  - 2 ] - fill[ i - 1 ] * x[ 2 * i - 1];
      solve_block_forward( x, i );
    }
    solve_block_forward( x, n_blocks );
    return true;
  }



///
/// Solve LT y = b   in a very standard way.
///
/// Use an int counter here to mind the 0 crossing
  inline bool backward_substitution(  std::valarray<Real>& x ){

    for( int i = (int)n_blocks - 2; i >=0; --i ){
      x[ 2 * i ]    =  x[ 2 * i ]     - ssub[ i ] * x[ 2 * i + 2 ];
      x[ 2 * i + 1] =  active_block( i ) * ( x[ 2 * i + 1 ] - fill[ i ] * x[ 2 * i + 2 ] );
    }

    if ( ! active_block( n_blocks-1 ) )
      x[ 2 * n_blocks - 1 ] = 0;

    return true;
  }

/// Driver.  Allow to start the solve at a variable different from the
/// begining in case x is all zeros up to `start'  Note that we never
/// start at 0.
  inline virtual void solve(  std::valarray<Real>  & x, size_t start = 1){
    if ( dirty ) {
      factor();
    }
    forward_elimination( x, start );
    backward_substitution( x );
    flip( x );

  }


///
///  Here we update a block which has changed from active to inactive or
///  vice versa.  If that block was active, it had the form (before
///  inverse)
///  [ a + d  1 ]
///  [ 1      e ]
///  and `d' is a value added from what came before.   This has been
///  inverted at this point though and so we re-invert the matrix to
///  recover a + d, then set the new form
///  [ a + d  0 ]
///  [ 0      1 ]
///  of an inactive block.  Then invert that and proceed.  For the other
///  case, we go the other way around.  The argument `activate'  determines
///  whether we go from active contact or the other way around.
///
  inline void update_block( size_t variable ) {

/// deactivate
    if (  active_block( variable ) ) {
// Need diagnostic for the case where the matrix isn't P
      Real delta =  1.0 / ( diag[ 2 * variable ] * diag[ 2 * variable + 1 ] - sub[ variable ] * sub[ variable ] );
      diag[ 2 * variable ] = delta * diag[ 2 * variable + 1 ];
      sub[ variable ] = 0;
      diag[ 2 * variable + 1 ] = 1;
    } else {     /// activate
// TODO:  Need diagnostic for the case where the matrix isn't P
// otherwise we can get garbage without warning.
      diag[ 2 * variable ] =  1.0 / diag[ 2 * variable ];
      diag[ 2 * variable + 1 ]  = original.diag[ 2 * variable + 1 ];
      sub[ variable ]  = original.sub[ variable ];
    }
    toggle_active( variable );

/// now we are ready to process this block and move ahead to the next,
/// making us ready to restart the factorization from `variable + 1'
    return ;
  }

/// will mask the result
  inline virtual Real keep_it ( const int & left_set, const size_t& i ) const {
    return Real(   ( left_set == ALL ) || ( left_set == FREE &&  active[ i ] )  || ( left_set == TIGHT &&  ! active[ i ] ) ) ;
  }

///
/// Standard  multiply add but with variable masking.
///
  virtual void multiply(  const std::valarray<Real>& b,   std::valarray<Real>& x, double alpha = 0.0, double beta = 1.0,
                          int  left_set = ALL, int /*right_set*/ = ALL ){
    if ( n_blocks ==  0 )
      return;

    Real even  = keep_it(left_set,  0 );

/// unfortunately, 0 * x = garbage
/// if x is uninitialized so we either need two routines, one where
/// there isn't alpha and the present, or we need to wipeout the vector
/// first.
    ( alpha == 0.0 )?  x = 0 : x = alpha * x ;

/// We take care of bisymmetry as we go along.
    Real beta1 = flip_sign * beta;

    /// Two special cases:
    if ( even ) {
      if ( n_blocks == 1 ) {
        x[  0  ]  +=  beta * original.diag[ 0 ] * b[ 0 ]  + beta1 * original.sub[ 0 ] * b[ 1 ];
      }
      else if ( n_blocks == 2 ) {
        x[  0  ]  +=  beta * original.diag[ 0 ] * b[ 0 ]  + beta1 * original.sub[ 0 ] * b[ 1 ] + beta * original.ssub[ 0 ] * b[ 2 ];
        x[  2  ]  +=  beta * original.diag[ 2 ] * b[ 2 ]  + beta1 * original.sub[ 1 ] * b[ 3 ] + beta * original.ssub[ 0 ] * b[ 0 ];
      }
      else {
        size_t i = 0;             /// block counter
        size_t j = 2 * i;         /// counter in diagonal array: avoid
        /// repeated multiplications
        ///  first block: looks forward only
        x[  j  ] +=
            beta  * original.diag[ j ] * b[ j     ]
          + beta  * original.ssub[ i ] * b[ j + 2 ]
          + beta1 * original.sub [ i ] * b[ j + 1 ];

        ///  last block: looks back only
        i = n_blocks - 1;  j = 2 * i ;

        x[  j  ] +=
            beta  * original.diag[ j     ] * b[ j     ]
          + beta  * original.ssub[ i - 1 ] * b[ j - 2 ]
          + beta1 * original.sub [ i     ] * b[ j + 1 ] ;
        /// everything in the middle

        for (  i = 1, j = 2;   i < ssub.size(); ++i, j+=2 ) {
          x[ j ]  +=
              beta  * original.diag[ j    ] * b[ j ]
            + beta  * original.ssub[ i    ] * b[ j + 2 ]
            + beta  * original.ssub[ i -1 ] * b[ j - 2 ]
            + beta1 * original.sub [ i    ] * b[ j + 1 ];
        }
      }
    }

    /// Odd variables: skip all if-statements for the ALL case
    if ( left_set == ALL ){
      for( size_t i=0,j = 1; i < n_blocks; ++i, j+=2 ) {
        x[ j ] += beta * original.sub[ i ] * b[ j - 1 ] + beta1 * original.diag[ j ] * b[ j ];
      }
    } else {
      for( size_t i=0,j = 1; i < n_blocks; ++i, j+=2 ) {
        if (keep_it( left_set, j ) )
          x[ j ] += beta * original.sub[ i ] * b[ j - 1 ] + beta1 * original.diag[ j ] * b[ j ]  ;
        else
          x[ j ] = 0;
      }
    }
    return;
  }
  ///
  /// Specialized multiplication which only considers the even variables.
  /// Here alpha and beta are vectors so that we can do
  ///
  ///  x =   A .* x + B .*  ( H(even, even) *  x );
  ///
  ///  The reason for this is that we are considering cases where   H = M + U  
  ///  where M is diagonal, but we want to compute
  ///  ( W + a * U ) * b
  ///  
  ///  Where W is not the diagonal of H
  ///  and we compute instead  a * ( ( 1 / a ) * ( M + (W - M) )  + U  );
  ////
  ///  
  /// 
  
  virtual void multiply_velocities(  const std::valarray<Real>& b,
                                     std::valarray<Real>& x, const std::valarray<Real> & D,
                                     const std::valarray<Real> & G, Real alpha = 0.0, Real  beta = 1.0 )
  {


    
    if ( n_blocks ==  0 )
      return;
    
    ( alpha == 0.0 )?  x = 0 : x = alpha * x ;

    if ( n_blocks == 1 ) {
      x[  0  ]  +=  beta * (  G[ 0 ] * ( D[ 0 ] + original.diag[ 0 ] ) ) * b[ 0 ]  ;
    }
    else if ( n_blocks == 2 ) {
      x[  0  ]  +=  beta * (  G[ 0 ] * ( D[ 0 ] + original.diag[ 0 ] ) ) * b[ 0 ]  +  beta * original.ssub[ 0 ] * b[ 1 ];
      x[  1  ]  +=  beta * (  G[ 1 ] * ( D[ 1 ] + original.diag[ 2 ] ) ) * b[ 1 ]  +  beta * original.ssub[ 0 ] * b[ 0 ];
    }
    else {
      size_t i = 0;             /// block counter
      size_t j = 2 * i;         /// counter in diagonal array: avoid
      /// repeated multiplications
      ///  first block: looks forward only
      x[  i  ] += beta  * ( G[ i ] * ( D[ i ] + original.diag[ j ] ) ) * b[ i ] + beta  * original.ssub[ i    ] * b[ i + 1 ];
      
      ///  last block: looks back only
      i = n_blocks - 1;  j = 2 * i ;
      
      x[  i  ] += beta * ( G[ i ] * ( D[ i ] + original.diag[ j ] ) ) * b[ i ] + beta  * original.ssub[ i - 1 ] * b[ i - 1 ] ;
      /// everything in the middle

      for (  i = 1, j = 2;   i < ssub.size(); ++i, j+=2 ) {
        x[ i ]  += beta  * ( G[ i ] * ( D[ i ] + original.diag[ j ] ) ) * b[ i ] + beta  * original.ssub[ i    ] * b[ i + 1 ]
          + beta  * original.ssub[ i -1 ] * b[ i - 1 ];
      }
    }

    return;
  }

/// For the bisymmetric case, the alternate entries on the diagonal are
/// negated in comparison to the unsymmetrized matrix.  Needed for the
/// LCP solver.
  inline virtual Real get_diagonal_element( size_t i ) const {
    return  ( i%2 ) ?  flip_sign * original.diag[ i ] : original.diag[ i ];
  }

/// Since `variable' is one which can be switched, there are only two
/// elements in that column: diagonal and first super diagonal.
/// However, we are only interested in the parts corresponding to the
/// active variables and that's just the super diagonal.
/// We want  v = - M(F, F) \ M(F, variable ) where F is the set of
/// active variables.
/// \todo: this needs to allow for a sparse version so that optimization
/// from the calee can be made.
  virtual void get_column( std::valarray<Real> & v, size_t variable ) {
    v[ variable - 1] = - flip_sign * original.sub [ ( variable - 1 ) / 2];
    v[ variable    ] = - flip_sign * original.diag[ variable ];
  }

  inline virtual void multiply_add_column( std::valarray<Real> & w, Real alpha, size_t variable  ){

    if ( ! active[ variable -1  ] ){
      w[ variable - 1 ] -= alpha * flip_sign * original.sub [ ( variable - 1 ) / 2];
    }
    if ( ! active[ variable    ] ){
      w[ variable    ] -=  alpha *  flip_sign * original.diag[ variable ];
    }

  }

};


////
/// diag4length class extends the diag4 4-diagonal matrix by one full row
/// and one full column.  As such, it reuses all the methods from diag for,
/// but ads a small correction to them.   This does mean overloading all
/// methods, but the amount of extra work is minimal.
/// With this, one can have both curvature energy and a global lenght
/// constraint.
///
/// TODO: biggest problem here is the semantic of the active variables.
///

struct diag4length : public diag4 {
  std::valarray<Real> J0;       // original data: Jacobian + diagonal
  std::valarray<Real> J;        // the Jacobian for the length part and
// diagonal element.  This gets overwritten
// with inv(M) * J0 to compute the Schur
// complement.
  Real i_schur;                  // inverse of  the Schur complement

  inline size_t last() const {
    if (J0.size() == 0) {
      return std::numeric_limits<size_t>::max();
    }
    return J0.size() - 1;
  }
  diag4length() : diag4(),  J0( 0 ), J( J0 ), i_schur( 0 ) {}

/// start with all empty
  diag4length(size_t _n, bool _bisymmetric = false) :
    diag4( _n, 2 *_n +1 , _bisymmetric  ), J0( 2 * _n + 1 ), J( 2 * n_blocks + 1 ), i_schur( 0 ) {
  }

/// double delegation here, via the diag4 constructor
  diag4length(const std::valarray<Real>& _diag,
              const std::valarray<Real> & _sub,
              const std::valarray<Real>& _ssub,
              const std::valarray<Real>& _J,
              const bool _bisymmetric,
              const std::valarray<bool> _active = std::valarray<bool>()):
    diag4(_J.size(),  _diag,  _sub, _ssub, _bisymmetric, _active ), J0( _J ), J( _J ), i_schur( 0 ){
  }
  inline virtual size_t size() const  { return J0.size(); }

#ifdef OCTAVE
/// utility to workf from octave
  diag4length(ColumnVector _diag, ColumnVector _sub, ColumnVector _ssub, ColumnVector _fill,
              ColumnVector _J, ColumnVector _active, bool _bisymmetric = false);
  virtual void convert_matrix_oct(octave_scalar_map & st);
/// utility to work from octave: sets the factor directly
  virtual void read_factor(const octave_value_list& args, int arg) ;
#endif

  virtual void multiply(  const std::valarray<Real>& b,   std::valarray<Real>& x, double alpha = 0.0, double beta = 1.0,
                          int  /*left_set*/ = ALL, int /*right_set*/ = ALL ){
    Real xl = 0;

    if ( alpha != 0.0 ) {
      xl = alpha * x[ last() ];
    } else {
      xl = 0;

    }

/// this will yield M * b where M is the underlying diag4 matrix
    diag4::multiply( b,   x, alpha, beta);
/// now we add  b(end) * J0
    x +=  flip_sign *beta * b[ last() ] * J0;
/// And here we get  J0(1:end-1)'*b(1:end-1) + J0(end) * b(end)
    xl += beta * (
      std::inner_product ( std::begin( J0 ), std::end( J0 ) -(size_t)1 , std::begin( b ), Real( 0 ) )
      + flip_sign * J0[ last() ] * b[ last() ]
      );
    x[ last() ] = xl;

  }


/// Just as the name says.  This is straight forward.  We cache
/// inv(M)*J0(1:end-1) since that's used when solving.
  virtual void get_schur_complement(){
    J = J0;
    J[ !active ]= 0;
    J[ last() ] = 0;
    diag4::solve( J );        //  now we have J = inv(M) * J0'
/// there is no sign flip here: for the bisymmetric case, the last
/// element will be negative, and all signs are absorbed in the
/// 'solve' method..
    i_schur = Real( 1.0 ) / (  std::inner_product( std::begin(J0), std::end(J0), std::begin( J),  Real( 0.0 ) -J0[ last() ] ) );
  }

  virtual void factor( int variable = -1){
    diag4::factor( variable );
    get_schur_complement();     // this will automatically factor via
    dirty = false;
  }


  virtual void solve(  std::valarray<Real>  & x, size_t start = 1){

    if ( dirty )
      factor( );

    diag4::solve( x, start );    // x is now  inv(M) * x

    if (  active[ last() ] ){

      double a = x[ last ( ) ]; // save this to solve for separately
      x[ last() ] = 0;          // nix this for safety: we are still doing
// inner products all the way

      x[ !active ] = 0 ;

/// This line below gives: G * inv(M) * x0, with x0 original x
      double gamma =  std::inner_product ( std::begin( J0 ), std::end( J0 ), std::begin( x ), Real( 0.0 ) );

      double alpha =   flip_sign *  i_schur * ( gamma - a  );

/// update x with the last variable
      x -= flip_sign * alpha * J ;
      x[ last( ) ] = alpha;

    }
  }

/// For the bisymmetric case, the alternate entries on the diagonal are
/// negated in comparison to the unsymmetrized matrix.  Needed for the
/// LCP solver.
  inline virtual Real get_diagonal_element( size_t i ) const {
    if ( i < last() )
      return diag4::get_diagonal_element( i );
    else
      return  flip_sign * J0[ last() ];
  }

  virtual void get_column( std::valarray<Real> & v, size_t variable ) {

    if ( variable < last() ) {
      diag4::get_column( v, variable );
      /// WARNING: explicit assumption that the last row is never negated
      /// by bisimmetry
      v[ last() ] = J0[ variable ];
    }
    else {
      for ( size_t i = 0; i < J0.size(); i+=2 )
        v[ i ] = flip_sign * J0[ i ];
    }
  }
  ///
  ///  Multiply and add a column to a vector, making use of the fact that
  ///  the columns are sparse here.
  ///
  inline virtual void multiply_add_column( std::valarray<Real> & w, Real alpha, size_t variable  ){

    /// Delegate to banded matrix
    if ( variable < last() ) {
      diag4::multiply_add_column( w, alpha, variable );
      if ( ! active[ last() ])
        w[ last() ] += alpha * J0[ variable ];
    }
    /// Handle the last column.
    else {
      for ( size_t i = 0; i < J0.size(); i+=2 ) {
        if ( ! active[ i ] ) {
          w[ i ] += flip_sign * alpha * J0[ i ];
        }
      }
    }
  }


} ;




#endif
