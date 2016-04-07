
#define OCTAVE
#ifdef OCTAVE
#include <octave/oct.h>
#endif

#include "tridiagcode.c"

DEFUN_DLD( rod, args, nargout , "rod simulation") {

  ColumnVector  masses     = static_cast<ColumnVector>(args(0).column_vector_value());
  ColumnVector  compliance = static_cast<ColumnVector>(args(1).column_vector_value());
  double        h          = static_cast<double>(args(2).scalar_value());
  double     tau           = static_cast<double>(args(3).scalar_value());


  tri_matrix M; // = build_rod_matrix( masses.length(), masses.fortran_vec(), compliance.fortran_vec(), h, tau, -1,-1);
  
  ColumnVector  y(n, 0.0);
  ColumnVector  D(n, 0); 
  ColumnVector  S(n-1, 0); 

  memcpy(D.fortran_vec(), m.factored->diag,    n     * sizeof( double ) );
  memcpy(S.fortran_vec(), m.factored->sub , ( n - 1 ) * sizeof( double ));
  
  tri_matrix_delete( m );

  ColumnVector DD(M.n, 0);
  ColumnVector SS(M.n-1, 0);
  
  memcpy(DD.fortran_vec(), M.diag,   M.n       * sizeof( double ) );
  memcpy(SS.fortran_vec(), M.sub , ( M.n - 1 ) * sizeof( double ) );
  tri_matrix_delete( M );

  octave_value_list retval;
  retval(0) = y;
  retval(1) = D;
  retval(2) = S;
  retval(3) = DD;
  retval(4) = SS;
  return retval;
}

