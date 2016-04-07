#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "tridiag_ldlt.h"

/**
   Allocation without any checks for error.

   The presumption here is that a copy of the original matrix should be
   kept for multiplication operations.  
 */
tri_matrix  tri_matrix_alloc( int n ){

  tri_matrix   mat;
  mat.ready = 0;
  mat.n = n; 
  mat.diag = (double * ) malloc(    n      * sizeof( double ) );
  mat.sub  = (double * ) malloc( ( n - 1 ) * sizeof( double ) );

  mat.factored      = ( tri_matrix * ) malloc( sizeof( tri_matrix ) );
  mat.factored->diag =  (double * ) malloc(    n      * sizeof( double ) );
  mat.factored->sub  =  (double * ) malloc( ( n - 1 ) * sizeof( double ) );
  mat.factored->n    =  n; 

  return  mat;

};


/**
   No error checking: use at your own risks!
 */
void tri_matrix_free( tri_matrix  m ){

  free ( m.diag ) ; 
  free ( m.sub  ) ; 
  free ( m.factored->diag ) ; 
  free ( m.factored->sub  ) ; 
  free ( m.factored       ) ; 
  
  return; 

}

/**
   This is LDLT factorization but we keep inv(D) instead.
   In place factorization. 
*/
int tri_factor( tri_matrix  * m ) 
{
  int i;
  if ( ! m->ready ) { 
    memcpy( m->factored->diag, m->diag,    m->n      * sizeof( double ) );
    memcpy( m->factored->sub , m->sub , ( m->n - 1 ) * sizeof( double ) );

    for ( i = 0; i< m->n - 1; ++i ){
      m->factored->diag[ i   ]  = 1.0  /  m->factored->diag[ i ] ;
      m->factored->diag[ i+1 ] -= m->factored->sub[ i ] * m->factored->sub[ i ] * m->factored->diag[ i ];
      m->factored->sub [ i   ] *= m->factored->diag[ i ];
    }

    m->factored->diag[ m->n-1 ] =  1.0 / m->factored->diag[ m->n-1 ];
    m->ready = 1; 
  } 

  return 1;
}

/**
   Forward elimination to solve with the lower triangle followed by
   multiplication with the inverse diagonal. 
*/
void tri_solve_lower( tri_matrix * m, double * rhs ){
  int i;

  if ( ! m->ready ){
    tri_factor( m );
  }

  /* Forward elimination including multiplication by inv( D )*/
  for ( i = 1; i< m->n ; ++i ){
    rhs[ i     ] -=  m->factored->sub [ i - 1 ] * rhs[ i - 1  ];
    rhs[ i - 1 ] *=  m->factored->diag[ i - 1 ];
  }
  rhs[ m->n - 1 ] *=  m->factored->diag[ m->n - 1 ];
}

/**
   Upper triangle solve, i.e., backsubstitution.
 */
void tri_solve_upper( tri_matrix * m, double * rhs )
{
  int i; 
  if ( ! m->ready ){
    tri_factor( m );
  }

  for ( i =  m->n-2; i >= 0; --i){
    rhs[ i ]  =  rhs[ i ] - m->factored->sub[ i ] * rhs[ i+1];
  }

}

void tri_solve( tri_matrix * m, double * rhs ){
  tri_solve_lower( m, rhs ); 
  tri_solve_upper( m, rhs ); 
}





/**
   Multiply symmetric tridiagonal matrix defined with diag and sub with x
   y = b * y + a * M * x ; 
   NOTE: this violates the BLAS convension where "a" would precede "x" and
   "b" would precede y. 
*/
void tri_multiply( tri_matrix m, double * x, double *y, double a, double b ) {
  
  int i;
  y[ 0 ] = b * y[ 0 ] + a * ( m.diag[ 0 ] * x[ 0 ] + m.sub[ 0 ] * x[ 1 ] ) ;
   
  for (i = 1; i < m.n-1; ++i ){
    y[ i ] = b * y[ i ] + a * ( m.diag[ i ] * x[ i ] + m.sub[ i-1 ] * x[ i-1 ] + m.sub[ i ] * x[ i + 1 ] );
  }
  y[ m.n-1 ] = b * y[ m.n-1 ] + a * ( m.diag[ m.n-1 ] * x[ m.n-1 ] + m.sub[ m.n-2 ] * x[ m.n-2 ] );
}

