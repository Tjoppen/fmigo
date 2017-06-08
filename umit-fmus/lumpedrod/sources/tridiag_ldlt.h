#ifndef TRIDIAG_LDLT
#define TRIDIAG_LDLT


#ifdef __cplusplus
extern "C" {
#endif


/** 
    LDLT factorization of a tridiagonal matrix without any pivoting. 
 */ 


/** 

    BLAS conventions are respected as much as possible.  This means that
    the left most arguments are data sources, right most are target.
    Exceptions to this rule are for using default arguments in axpy type
    functions. 

    In general, we have:
    source array name, target array size, ... , target array name, target array
    size, optional scalars.

    BLAS always implements, for instance
    y  = b * y  + a * op(x);

    This holds when op is vector or matrix operation.

*/



/**
   A symmetric tridiagonal matrix.  
*/
typedef struct tri_matrix {
  int ready;			/* true once factored */
  int n ;
  double * diag;		/* diagonal  */
  double * sub;			/* sub diagonal (also super) */
  struct tri_matrix * factored;
} tri_matrix ;

/** General API: allocate, free, factor, etc. 
 * WARNING: no checks are made here. 
*/ 
tri_matrix  tri_matrix_alloc( int n ); 
void tri_matrix_free( tri_matrix m );

/** this is the only one which requires a pointer argument */ 
int tri_factor( tri_matrix  * m);

/** Solve the linear problem and overwrite rhs.  This will check whether
 * the matrix is factorized already and if not, do so. 
 */
void tri_solve( tri_matrix  * m,  double * rhs);

/** Forward elimination *and* diagonal solve in one. Used internally by
 * tri_solve. 
*/ 
void tri_solve_lower( tri_matrix * m, double * rhs );

/** Backward substitution, used internally by tri_solve.
*/
void solve_upper( tri_matrix * m, double * rhs );

/** Standard BLAS API here: data on the left, results on the right. 
 *  y = a * M * x + b
 */
void tri_multiply( tri_matrix  m, double * x, double *y, double a, double b ) ;

#ifdef __cplusplus
}
#endif

#endif
