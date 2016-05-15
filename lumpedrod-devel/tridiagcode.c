#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>


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

/**
   The lumped rod has position, velocity and masses for all elements, as
   well as compliance for all connections.  

   Damping is left to the stepper though so that we can maintain stability.
*/
typedef struct lumped_rod{
  int n;			/* number of elements */
  double * x;			/* positions */
  double * v;			/* velocities */
  double * torsion;		/* constraint forces along the rod */
  double   rod_mass;		/* total mass: elements have mass mass/n */
  double   mass;		/* element mass */
  double   compliance;		/* global compliance: the compliance used in the constraints is compliance/n  */
  double mobility[ 4 ];		/* rod's mobility wrt forcing at each end */
} lumped_rod;


/**
   The full simulation struct which includes global parameters.
*/
typedef struct lumped_rod_sim{
  tri_matrix m;			/* system matrix */
  lumped_rod rod;
  double  forces[2];		/* forces at each end: nothing in between */
  double * z;			/* buffer for solution: contains velocities and multipliers*/
  double step;			/* time step */
  double tau;			/* relaxation */
  double gamma_x;
  double gamma_v;
} lumped_rod_sim; 


void lumped_rod_sim_mobility( lumped_rod_sim sim, double * mob );
tri_matrix  tri_matrix_alloc( int n ); 
void tri_matrix_free( tri_matrix m );
int factorm( tri_matrix  * m);
void solvem( tri_matrix  m,  double * rhs);
void tridiagonal_solvem( tri_matrix  m, double *rhs );
void tridiagonal_multiplym( tri_matrix  m, double * x, double *y, double a, double b ) ;
void hadamard( double * x, int n,  double *y,  double *z, double a , double b );
void axpy(double *x, int n,  double *y, double a, double b);


lumped_rod  lumped_rod_alloc( int n, double mass, double compliance ) ;
void lumped_rod_free( lumped_rod rod );
void lumped_rod_store( lumped_rod rod, lumped_rod store );
void lumped_rod_restore( lumped_rod store, lumped_rod rod );


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


void tri_matrix_free( tri_matrix  m ){

  free ( m.diag ) ; 
  free ( m.sub  ) ; 
  free ( m.factored->diag ) ; 
  free ( m.factored->sub  ) ; 
  free ( m.factored       ) ; 
  
  return; 

}

lumped_rod  lumped_rod_alloc( int n, double mass, double compliance ) {
  lumped_rod  rod;

  rod.n = n;			
  rod.rod_mass = mass;
  rod.mass = rod.rod_mass / ( double ) rod.n ;
  rod.compliance = compliance;

  rod.x            = ( double * ) calloc(    n    ,    sizeof( double ) );		
  rod.v            = ( double * ) calloc(    n    ,    sizeof( double ) );		
  rod.torsion      = ( double * ) malloc( ( n - 1 )  * sizeof( double ) );		

  return rod;
}

void lumped_rod_free( lumped_rod rod ){

  free( rod.x           );
  free( rod.v           );
  free( rod.torsion     );

  return;
}


lumped_rod_sim lumped_rod_sim_alloc( int n ) {
  lumped_rod_sim  sim;
  
  sim.z       = (double * ) malloc( ( 2 * n + 1 ) * sizeof( double ) );

  return sim;
}


lumped_rod_sim lumped_rod_sim_free( lumped_rod_sim sim ) {

  free ( sim.z );
  lumped_rod_free( sim.rod );
  tri_matrix_free( sim.m );
}


/**
   Factor in place. 

   This is LDLT factorization but we keep inv(D) instead.
*/
int factorm( tri_matrix  * m ) 
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

/** What follows are functions for Cholesky factorization of the
 * tridiagonal matrix.  
 */
#if 0 
/**
   Factor in place
*/
int factor( double * diag, double * sub, int n)
{
  int i;
  for ( i = 0; i< n - 1; ++i ){
    diag[ i ] = sqrt( diag[ i ] );
    sub[ i ] /= diag[ i ];
    diag[ i+1 ] -= sub[ i ] * sub[ i ];
  }
  diag[ n-1 ] = sqrt( diag[ n-1 ] );
  return 1;
}


/**
   Solve in place
*/
void solve( double * diag, double *sub, int n, double * rhs)
{
  int i;
  /* Forward elimination */
  rhs[ 0 ] /= diag[ 0 ];
  for ( i = 1; i<n; ++i ){
    rhs[ i ] = ( rhs[ i ] - sub[ i-1 ] * rhs[ i-1 ] ) / diag[ i ];
  }

  /* Back substitution  */
  rhs[ n-1 ] /= diag[ n-1 ];
  for ( i = n-2; i >= 0; --i){
    rhs[ i ]  = ( rhs[ i ] - sub[ i ] * rhs[ i+1] ) / diag[ i ];
  }
}



/**
   Solve a linear system for a symmetric positive triagonal matrix.
   diag   : entries on the diagonal
   sub    : entries on the subdiagonal
   n      : size of the matrix (length of diag)
   b      : right hand side in   A x = b;
   diag is overwritten with the diagonal of the lower triangular left factor
   sub is overwritten with the subdiagonal of the lower triangular left factor
   b is overwritten with the solution

*/
void tridiagonal_solve( double *diag, double *sub, int n, double *rhs ){
  factor(diag, sub, n);
  solve(diag, sub, n, rhs);
}



#endif

/**
   Solve in place.   
   This is for LDLT factorization where we keep only inv(D)
*/


/**
   Forward elimination
*/
void forward_elim( tri_matrix m, double * rhs ){
  int i;

  if ( ! m.ready ){
    factorm( &m );
  }

  /* Forward elimination including multiplication by inv( D )*/
  for ( i = 1; i< m.n ; ++i ){
    rhs[ i     ] -=  m.factored->sub [ i - 1 ] * rhs[ i - 1  ];
    rhs[ i - 1 ] *=  m.factored->diag[ i - 1 ];
  }
  rhs[ m.n - 1 ] *=  m.factored->diag[ m.n - 1 ];
}

void back_sub( tri_matrix m, double * rhs )
{
  int i; 
  if ( ! m.ready ){
    factorm( &m );
  }

  /* Back substitution  */

  for ( i =  m.n-2; i >= 0; --i){
    rhs[ i ]  =  rhs[ i ] - m.factored->sub[ i ] * rhs[ i+1];
  }

}

void solvem( tri_matrix m, double * rhs ){
  forward_elim( m, rhs ); 
  back_sub( m, rhs ); 
}





/**
   Multiply symmetric tridiagonal matrix defined with diag and sub with x
   y = b * y + a * M * x ; 
   NOTE: this violates the BLAS convension where "a" would precede "x" and
   "b" would precede y. 
*/
void tridiagonal_multiplym( tri_matrix m, double * x, double *y, double a, double b ) {
  
  int i;
  y[ 0 ] = b * y[ 0 ] + a * ( m.diag[ 0 ] * x[ 0 ] + m.sub[ 0 ] * x[ 1 ] ) ;
   
  for (i = 1; i < m.n-1; ++i ){
    y[ i ] = b * y[ i ] + a * ( m.diag[ i ] * x[ i ] + m.sub[ i-1 ] * x[ i-1 ] + m.sub[ i ] * x[ i + 1 ] );
  }
  y[ m.n-1 ] = b * y[ m.n-1 ] + a * ( m.diag[ m.n-1 ] * x[ m.n-1 ] + m.sub[ m.n-2 ] * x[ m.n-2 ] );
}

/**
   Multiply two vectors component by component and put result in a third. 
*/

void hadamard( double * x, int n,  double *y,  double *z, double a , double b ){
  int i;
  for (i = 0; i<n; ++i )
    z[ i ] = b * z[ i ] + a * ( x[ i ] * y[ i ] );
}


/**
   Standard axpy operation:
   y = b * y + a * x;
*/
void axpy(double *x, int n,  double *y, double a, double b){
  int i;
  for (i = 0; i < n; ++i )
    y[ i ] = b * y[ i ]  + a * x[ i ];
}

/**

   This implements the matrix for a string or a chain of particle in the
   form of a permutation of 
   [ M  -G' ]
   [ G   T  ]

   Of course, we work with 
   [ M   G' ]
   [ G  -T  ]
   which is symmetric and indefinite

   The permutation is such that we have 

   [  m  -1   0  0  0 ...
   [  1   e  -1  0  0 ...
   [  0   1   m -1  0 ...      ]
   [  0   0   1  e -1 ... 

   .
   .
   .
   [  0   0  ...   1  m ]


   I.e., every odd (even when starting from 0) row is a 'mass row'   with 

   ...  0  1  m -1  0  ... 

   and every even row (odd when starting from 0) 
   has 
   ...  0  1  e -1  0

   What this means is that we are imposing constraints

   v[ k   ]  -  v[ k + 1 ] = 0

   There are `n'  mass rows and `n-1'  constraint rows

*/

tri_matrix build_rod_matrix( lumped_rod  rod, double step, double tau ) { 

  int n = rod.n;
  tri_matrix m; 		/* matrix  */
  double gamma = 1.0 /  ( 1.0 + 4.0 * tau  );
  double compliance = - ( 4.0  * gamma / step / step ) * 
    rod.compliance  / ( double ) ( rod.n - 1 );
  int i;

  m = tri_matrix_alloc( 2 * n - 1 );

  for( i = 0; i < m.n; i+= 2 ){
    m.diag[ i ] =  rod.mass;
  }

  for( i = 1; i < m.n; i+= 2 ){
    m.diag[ i ] =   compliance;
  }
    

  for( i = 0; i < n - 1; ++i ) {
    m.sub[ 2 * i     ] =   ( double ) 1.0;
    m.sub[ 2 * i + 1 ] = - ( double ) 1.0;
  }
  

  factorm( &m );			/* this only has to be factored once */

  return m;
  
}





/**
   This assumes that both first and last particles are attached by
   springs.  If this assumption changes, this needs revision.
   Spring constants are the sub-diagonals, and the diagonals are sums of
   neighboring spring constants.  This is valid for a chain of particles
   connected with springs. 
   step is the step-size of the simulation
   gamma  is   assumed to be gamma = 1 / ( 1 + 4 * rate / step)
  
*/
void build_string_matrix(double *spring_constant, int n, double step, double
			 gamma, double *diag, double * sub, double a, double b){
  double factor = step * step / 4.0 / gamma ;
  if ( b != 0 ){
    int i; 
    for (i = 0; i<n-1; ++i )
      sub[ i ] = b * sub[ i ] - a * factor * spring_constant[ i+1 ];
    for (i = 0; i<n; ++i )
      diag[ i ] = b * diag[ i ]  + a * factor * ( spring_constant[ i ] + spring_constant[ i+1 ] );
  }
  else {
    int i;
    for (i = 0; i<n-1; ++i )
      sub[ i ] =  - a * factor * spring_constant[ i+1 ];
    for (i = 0; i<n; ++i )
      diag[ i ] =   a * factor * ( spring_constant[ i ] + spring_constant[ i+1 ] );
  }
  
}




/**
   Specific to string using stiff spring and dampers
*/

void build_string_rhs(double * mass, double * forces, tri_matrix m, 
		      double * x, double * v, double gamma, double step)
{
  double  tmp[ m.n ];
  int i;
  
  for (i = 0 ; i < m.n; ++i ) tmp[ i ] =  gamma * ( (-4.0 / step ) * x[ i ] +   v[ i ] );

  for ( i = 0; i < m.n; ++i ){ v[ i ] = mass[ i ] * v[ i ] + step * forces[ i ]; }
  /**  Multiply and add the result to the velocity vector */
  tridiagonal_multiplym( m, tmp, v, 1.0, 1.0 ) ;
    
  
}

#if 0 
void step_string(double * mass, double * forces, double * x, double * v,
                 double step, double gamma, double * spring_constant, int n,
                 int N )
{
  double  diag[n];
  double  sub[n-1];
  double  diagf[n];
  double  subf[n-1];
  int i;

  build_string_matrix(spring_constant, n,  step,  gamma, diag,  sub, 1.0, 0.0);
  memcpy(subf, sub, n * sizeof( double ) );
  memcpy(diagf, diag, n * sizeof( double ) );
  axpy(mass, n, diagf, 1.0, 1.0);
  factor(diagf, subf, n);
  
  for ( i = 0; i < N; ++i ){
    build_string_rhs(mass, forces, n, diag, sub, x, v, gamma, step);
    solve(diagf, subf, n, v);
    axpy(v, n, x, step, 1.0);
  }
  
}

#endif
 


lumped_rod_sim create_sim( int N, double mass, double compliance, double step, double tau  ){
  lumped_rod_sim sim = lumped_rod_sim_alloc( N );
  sim.rod = lumped_rod_alloc( N, mass, compliance ); 
  sim.step = step;
  sim.tau  = tau;
  sim.gamma_v = ( double  ) 1.0  /  (  1.0  +  4.0  * tau );
  sim.gamma_x = - (double ) 4.0  * sim.gamma_v   /  step  ;

  sim.m = build_rod_matrix( sim.rod, sim.step, sim.tau ) ;
  lumped_rod_sim_mobility( sim, sim.rod.mobility );
  
  return sim;
}


void build_rod_rhs( lumped_rod_sim sim ){
  int i;
  
  for ( i = 0; i < sim.rod.n ; ++i ) {
    sim.z[ 2 * i ] = sim.rod.mass * sim.rod.v[ i ]; 
  }

  /* forces at the end */
  sim.z[  0          ] +=  sim.step * sim.forces[ 0 ]; 
  sim.z[ sim.m.n - 1 ] +=  sim.step * sim.forces[ 1 ]; 

  for ( i = 0; i <  sim.rod.n - 1 ;  ++i ) {

    sim.z[ 2 * i  + 1 ] =
      sim.gamma_x * ( sim.rod.x[  i  ] - sim.rod.x[ i + 1 ] )
      + sim.gamma_v * ( sim.rod.v[  i  ] - sim.rod.v[ i + 1 ] );

  }

}

/**  
     The rod can be forced at the first and last particle.  The mobility
     here is the 2x2 matrix of reaction forces given unit impulses at the
     two signals.  

     The output matrix is assumed to be row major
 */
void lumped_rod_sim_mobility( lumped_rod_sim sim, double  * mob ){
  /* force on first mass */
  bzero( sim.z, sim.m.n * sizeof( double ) ); 
  sim.z[ 0 ] = ( double ) 1.0; 
  solvem( sim.m, sim.z );
  /* collect results */ 
  mob[ 0 ] = sim.z[ 0 ]; mob[ 1 ] = sim.z[ sim.m.n - 1 ];

  bzero( sim.z, sim.m.n * sizeof( double ) ); 
  sim.z[ sim.m.n - 1  ] = ( double ) 1.0; 
  solvem( sim.m, sim.z );
  
  /* collect results */ 
  mob[ 2 ] = sim.z[ 0 ]; mob[ 3 ] = sim.z[ sim.m.n - 1 ];

}

void step_rod_sim( lumped_rod_sim sim, int n ){
  
  int i, j ; 
  for ( j = 0; j < n; ++j  ){

    build_rod_rhs(  sim );
    solvem( sim.m, sim.z );
  
    for ( i = 0; i < sim.rod.n; ++i ){
      sim.rod.v[ i ]  = sim.z[ 2 * i ]; 
      sim.rod.x[ i ] += sim.step * sim.rod.v[ i ]; 
    }

    for ( i = 0; i < sim.rod.n - 1 ; ++i ){
      sim.rod.torsion[ i ] = sim.z[ 2 * i + 1 ] /  sim.step ;
    }
  }
}

#if 0 


int main(){
  int n = 20;
  double compliance = 1e-10;
  tri_matrix m;
  m  = tri_matrix_alloc( n );
  factorm( &m ); 
  tri_matrix_free( m );


  return 0 ; 
}
#endif

/** set the forces at the ends */
void lumped_sim_set_force ( lumped_rod_sim * sim, double f, int j ){
  sim->forces[ j ] = f;
}


/** get the positions at the end */
double lumped_sim_get_position ( lumped_rod_sim  sim, int j ){

  if ( j ==  1 ){
    j = sim.rod.n - 1 ; 
  }

  return sim.rod.x[ j ];

}


/** get the velocities at the end */
double lumped_sim_get_velocity ( lumped_rod_sim sim, int j ){

  if ( j ==  1 ){
    j = sim.rod.n - 1 ; 
  }

  return sim.rod.v[ j ];

}



int main(){


  int N = 10; 			/* number of elements */
  double mass = 2;		/* rod mass  */
  double compliance = 1e-2;	/* inverse stiffness */
  double step = ( double ) 1.0 / ( double ) 10.0;
  double tau  = ( double ) 2.0 ;
  lumped_rod_sim sim;
  

  sim = create_sim( N, mass, compliance, step, tau  );
  lumped_sim_set_force ( &sim,  10.0, 0);
  lumped_sim_set_force ( &sim, -10.0, 1);
  step_rod_sim( sim, 40 );


    for ( int i = 0; i < sim.rod.n; ++i ){ 
    fprintf(stderr, "X[ %02d ] = %g\n", i, sim.rod.x[i]); 
    } 

  fprintf(stderr, "Mobility: \n %g %g\n %g %g \n\n",
	  sim.rod.mobility[0], sim.rod.mobility[1],
	  sim.rod.mobility[2], sim.rod.mobility[3]);

  lumped_rod_sim_free( sim );
  return 0 ;
}
