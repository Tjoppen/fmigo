clutchmatrix;
N = size( M, 1 );
lower = -0.2 - 10 * rand( N, 1 );
upper =  0.2 + 10 * rand( N, 1 );
nt = 10000;
q = ( 1 - 2 * rand( N, 1 ) );
#[z0, status, stats] = boxed_keller( MB, q, lower, upper );
#[z, w, err, iterations]  = diaglcp(bandmatrix, q, lower, upper );

#[w1]  = diagmult(bandmatrix, z);
#[z0, z, abs(z0 - z), stats.index]
#[  ]
#
#[w, w1  + q, stats.w]
#


errs = [];
for i=1:nt
  bandmatrix.active = rand( size(bandmatrix.active) )  > 0.4;
  ix = find( bandmatrix.active == 1 );
  jx = find( bandmatrix.active == 0 );
  x = 10*( 1 - 2 * rand(N, 1) ); 
  x( jx ) = 0;
  b = 0 * x;
  b( ix ) = MB(ix, ix) * x( ix ); 
  x1 = diagsolve(bandmatrix, b ); 
  x1( jx ) = 0;
  
  errs = [errs; norm(x-x1)];
endfor
