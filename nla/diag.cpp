#include <iostream>
#include <vector>
#include <valarray>
#include <algorithm>
#include <string.h>

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

typedef valarray<Real>  array;

///
/// We store the matrix in a vector of arrays so that the first array
/// contains the diagonal, the second contains the first subdiagonal, etc. 
///
typedef vector< array * >   table;



typedef double Real;

struct diag4 {
  
  
  valarray<bool>  active;	  /* active variables */
  bool bisymmetric;       /* whether or not this is a bisymmetric matrix */
  bool dirty;             /* whether or not we need to refactor */
  array  flip;            /* which variables are negated */

  table   data;
  diag4 * original;       /* copy of original data */
  diag4( size_t N, size_t W, bool orig = false, bool bisym = false ) :
    active(true, N), bisymmetric( bisym ), dirty( true ), flip( Real(1.0), N )
  {
    
    data.reserve( W );
    
    for ( int i = 0; i < W; ++i ){
      data.push_back( new array( N - i ) );
    }

    if ( ! orig ){
      original = new diag4( N, W, true, bisym);
    }

  }

  inline double & diag_element ( size_t i ){
    return ( * data[ 0 ] ) [ i ];
  }
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
    return   ( *data[ i - j ] )[ i ];
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

  /// Copy the coloum below the diagonal in a buffer
  inline void get_current_lower_column( size_t j, Real * c){
    if ( j < size() ){
      size_t limit =  min( bandwidth(), size() - j );
    
      for ( size_t i = 1; i < limit; ++i ){
        double d = lower_element_raw( i + j, j );
        c[ i - 1 ]  =  d;
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

  void inline factor( char * foobar ){

    double  buffer[ bandwidth( ) - 1 ] ; 
    
    if ( dirty ){
      for ( size_t j = 0; j < size(); ++ j ){

        double d = Real(1.0);
        
        
        get_current_lower_column( j,  buffer);
        /// rank k update
        for ( size_t k = 1; k < bandwidth(); ++k ){
          rank_update_column( j, j + k,  buffer, d );
        }
        ( *data[ 0 ] )[ j ] = d;
        update_column  ( j );
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

  inline void forward_elimination( array & x ){

    
    for( size_t i = 0; i < size() - 1; ++i ){
      size_t limit  = col_bandwidth( i );

      for ( size_t j = 1; j < limit; ++j ){

        x[ i + j ] -= x[ i ] * ( *data[ j ] )[ i ];

      }

      x[ i ] *= ( *data[ 0 ] )[ i ];
      
    }

    return;
    
  }

  inline void back_substitution( array & x ){
    /// Start one before the end
    for ( size_t i = size() - 2; i != (size_t)-1; --i  ){
      size_t limit = col_bandwidth( i );
      for( size_t j = 1; j < limit; ++j ){
        x[ i ] -= ( *data[ j ] )[ i ] * x[ i + j ];
      }
    }
  }
  
  
  
  inline size_t size() const { return active.size(); }
  inline size_t last() const { return active.size() - 1 ; }
  inline size_t bandwidth() const { return data.size(); }

  inline size_t col_bandwidth( size_t j ) const {
    return min( data.size(), size() - j );
  }

  void multiply_add( const  array & x, array& y){
    y = 0;
    
    /// Everything above and including the diagonal, moving along bands
    /// (this trashes memory like mad)
    for ( size_t i= 0; i < bandwidth(); ++i ){
      for ( size_t j = i; j < size(); ++j ){
        y[ j - i ]  +=  ( flip[ j ] * active[ j ] ) * ( *original->data[ i ] ) [ j - i ]  * x[ j ];
      }
    }

    /// Everything below the diagonal
    for( size_t i = 1; i < data.size(); ++i){
      for ( int j = (int) size() - 1;   j >= i; --j ){
        y[ j ]  +=  active[ j ] * ( *original->data[ i ] )[ j - i ] * x[ j - i ];
      }
    }
    
  }
  array & operator[](size_t i) { return *original->data[ i ]; }

  inline void sync(){
    for ( size_t i = 0; i < bandwidth(); ++i ){
      memcpy(
        &( ( * data[ i ] )[ 0 ] ),
        &( ( * original->data[ i ] )[ 0 ] ), ( size() - i ) * sizeof( double )  );
    }
  }

  ///
  /// Apply left looking LDLT algorithm here without pivoting.  We keep 1/d
  /// on the diagonal.  
  ///
  /// We perform a rank-k update where k here is the bandwidth so that the
  /// matrix is updated on the right and below the current point. 
  ///
  ///
  /// 
  void factor( size_t start = 0){
    /// this simplifies operations and addressing for the rank-k update
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

};




int main(){
  size_t N = 5;
  size_t bw = 3;
  array x(Real(1.0), N);
  array y(Real(0.0), N);
  
  diag4 m( N, bw);

  for ( size_t i = 0; i < m.size(); ++i  ){
    m[ 0 ][ i ] = 40;
  }
  for ( size_t i = 0; i < m[1].size(); ++i  ){
    m[ 1 ][ i ] = -1;
  }
  for ( size_t i = 0; i < m[2].size(); ++i  ){
    m[ 2 ][ i ] = -2;
  }
  
  m.sync();
  
#if 0 
  m.multiply_add(x, y);
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
#if 1
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
  
#endif
    

  return 0;
}
