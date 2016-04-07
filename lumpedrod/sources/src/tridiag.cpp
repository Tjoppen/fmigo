
#define OCTAVE
#ifdef OCTAVE
#include <octave/oct.h>
#endif

#include "tridiagcode.c"

DEFUN_DLD( tridiag, args, nargout , "LDLT of tridiagonal") {

  ColumnVector  diag = static_cast<ColumnVector>(args(0).column_vector_value());
  ColumnVector  sub = static_cast<ColumnVector>(args(1).column_vector_value());
  ColumnVector  x = static_cast<ColumnVector>(args(2).column_vector_value());
  int n  = diag.length();
  ColumnVector  y(n, 0.0);
  ColumnVector  D(n, 0); 
  ColumnVector  S(n-1, 0); 
  double compliance = 1e-10;
  tri_matrix  m;
  m  = tri_matrix_alloc( n );
  memcpy(m.diag, diag.fortran_vec(), n       * sizeof( double ) );
  memcpy(m.sub, sub.fortran_vec(), ( n - 1 ) * sizeof( double ) );
  factorm( &m );
  y = x;
  solvem( m, y.fortran_vec() );
//  forward_elim( m, y.fortran_vec() );
 // back_sub( m, y.fortran_vec() );

  memcpy(D.fortran_vec(), m.factored->diag,    n     * sizeof( double ) );
  memcpy(S.fortran_vec(), m.factored->sub , ( n - 1 ) * sizeof( double ));
  
  tri_matrix_delete( m );

  octave_value_list retval;
  retval(0) = y;
  retval(1) = D;
  retval(2) = S;
  return retval;
}

