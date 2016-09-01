#ifndef BANDED_H
#define BANDED_H
#include <iostream>
#include <vector>
#include <valarray>
#include <algorithm>
#include <string.h>
#include <stdlib.h>
#include <diag4.h>
#include <diag4qp.h>


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


typedef double Real;


///
/// We store the matrix in a vector of arrays so that the first array
/// contains the diagonal, the second contains the first subdiagonal, etc. 
///
typedef std::vector< std::valarray<double> * >   table;

typedef double Real;

struct band_diag : public qp_diag4 {

  std::valarray<double>  negated;            /* which variables are negated */

  table   data;

  band_diag * original;       /* copy of original data */
  inline band_diag( size_t N, size_t W, bool bisym = false, bool orig = false ) : qp_diag4( N, bisym), 
                                                                           negated( Real(1.0), N ) 
  {
    
    data.reserve( W );
    
    for ( int i = 0; i < W; ++i ){
      data.push_back( new std::valarray<double>( N - i ) );
    }

    if ( ! orig ){
      original = new band_diag( N, W, bisym, true);
    } else {

      original = 0;
      
    }

  }
  inline ~band_diag(){
    for( size_t i = 0; i < data.size(); ++i ){

      delete data[ i ];

    }
    if ( original ){ 

      delete  original;

    }
  }
  
  ///
  /// Boiler plate stuff for accessors and mutators to save typing
  ///
  inline size_t size()      const { return active.size(); }
  inline size_t last()      const { return active.size() - 1 ; }
  inline size_t bandwidth() const { return data.size(); }
  std::valarray<double> & operator[](size_t i) { return *original->data[ i ]; }
  /// Number of elements *below* the diagonal. 
  inline size_t col_bandwidth( size_t j ) const {
    return std::min( data.size(), size() - j );
  }
  /// Number of elements to the *left* of the diagonal
  inline size_t row_bandwidth( size_t i ) const {
    return std::min( data.size(), i + 1 );
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
    /// Fix the diagonal. 
    ( * data[ 0 ] )[ k ] = double(1);
    
    return;
    
  }

  /// Copy the *factored* coloum below the diagonal in a buffer
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
  
  /// Copy the original coloum below the diagonal in a buffer
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

  ///
  /// Utility for the factorization which is right looking.
  ///
  inline void update_column( size_t j ){

    if ( j < size() ) {
      size_t limit = col_bandwidth( j );
      double d = diag_element( j ); 
    
      for ( size_t i = 1; i < limit; ++i ){

        ( * data[ i ] )[ j ] *= d;

      }

    }

    return;
     
  }

  ///
  ///  Utility: makes the code simpler to read. 
  ///  Note that the main diagonal contains 1/D in the LDLT factorization. 
  ///
  inline void update_diagonal( size_t j ){

    ( *data[ 0 ] )[ j ] = ( double ) 1.0  / ( *data[ 0 ] )[ j ];

    return;
       
  }
   

  /// 
  /// Here we update column k after having processed column j.  This is at
  /// most a rank K update where K is the bandwidth.  This will touch the
  /// elements in k which are less than k-j below the diagonal
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
  inline void forward_elimination( std::valarray<double> & x ){

    /// Walk down the rows, stop one before the end
    for( size_t i = 0; i < size() - 1; ++i ){
      size_t limit  = col_bandwidth( i );
      ///  Walk along column to the left of the diagonal
      for ( size_t j = 1; j < limit; ++j ){
        x[ i + j ] -= x[ i ] * ( *data[ j ] )[ i ];
      }
      /// Update with D inverse
      x[ i ] *= active[ i ]  * ( *data[ 0 ] )[ i ];
    }

    /// Wrap up last element: only D inverse
    x[ size() - 1 ] *= active[ size() - 1 ] * ( *data[ 0 ] )[ size() - 1 ];

    return;
    
  }

  inline void back_substitution( std::valarray<double> & x ){
    /// Start one before the end down to 0.
    for ( size_t i = size() - 2; i != (size_t)-1; --i  ){

      size_t limit = col_bandwidth( i );

      for( size_t j = 1; j < limit; ++j ){
        x[ i ] -= ( *data[ j ] )[ i ] * x[ i + j ];
      }

      x[ i ] *= active[ i ];
      
    }
  }


  ///
  /// Reset the  data to the original. 
  ///
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

///
/// TODO: apparently unused.
///
  inline virtual Real keep_it ( const int & left_set, const size_t& i ) const {
    return Real(   ( left_set == ALL ) || ( left_set == FREE &&  active[ i ] )  || ( left_set == TIGHT &&  ! active[ i ] ) ) ;
  }

  ////
  //// Implementation of pure virtuals in the base class.  Silly enough,
  //// all masking is done in the base class already and unused here.
  //// TODO: clean this up.
  ////
  inline virtual void multiply( const std::valarray<Real>& x, std::valarray<Real>& y, double alpha = 0.0, double beta = 1.0,
                                int  /* left_set */ = ALL, int /* right_set */  = ALL ) {
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
  ///  The start variable is systematically ignored at this time, meaning
  ///  that we factor the entire matrix. 
  ///  TODO: fix this to skip the part which does not need refactorization,
  ///  i.e., all rows before start.  The issue here is to recompute the
  ///  the parts of the rank updates from above start row which affect
  ///  stuff below it, taking the masks into account. 
  /// 
  inline virtual void factor( int start = -1){
    
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
  /// As declared in base class.  
  ///
  inline virtual void solve( std::valarray<double> & b, size_t /* variable */){
    factor();
    forward_elimination( b );
    back_substitution  ( b );
    
    for ( size_t i  = 0; i < b.size(); ++i ){
      b[ i ] *= negated[ i ];   /// accounts for bisymmetric case. 
    }

    return;

  }
  
  ///
  /// Needed by the base class.
  /// TODO: this is a duplication of previous code: diag_element
  inline virtual double  get_diagonal_element ( size_t i ) const {
    return negated[ i ] * ( * ( original->data[ 0 ]  ) ) [ i ];
  }
  
  ///
  /// Fetch the column  'variable', both below and above the diagonal
  ///
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
  inline virtual void multiply_add_column( std::valarray<Real> & v, Real alpha , size_t variable  ) {

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

#endif
