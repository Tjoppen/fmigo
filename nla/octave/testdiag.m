N = 200;
B = 10;
active = ones(N, 1);
#signs  = 1-2 * floor( 2 * rand(N, 1));
signs = active;
FREE = 0; 
TIGHT = 2;
LOWER = 2;
UPPER = 2;
ALL   = 6;

[m, dmat] = createbanded(N, 3, signs, active);

M0 = m;
banded = dmat;
x = rand(N, 1);
y = rand(N, 1);
alpha = 3; 
beta = 2; 
norm( ( beta * m * x + alpha * y ) - band_multiply(banded, x, y, alpha, beta) );



tests = struct(
	    "solve", 0, ...
	    "multiply_add", 0, ...
	    "multiply_add_sub", 0, ...
	    "search_direction", 0, ...
	    "velocity_multiply", 0, ...			  
	    "lcp", 1, ...			  
	    "solve_pp", 0);# rest not implemented, ...

##  "update_updown", 0, ...
##  "update_random", 0, ...
##  "factor", 0, ...	
##  "update_chosen", 0, ...
##  "io", 0, ...
## );



if ( tests.multiply_add)
  errmult = [];
  for i = 1:1000
    B = 1 - 2 * rand( size(M0, 1), 1);
    x = 1 - 2 * rand( size(M0, 1), 1);
    alpha = 1-2 * rand();
    beta = 1-2 * rand();
    y  = band_multiply(dmat, B, x, alpha, beta);
    x = alpha * x + beta * M0 * B;
    errmult = [errmult; norm(y-x)];
  endfor
  tests.multiply_add = errmult;
endif


if ( tests.solve )
  errsolve = [];
  for i = 1:100
    dmat.active = rand( size(dmat.active) )  > 0.4;
    idx = dmat.active;
    ix  = find(idx!=0);
    jx  = find(idx==0);
    X = (1:size(M0, 1))';
    X(jx) = 0;
    B = zeros(size(M0, 1), 1);
    B(ix) = M0(ix, ix) * X(ix);
    
    x   = band_solve( dmat, B );
    errsolve = [errsolve; norm(x - X) ] ;
  endfor
  tests.solve = errsolve;
endif

if ( tests.multiply_add_sub )

  alpha = 1; 
  beta = 1;
  

  X = 0 * (size(M0, 1) : -1 : 1)';
  B = rand( size(M0, 1), 1 );

  errs = [];
  for i = 1:1000
    dmat.active = rand( size(dmat.active) )  > 0.4;
    y = [
	 band_multiply(dmat, B, X,  alpha, beta, FREE, FREE), ...
	 band_multiply(dmat, B, X, alpha, beta, FREE,  TIGHT), ...
	 band_multiply(dmat, B, X, alpha, beta, FREE,  ALL  ), ...
	 band_multiply(dmat, B, X, alpha, beta, TIGHT, FREE ), ...
	 band_multiply(dmat, B, X, alpha, beta, TIGHT, TIGHT), ...
	 band_multiply(dmat, B, X, alpha, beta, TIGHT, ALL  ), ...
	 band_multiply(dmat, B, X, alpha, beta, ALL  , FREE ), ...
	 band_multiply(dmat, B, X, alpha, beta, ALL  , TIGHT), ...
	 band_multiply(dmat, B, X, alpha, beta, ALL,   ALL  ) ...
    ];

    idx  = dmat.active;
    jdx = 1 - idx;
    ix  = find(idx!=0);
    jx  = find(idx==0);

    w = [];
    x = 0 * B;
    ## left set is ix
    x(ix) = alpha * X(ix) + beta * M0(ix, ix) * B(ix);  
    x(jx) = 0 ; 
    w = [w, x];

    x(:) = 0 ;
    x(ix) = alpha *X(ix) + beta * M0(ix, jx) * B(jx);
    x(jx) = 0;
    w = [w, x];
    
    
    x(:) = 0 ;
    x(ix) = alpha *X(ix) + beta * M0(ix, :) * B(:);
    x(jx) = 0;
    w = [w, x];


    ## left set is jx
    x(jx) = alpha * X(jx) + beta * M0(jx, ix) * B(ix);  
    x(ix) = 0 ; 
    w = [w, x];

    x(:) = 0 ;
    x(jx) = alpha *X(jx) + beta * M0(jx, jx) * B(jx);
    x(ix) = 0;
    w = [w, x];
    
    x(:) = 0 ;
    x(jx) = alpha *X(jx) + beta * M0(jx, :) * B(:);
    x(ix) = 0;
    w = [w, x];

    ## left set is [ix, jx]
    x = alpha * X + beta * M0(:, ix) * B(ix);  
    w = [w, x];

    x(:) = 0 ;
    x = alpha *X + beta * M0(:, jx) * B(jx);
    w = [w, x];
    
    x(:) = 0 ;
    x = alpha *X + beta * M0(:, :) * B(:);
    w = [w, x];

    for i = 1:size(y, 2)
      errs = [errs; norm(y(:, i) - w(:, i))];
    endfor
  endfor
  tests.multiply_add_sub = errs;
endif


if ( tests.solve_pp )
  
  N = size(M0, 1);
  l = - inf * ones(N , 1 );
  u =   inf * ones(N, 1 );

  errs = [];

  for i = 1:100
    l = -5 * rand( length( l ), 1 );
    u =  5 * rand( length( u ), 1 );
    idx = FREE * ones(N, 1 );
    idx = 2 * floor( 2.4 * rand( length( idx ) , 1));
    q  =  10 * ( 1 - 2 * rand( N, 1 ));
    T = find( idx > 0 );
    F = find( idx <= 0 );
    z1 = solve_subproblem(M0, q, l, u, idx);
    z = band_solvesubproblem(dmat, q, l, u, idx);
    errs = [errs;norm(z - z1)];
  endfor
  tests.solve_pp = errs;
endif



  if ( tests.search_direction )
    errs = [];
    for i=1:100
      dmat.active = rand( size(dmat.active) )  > 0.4;
      for variable = 1:N		# the absolute value of the variables are
					# used: they must be even
	if ( dmat.active( variable ) )
	  v = band_searchdirection(dmat, variable);
	  idx  = dmat.active;
	  jdx = 1 - idx;
	  ix  = find(idx!=0);
	  jx  = find(idx==0);
	  v0 = 0*v;
	  v0(ix) = - M0(ix, ix) \ M0(ix, variable);
	  eee = norm(v0-v);
	  if ( eee > 1e-4 ) 
	    return
	  endif
	  errs = [errs; norm(v0-v) ];
	endif

      endfor
    endfor
    tests.search_direction = errs;
  endif

  if ( tests.lcp)
    errs = [];
    iterations = [];
    for i = 1:10
      lower = -0.2 - 10 * rand( N, 1 );
      upper =  0.2 + 10 * rand( N, 1 );
      q = ( 1 - 2 * rand( N, 1 ) );
      [z0, status, stats] = ...
      boxed_keller( M0, q, lower, upper, struct("max_it", 4*N) );
      [z, w, err, it]  = band_lcp(banded, q, lower, upper );
      iterations = [iterations;[stats.iterations, it]];
      errs = [errs; norm(z-z0)];
    endfor
    tests.lcp = errs;
    tests.lcp_iterations = iterations;
    
  endif


if 0
  if ( tests.io)
    [fdmat  ] = band_io( dmat  );
    [fdmatl ] = band_io( dmatl );
  endif








  ## basic factorization error: factors are used to reconstruct the matrix
  ## and this is then compared directly with the original. 
  if ( tests.factor )
    errfactor = [];
    for i = 1:1
      [fdmat0] = band_factor(dmat);
      [mm0, S0, DD0, LL0] = diag4reassemble (fdmat0);
      errfactor = [errfactor; norm(mm0 - M, 1 )];
    endfor
    tests.factor = errfactor;
  endif

  if ( tests.update_chosen  )
    errupdate = [];
    ## down date some variables.  
    ## fmat0 always contains the current factors and is simply fed back a the
    ## process goes on. 
    for variable = [n, n-2, n-3, n-4]
      fmat0 = band_factor(dmat, fdmat0, variable);
      active( variable ) = ~active( variable );
      mm0 = diag4reassemble (fdmat0); 
      errupdate = [errupdate; norm(  getactiveidx(M, active ) - getactiveidx( mm0, active ) , 1) ];
    endfor
    tests.update_chosen = errupdate;
  endif

  if ( tests.update_updown )
    errupdate = [];
    ## all the way up and down: there is sometimes a glitch when overwrite
    ## input arguments with the output. 
    for variable = [n:-1:1, 2:n]
      fmat1 = band_factor(dmat, fdmat0, variable);
      fmat0 = fmat1;
      active( variable ) = ~active( variable );
      mm0 = diag4reassemble (fdmat0);
      errupdate = [errupdate; norm(  getactiveidx(M, active ) - getactiveidx( mm0, active ), 1 ) ];
    endfor
    tests.update_updown = errupdate;
  endif

  if ( tests.update_random )
    errupdate = [];
    ## go nuts: flip update and downdate a large number of variables
    for i = 1:1000
      variable = ceil( n * rand);
      fmat0 = band_factor(dmat, fdmat0, variable);
      active( variable ) = ~active( variable );
      mm0 = diag4reassemble (fdmat0);
      errupdate = [errupdate; norm(  getactiveidx(M, active ) - getactiveidx( mm0, active ) , 1) ]; 
    endfor
    tests.update_random = errupdate;

  endif


endif
