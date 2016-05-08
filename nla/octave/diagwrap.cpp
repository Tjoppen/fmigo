#define OCTAVE
#ifdef OCTAVE
#include <octave/oct.h>
#include <octave/ov-struct.h>
#endif

#include <diag.cpp>
#include "diag4utils.h"

/**
 *  Read diagonals from a matrix whose columns are the bands
 */
qp_diag4 * read_band_matrix(const octave_value_list& args, int arg) {

  octave_scalar_map diag_struct = args(arg).scalar_map_value ();
  Matrix m   = static_cast<Matrix>(diag_struct.contents(string("matrix")).matrix_value());

  ColumnVector  signs   = static_cast<ColumnVector>(diag_struct.contents(string("signs")).column_vector_value());

  ColumnVector  active   = static_cast<ColumnVector>(diag_struct.contents(string("active")).column_vector_value());

  int band = m.cols();
  int N = m.rows();
  band_diag * M = new band_diag( N, band, true );

  for ( int i = 0; i < band; ++i ) {
    for ( int j = 0; j < N-i; ++j ){
      ( *M )[ i ][ j ] = m(j, i);
    }
  }

  for ( int i = 0; i < N; ++i ){
    M->negated[ i ] = signs( i );
    M->active[ i ] = ( bool ) active( i );
  }

  return M ;
}


  
void band_diag::read_factor(const octave_value_list& args, int arg) {

  return;
  
}

void band_diag::convert_matrix_oct(octave_scalar_map & st){

  int N = size();
  int BW =bandwidth();
  
  /// reformat the bands into an octave matrix
  Matrix M( N, BW, ( double ) 0 );
  ColumnVector A( N );          // active/ inactive
  ColumnVector S( N );          // signs
  
  for ( size_t j = 0; j < BW; ++j ){
    for ( size_t i = 0; i < (N - j) ; ++i ){
      M( i, j ) = ( *this )[ j ] [ i ];
    }
  }
  for ( size_t i = 0; i < N; ++i ){
    A( i ) = ( double ) active [ i ];
    S( i ) = ( double ) negated[ i ];
  }
  
  st.assign ("matrix", M);
  st.assign ("active", A);
  st.assign ("signs", S);
  

  return;
}

DEFUN_DLD( band_solve, args, nargout, "solve the linear system"){
  octave_value_list retval;

  qp_diag4 * M = read_band_matrix( args, 0 );
  
  ColumnVector  X   = static_cast<ColumnVector>(args(1).column_vector_value());
  std::valarray<double> x( X.length() );
  std::valarray<double> y( X.length() );
  
  oct_to_val(X, x);
  M->solve( x, 0 );
  
  
  val_to_oct( x, X );

  retval( 0 ) = X;
  
  if ( M ) { delete M ; M = 0; }
  return retval;
  
}


DEFUN_DLD( band_lcp, args, nargout, "lcp solver"){

  octave_value_list retval;

  qp_diag4 * M = read_band_matrix( args, 0 );
  
  ColumnVector  Q   = static_cast<ColumnVector>(args(1).column_vector_value());
  ColumnVector  L   = static_cast<ColumnVector>(args(2).column_vector_value());
  ColumnVector  U   = static_cast<ColumnVector>(args(3).column_vector_value());

  std::valarray<double> q( Q.length() ); oct_to_val(Q, q);
  std::valarray<double> l( L.length() ); oct_to_val(L, l);
  std::valarray<double> u( U.length() ); oct_to_val(U, u);
  std::valarray<double> z( U.length() ); 
  
  
  diag4qp QP( M );
  
  QP.solve( q, l, u, z);
  
  
  val_to_oct( z, Q );
  val_to_oct( QP.w, L );

  retval( 0 ) = Q;
  retval( 1 ) = L;
  retval( 2 ) = QP.get_complementarity_error( z );
  retval( 3 ) = QP.get_iterations(  );
  
  if ( M ) { delete M ; M = 0; }
  return retval;

}


DEFUN_DLD( band_multiply, args, nargout, "multiply"){

  octave_value_list retval;

  qp_diag4 * M = read_band_matrix( args, 0 );
  
  double alpha = 0.0;
  double beta =  1.0;
  int left_set = diag4::ALL;
  int right_set = diag4::ALL;
  
  
  ColumnVector  X   = static_cast<ColumnVector>(args(1).column_vector_value());
  ColumnVector  Y(X.length(), 0.0);   
  if ( args.length() > 2 ){
       Y = static_cast<ColumnVector>(args(2).column_vector_value());
  }
  if ( args.length() > 3 ){
    alpha = static_cast<double>(args(3).scalar_value());
  }
  if ( args.length() > 4 ){
    beta = static_cast<double>(args(4).scalar_value());
  }
  if ( args.length() > 5 ){
    left_set = static_cast<int>(args(5).scalar_value());
  }
  if ( args.length() > 6 ){
    right_set = static_cast<int>(args(6).scalar_value());
  }

  std::valarray<double> x( X.length() ); oct_to_val(X, x);
  std::valarray<double> y( X.length() ); oct_to_val(Y, y);
  

  // M->multiply( x, y, alpha, beta);
  M->multiply_submatrix(  x,   y, alpha, beta, left_set, right_set);
  
  
  
  val_to_oct( y, X );

  retval( 0 ) = X;
  
  if ( M ) { delete M ; M = 0; }
  return retval;

}



/**
 *  This solves a subproblem as understood from the LCP perspective.  Given
 *  active variables A and inactive variables B, we solve
 *  M( A, A ) z( Z ) = -q( A ) - M(A, B) b(B)
 *  where b are the bounds on the z variables. 
 */

DEFUN_DLD( band_solvesubproblem, args, nargout , "Solve a subproblem ") {

  octave_value_list retval;

  int argins = args.length();
  qp_diag4 * m = read_band_matrix( args, 0 );

  ColumnVector Q = static_cast<ColumnVector>(args(1).column_vector_value());
  valarray<Real> q( Q.length() );
  oct_to_val( Q, q);


  ColumnVector L = static_cast<ColumnVector>(args(2).column_vector_value());
  valarray<Real> l( Q.length() );
  oct_to_val( L, l);
  ColumnVector Z( Q.length() );
  valarray<Real> z( Z.length() );

  ColumnVector U = static_cast<ColumnVector>(args(3).column_vector_value());
  valarray<Real> u( U.length() );
  oct_to_val( U, u);

  ColumnVector IDX = static_cast<ColumnVector>(args(4).column_vector_value());
  valarray<int> idx( IDX.length() );

  oct_to_val_int(IDX , idx);

  m->solve_subproblem(q, l, u, idx, z);

  val_to_oct( z, Z);
  retval(0) = Z;
  delete m;
  return retval;
}


/**
 *  This is needed by the LCP solver which requires columns of the inverse
 *  of the *active* matrix  M( A, A ) so that here, we compute 
 *  v = - M( A, A ) \ M( A, variable )
 *  and variable is *not* in the set A.
 */
DEFUN_DLD( band_searchdirection, args, nargout , "Get a column of the inverse of the active matrix") {

  octave_value_list retval;
  int argins = args.length();
  qp_diag4 * m = read_band_matrix( args, 0 );

  int variable = static_cast<double>(args(1).scalar_value()) - 1;

  ColumnVector V( m->size());

  valarray<Real> x( m->size() );

  double mtt;

  m->get_search_direction(variable,  x, mtt);
  val_to_oct( x, V);
  retval(0) = V;
  retval(1) = mtt;
  delete m;
  return retval;
}

/**
 *  Unit test
 */
DEFUN_DLD( band_get_column, args, nargout , "Get a column of the matrix") {

  octave_value_list retval;
  int argins = args.length();
  qp_diag4 * m = read_band_matrix( args, 0 );

  int variable = static_cast<int>(args(1).scalar_value()) - 1;

  ColumnVector V( m->size() );
  valarray<double> x( m->size() );


  m->get_column(x, variable);
  
  val_to_oct( x, V);
  retval(0) = V;
  delete m;
  return retval;
}

DEFUN_DLD( band_convert, args, nargout , "Create a matrix and return a struct")
{

  octave_value_list retval;
  int argins = args.length();
  int N = 9;
  int BW =  3;
   
  band_diag m( N , BW, true);

  m[ 0 ] = 40;
  for ( size_t j = 1; j< BW; ++j ){ 
    m[ j ] = - ( double )( BW - j );
  }
  
  octave_scalar_map st;
  
  m.convert_matrix_oct( st );

  retval(0) = st;
  return retval;
}

DEFUN_DLD( clutch_matrix, args, nargout , "Create the clutch matrix") {

  octave_value_list retval;
  double masses[]{1,2,3,4};
  double k1   = 1e-1;
  double k2   = 1e4;
  double force = 1e3;
  double mu = 1.0;
  double lo[] = {-0.5, -1.0};
  double up[] = { 0.5,  1.0};
  double torque_in = 100.0;
  double torque_out = 0.0;
  double compliances[]= {1e-10, 1e-10, 1e-10};
  double step  = 1.0/100.0;
  double theta = 2.0;
  double x[] = {0.0, 0.0, 0.0, 0.0};
  double v[] = {0.0, 0.0, 0.0, 0.0};
  size_t n_steps = 1;
  
  

  /// parse the arguments if available. 
  if ( args.length() >  0 ) {

    octave_scalar_map clutch_params = args(0).scalar_map_value ();
    octave_value tmp;
 
    tmp = clutch_params.contents ("x");
    if ( tmp.is_defined( ) ) {
      ColumnVector X = static_cast<ColumnVector>( tmp.column_vector_value( ) );
      for ( size_t i = 0; i < sizeof( x ) / sizeof( x[ 0 ] ); ++i ){
        x[ i ] = X( i );
      }
    }
  
    tmp = clutch_params.contents ("v");
    if ( tmp.is_defined( ) ) {
      ColumnVector X = static_cast<ColumnVector>( tmp.column_vector_value( ) );
      for ( size_t i = 0; i < sizeof( v ) / sizeof( v[ 0 ] ); ++i ){
        v[ i ] = X( i );
      }
    }   

    tmp = clutch_params.contents ("masses");
    if ( tmp.is_defined( ) ) {
      ColumnVector x = static_cast<ColumnVector>( tmp.column_vector_value( ) );
      for ( size_t i = 0; i < sizeof( masses ) / sizeof( masses[ 0 ] ); ++i ){
        masses[ i ] = x( i );
      }
    }

    tmp = clutch_params.contents ("k1");
    if ( tmp.is_defined( ) ) {
      k1 = static_cast<double>( tmp.scalar_value( ) );
    }
    
    tmp = clutch_params.contents ("k2");
    if ( tmp.is_defined( ) ) {
      k2 = static_cast<double>( tmp.scalar_value( ) );
    }
    
    tmp = clutch_params.contents ("force");
    if ( tmp.is_defined( ) ) {
      force = static_cast<double>( tmp.scalar_value( ) );
    }
    
    tmp = clutch_params.contents ("mu");
    if ( tmp.is_defined( ) ) {
      mu = static_cast<double>( tmp.scalar_value( ) );
    }
    
    tmp = clutch_params.contents ("torque_in");
    if ( tmp.is_defined( ) ) {
      torque_in = static_cast<double>( tmp.scalar_value( ) );
    }
    
    tmp = clutch_params.contents ("torque_out");
    if ( tmp.is_defined( ) ) {
      torque_out = static_cast<double>( tmp.scalar_value( ) );
    }
    
    tmp = clutch_params.contents ("step");
    if ( tmp.is_defined( ) ) {
      step = static_cast<double>( tmp.scalar_value( ) );
    }
 
    tmp = clutch_params.contents ("n_steps");
    if ( tmp.is_defined( ) ) {
      n_steps = static_cast<size_t>( tmp.scalar_value( ) );
    }   

    tmp = clutch_params.contents ("theta");
    if ( tmp.is_defined( ) ) {
      theta = static_cast<double>( tmp.scalar_value( ) );
    }
    
    tmp = clutch_params.contents ("lo");
    if ( tmp.is_defined( ) ) {
      ColumnVector x = static_cast<ColumnVector>( tmp.column_vector_value( ) );
      for ( size_t i = 0; i < sizeof( lo ) / sizeof( lo[ 0 ] ); ++i ){
        lo[ i ] = x( i );
      }
    }
    
    tmp = clutch_params.contents ("up");
    if ( tmp.is_defined( ) ) {
      ColumnVector x = static_cast<ColumnVector>( tmp.column_vector_value( ) );
      for ( size_t i = 0; i < sizeof( up ) / sizeof( up[ 0 ] ); ++i ){
        up[ i ] = x( i );
      }
    }
    
    tmp = clutch_params.contents ("compliances");
    if ( tmp.is_defined( ) ) {
      ColumnVector x = static_cast<ColumnVector>( tmp.column_vector_value( ) );
      for ( size_t i = 0; i < sizeof( compliances ) / sizeof( compliances[ 0 ] ); ++i ){
        compliances[ i ] = x( i );
      }
    }

  }
  
  
  clutch_sim clutch(
    masses,
    k1,                       // first spring constant
    k2,                        // second spring constant
    force,                        // force
    mu,                        // friction coefficient
    torque_in,                          // torque on input shaft
    torque_out,                          // torque on output shaft
    step,                  // time step
    theta * step,
    lo,     // lower range bounds
    up,        // upper range bounds
    compliances[0],
    compliances[1],
    compliances[2]
    );

  for ( size_t i = 0; i < sizeof( x ) / sizeof( x[ 0 ] ); ++i ){
    clutch.x[ i ] = x[ i ];
  }
  
  for ( size_t i = 0; i < sizeof( x ) / sizeof( x[ 0 ] ); ++i ){
    clutch.v[ i ] = v[ i ];
  }
  
  
  octave_scalar_map st;
  clutch.M.convert_matrix_oct( st );

  clutch.do_step( n_steps );
  ColumnVector R( clutch.M.size() );
  ColumnVector L( clutch.M.size() );
  ColumnVector U( clutch.M.size() );
  ColumnVector Z( clutch.M.size() );
  ColumnVector X( clutch.x.size() );
  ColumnVector V( clutch.v.size() );
  ColumnVector g( clutch.g.size() );
  val_to_oct( clutch.rhs, R);
  val_to_oct( clutch.lower, L);
  val_to_oct( clutch.upper, U);
  val_to_oct( clutch.z, Z);
  val_to_oct( clutch.x, X);
  val_to_oct( clutch.v, V);
  val_to_oct( clutch.g, g);

  retval(0) = st;
  retval(1) = R;
  retval(2) = L;
  retval(3) = U;
  retval(4) = Z;
  retval(5) = X;
  retval(6) = V;
  retval(7) = g;

  return retval;
}

