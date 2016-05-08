#include <iostream>
#include <vector>
#include <valarray>
#include <algorithm>
#include <string.h>
#include <stdlib.h>
#include <diag4.h>
#include <diag4qp.h>


static const double infinity  = std::numeric_limits<Real>::infinity() ;


///
///  This is to factorize symmetric or bisymmetric banded matrices with
///  bandwidth W, allowing for factorization update and downdate and needed
///  in an LCP solver.
///
///  We store the diagonal and subdiagonals.
///
///  With this storage scheme, the elements i of column j below any
///  diagonal are 
/// 
///  table[ i ]->[ j ],  i = 1, 2, ... , W -1,
///
///   as long as j + i < N
///
///   This means that element (I, J)  with I >= J is found at 
///   table[ I - J ]->[ J ],   with I - J < W,  
///
///  which are the same as the elements of row j to the right of diagonal.
///
///  If we want the elements in column j above the diagonal then we access 
///
///  table[ i ]->[ j - i ],  i = 1, 2, ... , W - 1,
///
///  
///

using namespace std;

typedef double Real;


///
/// We store the matrix in a vector of arrays so that the first array
/// contains the diagonal, the second contains the first subdiagonal, etc. 
///
typedef vector< valarray<double> * >   table;



typedef double Real;

struct band_diag : public qp_diag4 {

  valarray<double>  negated;            /* which variables are negated */

  table   data;
  band_diag * original;       /* copy of original data */
  band_diag( size_t N, size_t W, bool bisym = false, bool orig = false ) : qp_diag4( N, bisym), 
                                                                           negated( Real(1.0), N ) 
  {
    
    data.reserve( W );
    
    for ( int i = 0; i < W; ++i ){
      data.push_back( new valarray<double>( N - i ) );
    }

    if ( ! orig ){
      original = new band_diag( N, W, bisym, true);
    }

  }
  
  ///
  /// Boiler plate stuff for accessors and mutators to save typing
  ///
  inline size_t size()      const { return active.size(); }
  inline size_t last()      const { return active.size() - 1 ; }
  inline size_t bandwidth() const { return data.size(); }
  valarray<double> & operator[](size_t i) { return *original->data[ i ]; }
  /// Number of elements *below* the diagonal. 
  inline size_t col_bandwidth( size_t j ) const {
    return min( data.size(), size() - j );
  }
  /// Number of elements to the *left* of the diagonal
  inline size_t row_bandwidth( size_t i ) const {
    return min( data.size(), i + 1 );
  }
  /// saves a bit of typing
  inline double & diag_element ( size_t i ){
    return ( * data[ 0 ] ) [ i ];
  }
  /// saves a bit of typing
  inline double  diag_element ( size_t i ) const {
    return ( * data[ 0 ] ) [ i ];
  }

  ///
  ///  Element below the diagonal within the bandwidth.  No check performed. 
  /// 
  inline double & lower_element_raw( size_t i, size_t j) {
    return   ( *data[ i - j ] )[ j ];
  }
  inline double  lower_element_raw( size_t i, size_t j) const {
    return   ( *data[ i - j ] )[ j ];
  }
  inline double  lower_element_raw_original( size_t i, size_t j) const {
    return   ( * ( original->data[ i - j ] ) )[ j ];
  }

  /// fetch an element
  inline Real elem( size_t i, size_t j) const  {
    double r = 0;
    
    if (  i >= j ){
      if ( i - j  <  data.size()  ){
        r =  ( *( original->data[ i - j ] ) ) [ j ];
      }
    }
    else if  (  j - i  < data.size() ){
      r = elem( j, i );
    }

    return r;
    
  }

 
  ///
  /// Assuming the matrix has been reset, we zero out a row and column of
  /// the matrix, replaxing the diagonal element with 1.
  ///
  inline void delete_row_col( size_t k ) {
    /// start with nixing the column
    size_t limit = col_bandwidth( k );

    for ( size_t i = 1; i < limit; ++i ){
      lower_element_raw( i + k, k ) = 0;
    }
    /// take care of the row
    limit = row_bandwidth( k );
    for( size_t i = 1; i < limit; ++i ){
      lower_element_raw( k, k  - i  ) = 0;
    }
    /// nix the diagonal
    ( * data[ 0 ] )[ k ] = double(1);
    
    return;
    
  }

  /// Copy the coloum below the diagonal in a buffer
  inline void get_current_lower_column( size_t j, Real * c) const {

    if ( j < size() ){
      size_t limit = col_bandwidth( j );
    
      for ( size_t i = 1; i < limit; ++i ){

        c[ i - 1 ]  = lower_element_raw( i + j, j );
        
      }

    } else {

      memset( c, -1, ( bandwidth() -1 ) * sizeof( double ) );

    }
    
    return;
  }
  
  /// Copy the coloum below the diagonal in a buffer
  inline void get_current_lower_column_original ( size_t j, Real * c) const {

    if ( j < size() ){
      size_t limit = col_bandwidth( j );
    
      for ( size_t i = 1; i < limit; ++i ){

        c[ i - 1 ]  = lower_element_raw_original( i + j, j);
        
      }

    } else {

      memset( c, -1, ( bandwidth() -1 ) * sizeof( double ) );

    }
    
    return;
  }

  inline void update_column( size_t j ){

    if ( j < size() ) {
      size_t limit = col_bandwidth( j );
      double d = diag_element( j ); // diagonal element
    
      for ( size_t i = 1; i < limit; ++i ){

        ( * data[ i ] )[ j ] *= d;

      }

    }

    return;
     
  }

  inline void update_diagonal( size_t j ){

    ( *data[ 0 ] )[ j ] = ( double ) 1.0  / ( *data[ 0 ] )[ j ];
    return;
       
  }
   

  /// 
  /// Here we update column k after having processed column j.  This will
  /// touch the elements in k which are less than k-j below the diagonal
  ///
  inline void rank_update_column( size_t j, size_t k, double *cj, double d ){

    if ( k - j <=  bandwidth( ) ) {

      size_t start = k - j - 1;   // starting point in the cj vector
      size_t end   =   col_bandwidth( j );
    
      for ( size_t i = start; i < end; ++i ){

        ( * data[ i - start ]  )[ k ] -=  cj[ i ] * d * cj[ start ];
      
      }
    }

    return;

  }


  ///
  /// Combine lower triangular solve and multiplication with D inverse Here
  /// we walk along the columns.  In each column j we solve for b[ j ] and
  /// then update the b vector below it.  After that we multiply b[ j ]
  /// with the inverse diagonal element.
  ///
  /// Bounds are checked when we reach 
  /// 

  inline void forward_elimination( valarray<double> & x ){
    /// Stop one before the end
    for( size_t i = 0; i < size() - 1; ++i ){
      size_t limit  = col_bandwidth( i );

      for ( size_t j = 1; j < limit; ++j ){

        x[ i + j ] -= x[ i ] * ( *data[ j ] )[ i ];

      }

      x[ i ] *= active[ i ]  * ( *data[ 0 ] )[ i ];
      
    }

    x[ size() - 1 ] *= active[ size() - 1 ] * ( *data[ 0 ] )[ size() - 1 ];
    return;
    
  }

  inline void back_substitution( valarray<double> & x ){
    /// Start one before the end
    for ( size_t i = size() - 2; i != (size_t)-1; --i  ){
      size_t limit = col_bandwidth( i );

      for( size_t j = 1; j < limit; ++j ){
        x[ i ] -= ( *data[ j ] )[ i ] * x[ i + j ];
      }
      x[ i ] *= active[ i ];
      
    }
  }


  inline void sync(){
    for ( size_t i = 0; i < bandwidth(); ++i ){
      memcpy(
        &( ( * data[ i ] )[ 0 ] ),
        &( ( * original->data[ i ] )[ 0 ] ), ( size() - i ) * sizeof( double )  );
    }

    /// brutally deactivate rows and columns
    for ( size_t i = 0; i < size(); ++i ){
      if ( ! active[ i ] ){
        delete_row_col( i );
      }
    }

  }

/// will mask the result
  inline virtual Real keep_it ( const int & left_set, const size_t& i ) const {
    return Real(   ( left_set == ALL ) || ( left_set == FREE &&  active[ i ] )  || ( left_set == TIGHT &&  ! active[ i ] ) ) ;
  }
  ////
  //// Implementation of pure virtuals in the base class.
  //// Silly enough, the masking of the right set is done in the base class
  //// already 
  ////
  virtual void multiply( const std::valarray<Real>& x, std::valarray<Real>& y, double alpha = 0.0, double beta = 1.0,
                         int  left_set = ALL, int /* right_set */  = ALL ) {
    ( alpha == 0.0 )?  y = 0 : y = alpha * y ;

    /// Everything above and including the diagonal, moving along bands
    /// (this trashes memory like mad)
    for ( size_t i= 0; i < bandwidth(); ++i ){
      for ( size_t j = i; j < size(); ++j ){
        y[ j - i ]  +=  ( beta *  negated[ j ] ) * ( *original->data[ i ] ) [ j - i ]  * x[ j ];
      }
    }

    /// Everything below the diagonal
    for( size_t i = 1; i < data.size(); ++i){
      for ( int j = (int) size() - 1;   j >= i; --j ){
        y[ j ]  +=  ( beta * negated[ j - i  ] ) * ( *original->data[ i ] )[ j - i ] * x[ j - i ];
      }
    }
  }



  ///
  /// Apply left looking LDLT algorithm here without pivoting.  We keep 1/d
  /// on the diagonal.  
  ///
  /// We perform a rank-k update where k here is the bandwidth so that the
  /// matrix is updated on the right and below the current point. 
  ///
  ///  The start variable is systematically ignored.
  /// 
  virtual void factor( int start = -1){
    
    if ( dirty ) { 

      sync();                   // reset the matrix first, then remove
                                // inactive rows and columns
      
      /// This simplifies operations and addressing for the rank-k update
      /// by copying column data into a buffer. 
      double b[ bandwidth() - 1 ];
    
      /// move along columns
      for ( size_t j = 0; j < size(); ++j ){
        double d = double( 1.0 ) / diag_element( j );
        size_t limit = col_bandwidth( j );

        get_current_lower_column( j, b);

        for ( size_t k = 1; k < limit; ++k  ){
          /// update the diagonal
          diag_element( j + k )  -=  d * b[ k - 1 ] * b[ k - 1 ];
        }

        /// move along all columns to the right of j
        for ( size_t k = 1; k < limit; ++k  ){
          /// update the rows below the diagonal
          for ( size_t i = k + 1; i < limit ; ++i ) {
            lower_element_raw( j + i,  j + k ) -=  d * b[ k - 1 ] * b[ i - 1 ];
          }
        }
       
        /// store the inverse of the diagonal 
        diag_element( j ) = d;
        update_column( j );

      }
    }
    dirty  = false;
  }



  ///
  /// As declared in base class
  ///
  inline virtual void solve( valarray<double> & b, size_t /* variable */){
    factor();
    forward_elimination( b );
    back_substitution  ( b );
    
    for ( size_t i  = 0; i < b.size(); ++i ){
      b[ i ] *= negated[ i ];
    }

    return;

  }
  
  /// needed by the base class
  inline virtual double  get_diagonal_element ( size_t i ) const {
    return negated[ i ] * ( * ( original->data[ 0 ]  ) ) [ i ];
  }
  
  /// Fetch the column  'variable', both below and above the diagonal
  inline virtual void get_column( std::valarray<Real> & v, size_t variable  ){

    v = 0;
    v[ variable ]      =  - negated[ variable ] * ( * original->data[ 0 ] )[ variable ];
    
    size_t limit = col_bandwidth( variable );
    
    for ( size_t i = 1; i < limit; ++i ){
      v[ variable + i ] =  - negated[ i ] * lower_element_raw_original( i + variable, variable );
    }

    limit = row_bandwidth( variable );

    for( size_t i = 1; i < limit; ++i ){
      v[ variable - i ] =  - negated[ i ] * lower_element_raw_original( variable, variable  - i  );
    }
    return;
  }

  ///
  /// As needed to implement bisymmetry
  inline virtual void flip( std::valarray<Real> & x ) {
    for ( size_t i = 0; i < x.size(); ++i ){
      x[ i ]  *= negated[ i ];
    }
  }

  /// Defined as pure virtual in base class but most likely not needed
  virtual void multiply_add_column( std::valarray<Real> & v, Real alpha , size_t variable  ) {

    v[ variable ]  -= alpha * negated[ variable ] * ( * original->data[ 0 ] )[ variable ];


    double b [ bandwidth() ];
    
    get_current_lower_column_original( variable, b);
    size_t limit = col_bandwidth( variable );
    
    /// everything below the diagonal
    for ( size_t i = 1;  i < limit; ++ i ){ 
      v[ variable + i ] -= alpha * active[ variable + i ] * negated[ variable + i ] * b[ i - variable ];
    }
    
    limit = row_bandwidth( variable );
    for( size_t i = 1; i < limit; ++i ){
      v[ variable - i ] -= alpha * active[ variable - i ] * negated[ variable + i ]  *  lower_element_raw_original( variable, variable  - i  );
    }
    
  }


///
/// Utilities for octave.  These are defined in `diag4utils.h'
///
#ifdef OCTAVE
///
/// Reset the factor of matrix from data available in octave.
///
  virtual void read_factor(const octave_value_list& args, int arg);
  
  
///
/// Write the data in the matrix to an octave struct.
///
  virtual void convert_matrix_oct(octave_scalar_map & st);
  

#endif
};

struct clutch_sim {
  double mass[4];
  double k1;
  double k2;
  double force;
  double mu;
  double torque_in;
  double torque_out;
  double step;
  double lo[2];                 // Lower bounds for the motion range of the
// two first bodies.
  double up[2];                 // Upper bounds for motion range of same. 
  double tau;                   // damping rate
  double c1;
  double c2;
  double c3;
  
  band_diag M; // 9x9, bandwidth of 3, bisymmetric

  
  valarray<double> x;
  valarray<double> v;
  valarray<double> lower;
  valarray<double> upper;
  valarray<double> rhs;
  valarray<double> z;
  double gamma;
  valarray<double> g;           // constraint violations
  valarray<double> gdot;           // constraint violations
  
  diag4qp QP;                    /// An instance of the LCP solver.  

  clutch_sim( double _M[4], double K1, double K2, double F, double MU,
              double TIN, double TOUT, double H, double TAU,
              double LO[2], double UP[2], 
              double COMP1 = 1e-8, double COMP2 = 1e-8,
              double COMP3 = 1e-8  ) : 
    mass{ _M[0], _M[1], _M[2], _M[3]},
    k1( K1 ), k2( K2 ), force( F ), mu( MU ),
    torque_in( TIN), torque_out( TOUT),  step( H ), tau( TAU ),
    lo{ LO[ 0 ],LO[ 1 ] }, up{ UP[ 0 ], UP[ 1 ] },
    c1( COMP1 ), c2( COMP2 ), c3( COMP2 ),
    M( 9, 4, true ),
    x( 0.0, 4 ), 
    v( 0.0, 4 ), 
    lower( -std::numeric_limits<Real>::infinity(), 9 ),
    upper(  std::numeric_limits<Real>::infinity(), 9 ),
    rhs ( 0.0, 9 ), z( 0.0, 9), QP( &M )
  {

    gamma =  Real( 1.0 ) / ( 1.0 + 4 * tau / step  );
    
    double DD =  gamma *  4  /  ( step * step );
    
    double kk1 = k1 / DD;
    
    double kk2 = k2 / DD;
    
    
    M.negated[ 0 ] =  1.0;     // body 1
    M.negated[ 1 ] = -1.0;      // low range
    M.negated[ 2 ] = -1.0;      // up range
    M.negated[ 3 ] =  1.0;     // body 2
    M.negated[ 4 ] = -1.0;      // low range
    M.negated[ 5 ] = -1.0;      // up range
    M.negated[ 6 ] =  1.0;     // body 3
    M.negated[ 7 ] = -1.0;      // friction
    M.negated[ 8 ] =  1.0;     // body 4
    
/// Build the matrix
    M[ 0 ][ 0 ] =  mass[ 0 ] + kk1; // first mass including spring
    M[ 0 ][ 1 ] = -c1;             // lower bound for first spring
    M[ 0 ][ 2 ] = -c1;             // upper bound for first spring
    M[ 0 ][ 3 ] =  mass[ 1 ] + kk1 + kk2; // second mass
    M[ 0 ][ 4 ] = -c2;          // lower bound for second spring
    M[ 0 ][ 5 ] = -c2;          // upper bound for second spring
    M[ 0 ][ 6 ] =  mass[ 2 ] +  kk2; // third mass including spring
    M[ 0 ][ 7 ] = -c3;              // dry friction constraint
    M[ 0 ][ 8 ] =  mass[ 3 ];       // fourth mass coupled to third with
    /// safety
    M[ 1 ]  = 0.0;
    M[ 2 ]  = 0.0;
    M[ 3 ]  = 0.0;
     
#if 1 

    /// first two masses bound constraints
    M[ 1 ][ 0 ] =  Real( 1.0 );
    M[ 1 ][ 1 ] =  Real( 0.0 );
    M[ 1 ][ 2 ] =  Real( 1.0 );

    M[ 2 ][ 0 ] =  Real(-1.0 );
    M[ 2 ][ 1 ] =  Real(-1.0 );
    M[ 2 ][ 2 ] =  Real( 0.0 );
#endif
#if 1

    /// this connects the second and third mass via bound constraints
    M[ 1 ][ 3 ] =  Real( 1.0 );
    M[ 1 ][ 4 ] =  Real( 0.0 );
    M[ 1 ][ 5 ] =  Real( 1.0 );
    

    M[ 2 ][ 3 ] =  Real(-1.0 );
    M[ 2 ][ 4 ] =  Real(-1.0 );
    M[ 2 ][ 5 ] =  Real( 0.0 );
    M[ 2 ][ 6 ] =  Real( 0.0 );
    
    /// second and third
#endif

#if 1 
    /// this connects the third and fourth mass via dry friction
    M[ 1 ][ 6 ] =  Real( 1.0 );
    M[ 1 ][ 7 ] =  Real(-1.0 );

    
#endif 

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
/// Now the limits
    lower[ 1 ] = 0.0;              // inequality for first lower limit
    upper[ 1 ] = infinity;         // inequality for first lower limit
    
    lower[ 2 ] = 0.0;     // inequality for first upper limit
    upper[ 2 ] = infinity;             // inequality for first upper limit

    lower[ 4 ] = 0.0;              // inequality for second lower limit
    upper[ 4 ] = infinity;         // inequality for second lower limit

    lower[ 5 ] = 0.0;              // inequality for second upper limit
    upper[ 5 ] = infinity;         // inequality for second upper limit
    
    lower[ 7 ] = - step * mu * force;     // dry friction
    upper[ 7 ] =   step * mu * force;     // 
  }

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
    g   [ 0 ]  =  dx1  - lo[ 0 ];
    gdot[ 0 ]  =  dv1;
    
    g   [ 1 ]  = -dx1  + up[ 0 ];
    gdot[ 1 ]  = -dv1;

    
#if 1 
    g    [ 2 ] =  dx2  - lo[ 1 ];
    gdot [ 2 ] =  dv2;
    
    g    [ 3 ] = -dx2  + up[ 1 ];
    gdot [ 3 ] = -dv2;
    
#endif
#if 0 
    g    [ 4 ] =  v[ 3 ] - v[ 4 ];
    gdot [ 4 ] =  v[ 3 ] - v[ 4 ];

#endif
  }

  inline bool  impact(){
    bool i(false);

    double threshold_g(1e-3);
    double threshold_v(1e-3);

#if 1  
    i =  i || ( g[ 0 ] < threshold_g  &&  gdot[ 0 ] < threshold_v );
    i =  i || ( g[ 1 ] < threshold_g  &&  gdot[ 1 ] < threshold_v );
    i =  i || ( g[ 2 ] < threshold_g  &&  gdot[ 2 ] < threshold_v );
    i =  i || ( g[ 3 ] < threshold_g  &&  gdot[ 3 ] < threshold_v );

    i =  i ||  abs( g[ 4 ]   > threshold_g  &&  gdot[ 4 ] > threshold_v );
    i =  i ||  abs( g[ 4 ]   < - threshold_g  &&  gdot[ 4 ] < - threshold_v );
#endif

    return i;
    
  }
  
  inline void  get_rhs_impact(){
    rhs = 0;

    
    /// start with the masses
    rhs[ 0 ] = - ( mass[ 0 ] * v[ 0 ] );
    
    
    rhs[ 3 ] = - ( mass[ 1 ] * v[ 1 ] );
    
    
    rhs[ 6 ] = - ( mass[ 2 ] * v[ 2 ] );

    rhs[ 8 ] = - ( mass[ 3 ] * v[ 3 ] );
    
  }
  

  inline void  get_rhs(){
 
    
    
    /// displacements for the two springs
    double dx1 = x[0] - x[1];
    double dv1 = v[0] - v[1];
    double dx2 = x[1] - x[2];
    double dv2 = v[1] - v[2];   

    /// scaling of spring constants
    double KK1V =  step * step * k1 / Real(4.0); 
    double KK2V =  step * step * k2 / Real(4.0); 
    
    /// spring forces
    double fk1 = -  step * k1 * dx1  + KK1V * dv1;
    double fk2 = -  step * k2 * dx2  + KK2V * dv2;
    

    
    
    rhs = 0;
    
    /// start with the masses
    rhs[ 0 ] = - ( mass[ 0 ] * v[ 0 ] + fk1         +  step * torque_in  );
    
    rhs[ 3 ] = - ( mass[ 1 ] * v[ 1 ] - fk1  + fk2                       );
    
    rhs[ 6 ] = - ( mass[ 2 ] * v[ 2 ]        - fk2                      );

    #if 1  
    rhs[ 8 ] = - ( mass[ 3 ] * v[ 3 ]               + step * torque_out );
    #endif

    /// Now constraints;

    get_constraints();
    
    
    rhs[ 1 ] =  - ( gamma * ( ( -4.0 / step ) * g[ 0 ] + dv1 ) );
    rhs[ 2 ] =  - ( gamma * ( ( -4.0 / step ) * g[ 1 ] - dv1 ) );

                    
    rhs[ 4 ] =  - ( gamma * ( ( -4.0 / step ) * g[ 2 ] + dv2 ) );
    rhs[ 5 ] =  - ( gamma * ( ( -4.0 / step ) * g[ 3 ] - dv2 ) );
    
    /// and the friction constraint
    rhs[ 7 ] =  0;

    return;
    
  }

  void do_step( size_t n ){

    for ( size_t t = 0; t < n; ++t ){
      get_constraints();        // TODO: this is duplicated!
      if (   impact() ){

        get_rhs_impact();
        z = 0;
        
        QP.solve( rhs, lower, upper, z);
        
        v[ 0 ] = z[ 0 ];
        v[ 1 ] = z[ 3 ];
        v[ 2 ] = z[ 6 ];
        v[ 3 ] = z[ 8 ];     
      }
      
      get_rhs();
      
      z = 0;
      
      QP.solve( rhs, lower, upper, z);
      //QP.report_stats( std::cerr , z, lower, upper);
      

      v[ 0 ] = z[ 0 ];
      v[ 1 ] = z[ 3 ];
      v[ 2 ] = z[ 6 ];
      v[ 3 ] = z[ 8 ];
      
      x += step * v;
#if 0 
      cerr << endl;
      std::cerr << "Q" << endl;
      cerr << endl;
      for( auto i: rhs) std::cerr << i << std::endl;
      cerr << endl;
      
      std::cerr << "Z" << endl;
      for( auto i: z) std::cerr << i << std::endl;
      cerr << endl;

      std::cerr << "L" << endl;
      for( auto i: lower) std::cerr << i << std::endl;
      cerr << endl;

      std::cerr << "U" << endl;
      for( auto i: upper) std::cerr << i << std::endl;
      cerr << endl;
      
      std::cerr << "RHS" << endl;
      for( auto i: rhs) std::cerr << i << std::endl;
      cerr << endl;
#endif
    }
  }
};
 
  

int main(){


  
  




  
  
#if 0 
  size_t N = 5;
  size_t bw = 3;
  valarray<double> x(Real(1.0), N);
  valarray<double> y(Real(2.0), N);
  
  band_diag m( N, bw, false);
  m[ 0 ] = 40;
  m[ 1 ] = -1;
  m[ 2 ] = -2;
  
  m.sync();
  
  //m.multiply(x, y, 2.0, 3.0);
  m.active[ 1 ] = false;
  m.active[ 4 ] = false;
  m.multiply_submatrix(x, y, 0.0, 1.0, qp_diag4::FREE, qp_diag4::TIGHT);
  
  for( size_t i = 0; i < x.size(); ++i ){
    cerr << " y[ " << i << " ] =  "  << y[ i ] << endl;
  }
  Real b[ m.bandwidth() - 1 ];
  memset(b, 0, sizeof( b ) );
  
  for ( int i = 0; i < m.size(); ++i ){
    memset(b, 0, sizeof(b) );
    
    m.get_current_lower_column( i, b);
    for( size_t i = 0; i < bw-1; ++i ){
      cerr << " b[ " << i << " ] =  "  << b[ i ] << endl;
    }
  }

  size_t col0 = 0;
  size_t col1 = 1;
  

  double d = (*m.data[ 0 ])[ 0 ];
  m.get_current_lower_column( 0, b);
  m.rank_update_column( col0, 1, b, 1.0 / d );
  m.rank_update_column( col0, 2, b, 1.0 / d );
  m.rank_update_column( col0, 3, b, 1.0/d); 
  for ( int i = 1; i < 4; ++i ){
    memset(b, 0, sizeof(b) );
    
    m.get_current_lower_column( i, &b[0]);
    for( size_t i = 0; i < bw-1; ++i ){
      cerr << " b[ " << i << " ] =  "  << b[ i ] << endl;
    }
  }

  for ( size_t i = 0 ; i < m.size(); ++i ){
    cerr << " diag [ " << i << " ]  = "  << ( * m.data[ 0 ] ) [ i ] << endl;
    
  }
  
  if ( 0 ){
    for ( size_t i = 0; i < m.bandwidth(); ++i ){
      for ( size_t j = 0; j < ( * m.data[ i ] ).size(); ++j ){

        cerr << "Band: " << i <<  " element: " << j << "  " << ( *m.data[ i ] )[ j ] << endl;
      
      }

    }
  }

  x = 1;
  
  //m.forward_elimination( x );
  m.back_substitution( x );
#endif
#if 0
  m.factor();
      
  for ( size_t band = 0; band < m.bandwidth(); ++band){
    for ( size_t i = 0; i < ( *m.data[ band ]).size(); ++i ){
      cerr << "band[ " << band << " ][ " << i << " ] " << ( *m.data[ band ] ) [ i ] << endl;
      
    }
  }
  
#endif
#if 0 

  m.update_column( 0 );
  
  for ( size_t band = 1;  band < m.bandwidth(); ++band ) {
  
    for ( size_t i = 0; i < m[ band ].size(); ++i ){
      cerr << "band[ " << band << " ][ " << i << " ] " << ( *m.data[ band ] ) [ i ] << endl;
    }
    
  }
  

  valarray<double> v( 0.0, m.size() );
  
  m.get_column( v,  4 );
  for ( size_t i = 0; i < v.size(); ++i ){
    cerr << "v[ " << i << " ] = " << v[ i ] << endl;
  }
#endif

#if 1

  double masses[]{1,1,1,4000};
  double lo[] = {-10.5, -1.00};
  double up[] = { 10.5,  10.0};
  
  
  clutch_sim clutch(
    masses,
    0*1e-1,                       // first spring constant
    0*1e4,                        // second spring constant
    1e3,                        // force
    1.0,                        // friction coefficient
    100,                          // torque on input shaft
    0,                          // torque on output shaft
    1.0/100.0,                  // time step
    2.0,
    lo,     // lower range bounds
    up,        // upper range bounds
    1e-6,
    1e-6,
    1e-6
    );

  clutch.do_step( 90 );

#endif
  
  return 0;
}
