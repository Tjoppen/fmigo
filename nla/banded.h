#ifndef BANDED_H
#define BANDED_H



#include <iostream>
#include <vector>
#include <valarray>
#include <algorithm>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory>
#include "diag4.h"
#include "qp_solver.h"


#ifdef USE_IOH5
#include "ioh5.h"
#include <sstream>
#include <iomanip>
#endif
#ifdef USE_OCTAVE_IO
#include "octaveio.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#endif

#if defined(_WIN32) 
#define alloca _alloca
#endif 


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

///
/// We store the matrix in a vector of arrays so that the first array
/// contains the diagonal, the second contains the first subdiagonal, etc. 
///
typedef std::vector< std::valarray<double> * >   table;
typedef std::valarray< Real >    real_valarray;

struct banded_matrix : public qp_matrix {

  real_valarray  negated;            /* which variables are negated */

  table   data;

  banded_matrix * original;       /* copy of original data */
  banded_matrix( size_t N, size_t W, bool bisym = false, bool orig = false ) :
    qp_matrix( N, bisym),
    negated( Real(1.0), N ) 
  {
    
    data.reserve( W );
    
    for ( int i = 0; i < W; ++i ){
      data.push_back( new real_valarray ( Real( 0.0 ), N - i ) );
    }

    if ( ! orig ){
      original = new banded_matrix( N, W, bisym, true);
    } else {

      original = 0;
      
    }

  }
  inline virtual ~banded_matrix(){
    for( size_t i = 0; i < data.size(); ++i ){
      delete data[ i ];
      data[ i ] = 0;
    }
    if ( original ){ 
      delete  original;
      original = 0;
    }
  }
  void negate( size_t i ) { negated[ i ] = Real( -1.0 ); }


  /**
   *  This will return the diagonal and subdiagonal arrays.
   *  if i = 0, then you get the diagonal
   *  if i = 1, then you get the first subdiagonal
   *  etc.
   */

  real_valarray & get_subdiagonal(size_t i) { return  *data[i]; }
  
  ///
  /// Boiler plate stuff for accessors and mutators to save typing
  ///
  inline size_t size()      const { return active.size(); }
  inline size_t last()      const { return active.size() - 1 ; }
  inline size_t bandwidth() const { return data.size(); }
  real_valarray & operator[](size_t i) { return *original->data[ i ]; }
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
  inline void forward_elimination( real_valarray & x ){

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
    x[ size() - 1 ] *=  ( *data[ 0 ] )[ size() - 1 ];

    return;
    
  }

  inline void back_substitution( real_valarray & x ){
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
        &((*data[i])[0]),
        &((*original->data[i])[0]), ( size() - i ) * sizeof( double )  );
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
  ///   y = alpha * y + beta * M * x;
  ///
  inline virtual void multiply( const real_valarray &x, real_valarray &y, double alpha = 0.0, double beta = 1.0,
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
  inline virtual void factor( int /*start = -1*/){
    
    if ( dirty ) { 

      sync();                   // reset the matrix first, then remove
                                // inactive rows and columns
      
      /// This simplifies operations and addressing for the rank-k update
      /// by copying column data into a buffer. 
      double *b = (double *)alloca(sizeof(double)*(bandwidth() - 1) );
    
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
  inline virtual void solve( real_valarray & b, size_t variable ){
    factor( (int)variable);
    forward_elimination( b );
    back_substitution  ( b );
    
    for ( size_t i  = 0; i < negated.size(); ++i ){
      b[ i ] *= negated[ i ];   /// accounts for bisymmetric case. 
    }

    return;

  }
  
  ///
  /// Needed by the base class.
  /// TODO: this is a duplication of previous code: diag_element
  ///
  inline virtual double  get_diagonal_element ( size_t i ) const {
    return negated[ i ] * ( * ( original->data[ 0 ]  ) ) [ i ];
  }
  
  ///
  /// Fetch the column  'variable', both below and above the diagonal
  ///
  inline virtual void get_column( real_valarray & v, size_t variable , Real sign  ){

    v = 0;
    if ( active[ variable ] )
      v[ variable ] =  sign *  negated[ variable ] * ( * original->data[ 0 ] )[ variable ];
    
    size_t limit = col_bandwidth( variable );
    
    /// elements below the diagonal are stored with their correct signs
    for ( size_t i = 1; i < limit; ++i ){
      if ( active[ variable + i ] )
        v[ variable + i ] =  sign *   lower_element_raw_original( i + variable, variable );
    }

    limit = row_bandwidth( variable );

    /// to go above the diagonal, we go along the row towards the left and
    /// negate accordingly
    for( size_t i = 1; i < limit; ++i ){
      if ( active[ variable - i ] )
        v[ variable - i ] =  sign * negated[ variable ] * lower_element_raw_original( variable, variable  - i  );
    }
    return;
  }

  ///
  /// As needed to implement bisymmetry
  virtual void flip( real_valarray & x ) {
    for ( size_t i = 0; i < negated.size(); ++i ){
      x[ i ]  *= negated[ i ];
    }
  }

  /// Defined as pure virtual in base class but most likely not needed
  inline virtual void multiply_add_column( real_valarray & v, Real alpha , size_t variable  ) {

    /// The diagonal element
    v[ variable ]  += alpha * negated[ variable ] * ( * original->data[ 0 ] )[ variable ];

    size_t limit = col_bandwidth( variable );
    
    /// Everything in the column below the diagonal, going downwards: signs
    /// are already correct here
    for ( size_t i = 1;  i < limit; ++ i ){ 
      v[ variable + i ] += alpha * ( ! active[ variable + i ] ) * lower_element_raw_original( variable + i , variable );
    }
    
    /// To get the elements above the diagonal, we go leftward along the
    /// row.  These element have to be negated for the negated variables.
    limit = row_bandwidth( variable );
    for( size_t i = 1; i < limit; ++i ){
      v[ variable - i ] += alpha * ( ! active[ variable - i ] ) * negated[ variable  ]  *  lower_element_raw_original( variable, variable  - i  );
      ///      ^^^^^^  LEFTWARD
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

#ifdef USE_IOH5
  virtual void write_hdf5( H5::Group &g, std::string name = "banded_matrix"  ) {

    h5::write_scalar( g, std::string("bandwidth"), 2 * original->data.size() - 1 );
    h5::write_scalar( g, std::string("size"),      original->data[ 0 ]->size() );
    h5::write_vector (g, std::string("signs"),     negated );
    
    for ( size_t i = 0; i < original->data.size(); ++i  ){
      std::stringstream band_name;
      band_name << std::string("band") << std::setw(4) << std::setfill('0') << i;
      h5::write_vector ( g, band_name.str(),   *original->data[ i ] );
    }

    
  }
  
  virtual void write_hdf5( std::string filename, std::string prefix = "matrix"  ) {
    H5::H5File * file = h5::append_or_create( filename );
    H5::Group   g     = h5::append_problem( file, prefix );
    write_hdf5( g,  "banded_matrix");
    g.close();
    file->close();
  }

  
  virtual void read_hdf5 ( std::string filename, std::string prefix, int id  ) {};

  virtual void read_hdf5 ( H5::Group &g ) {};

#endif
#ifdef USE_OCTAVE_IO
  virtual std::ostream &  write_octave( std::ostream & os, const std::string & name){
    
    /// compute the *real* bandwidth, i.e., bands that aren't empty.
    size_t bw = 0;
    for ( size_t i  = 0; i < original->data.size(); ++i ){
      if ( original->data[ i ]->size() )
        ++bw;
    }

    octaveio::write_struct( os, name, 3 );
    octaveio::write_vector( os, "signs", negated );
    octaveio::write_vector( os, "active", active );
    
    
    size_t N = original->data[ 0 ]->size();

    os << "# name: bands" << std::endl;
    os << "# type: matrix" << std::endl;
    os << "# rows: " <<  2 * bw  - 1 << std::endl;
    os << "# columns: " <<  N << std::endl; 
    

    for ( size_t i = bw - 1; i != 0; --i ){
      octaveio::write_vector_patched( os,  *( original->data[ i ] ), N, false );
    }
    for ( size_t i = 0; i < bw; ++i ){
      octaveio::write_vector_patched( os,  *( original->data[ i ] ), N, true  );
    }
    return os;
  }

  virtual void  write_octave( const std::string & filename, const std::string & name  ){
    static int i = 0;
    std::stringstream real_name;
    real_name << filename << "_" << std::setw(4) << std::setfill('0') << i++;
    
    std::fstream fs;
    fs.open (real_name.str(), std::fstream::in|std::fstream::out|std::fstream::app);
    write_octave( fs, name );
    fs.close();
    return;
    
  }


  template <class T> 
  std::ostream & write_cpp_array( std::ostream& os, const std::string & name, T & v ){
  
    for ( size_t i = 0; i < v.size(); ++i ){
      os << name << "[ " << i << " ] = " << v[ i ] << ";" << std::endl;
    }
    return os;
  }


  virtual std::ostream & write_cpp( std::ostream& os, std::string name ){
//  os << "namespace "  << name  << " { " << std::endl;

    os << "banded_matrix & matrix ()  {" << std::endl;
    os << " static banded_matrix matrix( " << size() << ", " <<  data.size() << ", " <<  bisymmetric << ");" << std::endl;
    write_cpp_array(os, "matrix.negated", negated );
    write_cpp_array(os, "matrix.active", active );
    for ( size_t i = 0; i < original->data.size(); ++i ){
      std::valarray< double > & c =  *original->data[ i ];
    
      os << "{" << std::endl;
      os << "std::valarray< double > & b = ( *matrix.original->data[ " << i << " ] ); " << std::endl ;
      write_cpp_array(os, "b", c);
      os << "}" << std::endl;
    }
    return os << " return " << name << ";" << std::endl << "}" << std::endl;
  }

  

#endif
 
  
};




////
/// This extends any qp_matrix to have an extra row and column representing
/// a global length constraint. 
///
///
/// As such, it reuses all the methods from diag for,
/// but ads a small correction to them.   This does mean overloading all
/// methods, but the amount of extra work is minimal.
/// With this, one can have both curvature energy and a global lenght
/// constraint.
///
/// TODO: biggest problem here is the semantic of the active variables.
///

struct banded_length : public qp_matrix {
  std::shared_ptr<qp_matrix>   M;               // base matrix: we are adding a row and
  // column to this
  
  std::valarray<Real> J0;       // original data: Jacobian + diagonal.
  std::valarray<Real> J;        // This gets overwritten with inv(M) * J0
                                // to compute the Schur complement.
  Real i_schur;                  // inverse of  the Schur complement
  bool active_last;             /// Whether or not the last variable is
                                /// active 
  Real sign_last;

  inline size_t last() const {
    if (J0.size() == 0) {
      return std::numeric_limits<size_t>::max();
    }
    return J0.size() - 1;
  }

  virtual void flip( std::valarray<Real> & x) {
    M->flip( x );
    x[ last() ] *= sign_last;
  }
/// start with all empty
  banded_length() :  qp_matrix(), J0( 0 ), J( 0 ), i_schur( 0 ), M( 0 ), active_last( true), sign_last(1.0)  {}

  banded_length(qp_matrix * _M, bool _bisymmetric = false) :
    qp_matrix( _bisymmetric), J0( _M->size() + 1 ), J( J0  ), i_schur( 0 ), M( _M ), active_last( true ), sign_last(1.0) {
  }
 
  banded_length( std::shared_ptr<qp_matrix>  _M, bool _bisymmetric = false) :
    qp_matrix( _bisymmetric), J0( _M->size() + 1 ), J( J0  ), i_schur( 0 ), M( _M ), active_last( true ), sign_last(1.0) {
  }

  inline virtual size_t size() const  { return J0.size(); }


  virtual void negate( size_t i ) {
    if( i < M->size() ) {
      M->negate( i );
    } else {
      sign_last = - Real( 1.0 ) ;
    }
  }
  


  virtual void multiply(  const std::valarray<Real>& b,   std::valarray<Real>& x, double alpha = 0.0, double beta = 1.0,
                          int  /*left_set*/ = ALL, int /*right_set*/ = ALL ){
    Real xl = 0;

    if ( alpha != 0.0 ) {
      xl = alpha * x[ last() ];
    } else {
      xl = 0;
    }

/// this will yield M * b where M is the underlying diag4 matrix
    M->multiply( b,   x, alpha, beta);
/// now we add  b(end) * J0
    x +=  sign_last *beta * b[ last() ] * J0;
/// And here we get  J0(1:end-1)'*b(1:end-1) + J0(end) * b(end)
    xl += beta * 
      std::inner_product ( std::begin( J0 ), std::end( J0 ) -(size_t)1 , std::begin( b ), 
                           sign_last * J0[ last() ] * b[ last() ] ) ;
    x[ last() ] = xl;

  }


/// Just as the name says.  This is straight forward.  We cache
/// inv(M)*J0(1:end-1) since that's used when solving.
  virtual void get_schur_complement(){
    if ( active_last ){ 
      J  = J0;                     
      for( size_t i = 0; i < M->active.size(); ++i ) {
        if ( ! M->active[ i ] )
          J[ i ]= 0;
      }
      J[ last() ] = 0;
      /// make sure that the sign flips in the base matrix match 
      /// the signs in the J matrix.
      M->solve( J );        //  now we have J = inv(M) * J0
/// then the end, we want J0[last()] - J0' * inv(M) * J0
/// there is no sign flip here: for the bisymmetric case, the last
/// element will be negative, and all signs are absorbed in the
/// 'solve' method..
      i_schur =  sign_last * std::inner_product( std::begin(J0), std::end( J0 ) -(size_t)1, std::begin( J),  Real(0.0) ) ;
      i_schur =  sign_last * J0[ last ( )] - i_schur;
      i_schur = 1.0 / i_schur;
    }
    else {
      i_schur  = Real(1.0);
    }
  }
  

  virtual void factor( int variable = -1){
    M->factor( variable );
    get_schur_complement();     // this will automatically factor via
    dirty = false;
  }


  virtual void solve(  std::valarray<Real>  & x, size_t start = 1){

    double a = x[ last ( ) ]; // save this to solve for separately
    x[ last() ] = 0;          // nix this for safety

    if ( dirty )
      factor( );

    M->solve( x, start );    // x is now  inv(M) * x

    if (  active_last ){
      
      for ( size_t i = 0; i < M->active.size(); ++i ) {
        if ( ! M->active[ i ] )
          x[ i ] = 0.0;
      }
/// This line below gives: G * inv(M) * x0, with x0 original x
      double gamma =  std::inner_product ( std::begin( J0 ), std::end( J0 )-(size_t)1, std::begin( x ), Real( 0.0 ) );
      double alpha =  i_schur * (  a  -  gamma  );
/// update x with the last variable
      x -= sign_last * alpha * J ;
      x[ last( ) ] =  alpha;

    } else {
      x[ last( ) ] = a;
    }
  }

///
/// Self-explanatory: used by LCP solver, and is *variable* based, not
/// *block* based.  Subclasses have `blockwise'  version of these where
/// the index is the index of a Lagrange multiplier, not just a variable.
///
  virtual bool toggle_active( size_t i ){
    dirty = true;
    if ( i < M->size() )
      return M->toggle_active( i );
    else 
      active_last = ! active_last;
    return active_last;
  }

  virtual void set_active( ){
    dirty = true;
    M->set_active();
    active_last = true;
  }
 
  virtual void set_active( size_t i, bool a ){
    dirty = true;
    if ( i < M->size( ) )
      M->set_active( i, a );
    else
      active_last = a;
  } 
///
/// self-explanatory: for LCP solver: variable based, not  block  based.
///
  virtual void set_active( const std::valarray<int>& idx ){
    dirty = true;
    for ( size_t i =0 ; i < M->size(); ++i )
      M->set_active( i, idx[ i ] <= 0 );
    active_last = idx[ idx.size() - 1 ] <= 0;
    return ;
  }

  /// What follows are operations needed for the LCP solver

/// For the bisymmetric case, the alternate entries on the diagonal are
/// negated in comparison to the unsymmetrized matrix.  
  inline virtual Real get_diagonal_element( size_t i ) const {
    if ( i < last() )
      return M->get_diagonal_element( i );
    else
      return  sign_last * J0[ last() ];
  }

  virtual void get_column( std::valarray<Real> & v, size_t variable, Real sign ) {

    v = 0;
    if ( variable < last() ) {
      M->get_column( v, variable, sign );
      if ( active_last )
        v[ last() ] =  sign * J0[ variable ];
    }
    else {
      for ( size_t i = 0; i < J0.size()-1; ++i ) { 
        if ( M->active[ i ] )
          v[ i ] = sign * sign_last * J0[ i ];
      }
      if ( active_last )
        v[ last() ] = sign* sign_last * J0[ last( ) ];
    }
  }
  ///
  ///  Multiply and add a column to a vector, making use of the fact that
  ///  the columns are sparse here.
  ///
  inline virtual void multiply_add_column( std::valarray<Real> & w, Real alpha, size_t variable  ){

    /// Delegate to banded matrix
    if ( variable < last() ) {
      M->multiply_add_column( w, alpha, variable );
      if ( ! active_last )
        w[ last() ] += alpha * J0[ variable ];
    }
    /// Handle the last column.
    else {
      for ( size_t i = 0; i < M->size(); ++i ) {
        if ( ! M->active[ i ] ) {
          w[ i ] += sign_last * alpha * J0[ i ];
        }
      }
      if ( ! active_last )
        w[ last() ] += sign_last * alpha * J0[ last( ) ];
    }
  }


  virtual void multiply_submatrix(  const std::valarray<Real>& b,   std::valarray<Real>& x,
                                    double alpha = 0.0, double beta = 1.0,
                                    int  left_set = ALL, int right_set = ALL ){

    tmp_sub = b;

    if ( right_set != ALL){
      for ( size_t i = 0;  i < M->size(); ++i ) {

        if ( right_set == FREE  && ! M->is_active( i ) ){
          tmp_sub[ i ] =  Real(0.0) ;
        }
        else if ( right_set == TIGHT &&  M->is_active( i ) ){
          tmp_sub[  i ] =  Real(0.0) ;
        }
      }

      if ( right_set == FREE && ! active_last ) {
        tmp_sub[ last() ] =  Real(0.0) ;
      } else if ( right_set == TIGHT &&  active_last ){
        tmp_sub[  last() ] =  Real(0.0) ;
      }
    }

    multiply(tmp_sub, x, alpha, beta, left_set, right_set);

    // filter variables according to active variables or the reverse
    if ( left_set == FREE ){
      for ( size_t i = 0; i < M->active.size(); ++i ) { 
        if ( ! M->active[ i ] )
          x[ i ] = 0.0;
      }
      if ( ! active_last)
        x[ last( ) ] = 0.0;
    }
    else if ( left_set == TIGHT ){
      for ( size_t i = 0; i < M->active.size(); ++i ) { 
        if (  M->active[ i ] )
          x[ i ] = 0.0;
      }
      if ( active_last)
        x[ last( ) ] = 0.0;
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

    get_column( v , variable, -1.0 );

    solve( v );

    /// \todo : most time is spent here!  We actually only want one element
    /// and wasting all this time getting everyone and masking etc.
    /// Need a specialized method to get the multiplication of
    ///  mss = M(s, s) + M(s, F) * v( F)
    multiply_submatrix(v, tmp_search, Real(0.0), Real(1.0), ALL, FREE );

    mtt = get_diagonal_element( variable ) + tmp_search[ variable ];

    return true;
  }

  virtual bool is_active( size_t i ) const {
    return ( i < last() ) ? M->is_active( i ) : active_last ;
  }

#ifdef OCTAVE
/// utility to workf from octave
//  banded_length();
  virtual void convert_matrix_oct(octave_scalar_map & st){};
/// utility to work from octave: sets the factor directly
  virtual void read_factor(const octave_value_list& args, int arg) {};
#endif


#ifdef USE_IOH5
  virtual void write_hdf5( std::string & filename, std::string prefix = "matrix"  ) { };
  virtual void write_hdf5( H5::Group &g, std::string & name = "matrix" ) {};
  virtual void read_hdf5 ( std::string & filename, std::string & prefix, int id  ) {};
  virtual void read_hdf5 ( H5::Group &g ) {};
#endif

#ifdef USE_OCTAVE_IO

  virtual std::ostream &  write_octave( std::ostream & os, const std::string & name) { 
    octaveio::write_struct( os, name, 3 );
    M->write_octave( os, "submatrix" );
    octaveio::write_vector( os, "last_row", J0 );
    octaveio::write_scalar( os, "sign", flip_sign);
    return os;
  }

  virtual std::ostream & write_cpp( std::ostream& os, std::string name ) {return os; };
#endif





  
} ;




#endif
