FREE = 0; 
TIGHT = 2;
ALL   = 6;

#[m, dmat] = createbanded(N, 3, signs, active);
N = 20;
[m, dmat] = createwire(N, 20, 1e-4, 400);
N = size(m, 1);
length = 1;
if ( length ) 
  dmat.signs = [dmat.signs; -1];
  G =     zeros(N+1, 1);
  G(1:4:end) = 1;
  G(end) = dmat.signs(end)*1;
  m = [m, dmat.signs(end)*G(1:end-1); G'(1:end-1), dmat.signs(end)*G(end)];
  dmat.G = G;
  N = N + 1 ; 
  dmat.active = [dmat.active; 1];
  sc = m(end,end) - m(end, 1:end-1) * ( m(1:end-1, 1:end-1) \ m(1:end-1, end ) );
endif
M0 = m;

banded = dmat;

tests = struct( ...
		"multiply_add", 0, ...
		"solve", 0, ...
		"multiply_add_sub", 0, ...
		"search_direction", 0, ...
		"velocity_multiply", 0, ...			  
		"do_step", 0, ...			  
		"get_column", 1, ...			  
		"multiply_add_column", 0, ...			  
		"lcp", 0, ...			  
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
    z = x;
    z = alpha * z + beta * M0 * B;

    

    errmult = [errmult; norm(y-z)];
  endfor
  tests.multiply_add = errmult;
endif


if ( tests.solve )
  errsolve = [];
  for i = 1:100 #
    dmat.active = rand( size(dmat.active) )  > 0.4;
    idx = dmat.active;
    ix  = find(idx!=0);
    jx  = find(idx==0);
    X = (1:size(M0, 1))';
    X(jx) = 0;
    B = zeros(size(M0, 1), 1);
    B(ix) = M0(ix, ix) * X(ix);
    
    x   = band_solve( dmat, B );
    y  = band_multiply(dmat, x, 0*B, 0, 1);
    errsolve = [errsolve; norm(x - X) ] ;
  endfor
  tests.solve = errsolve;
endif

if ( tests.multiply_add_sub )

  alpha = 1; 
  beta = 2;
  

  X = 2 * ones( size(m, 1), 1 ); #) (size(M0, 1) : -1 : 1)';
  B = 2 * ones( size(m, 1) , 1); #) (size(M0, 1) : -1 : 1)';
  B = rand( size(M0, 1), 1 );

  errs = [];
  for i = 1:10
    dmat.active =  rand( size(dmat.active) )  > 0.4;
    dmat.active(1:4:end) = 1;
    #dmat.active = ones( size(dmat.active) )  ;
    y = [
	 band_multiply(dmat, B, X, alpha, beta, TIGHT, TIGHT), ...
	 band_multiply(dmat, B, X, alpha, beta, TIGHT, FREE ), ...
	 band_multiply(dmat, B, X, alpha, beta, TIGHT, ALL  ), ...
	 band_multiply(dmat, B, X, alpha, beta, FREE,  TIGHT), ...
	 band_multiply(dmat, B, X, alpha, beta, FREE, FREE), ...
	 band_multiply(dmat, B, X, alpha, beta, FREE,  ALL  ), ...
	 band_multiply(dmat, B, X, alpha, beta, ALL  , TIGHT), ...
	 band_multiply(dmat, B, X, alpha, beta, ALL  , FREE ), ...
	 band_multiply(dmat, B, X, alpha, beta, ALL,   ALL  ) ...
    ];

    idx  = dmat.active;
    jdx = 1 - idx;
    T  = find(idx ==0);
    F  = find(idx!=0);

    w = [];
    x =  X;
    ## left set is T
    x(T) = alpha * X(T) + beta * M0(T, T) * B(T);  
    w = [w, x];

    X = X;
    x(T) = alpha *X(T) + beta * M0(T, F) * B(F);
    w = [w, x];
    
    
    x = X;
    x(T) = alpha *X(T) + beta * M0(T, :) * B(:);
    w = [w, x];


    ## left set is F
    x = X;
    x(F) = alpha * X(F) + beta * M0(F, T) * B(T);  
    w = [w, x];

    x  = X ;
    x(F) = alpha *X(F) + beta * M0(F, F) * B(F);
    w = [w, x];
    
    x = X;
    x(F) = alpha *X(F) + beta * M0(F, :) * B(:);
    w = [w, x];

    ## left set is [T, F]
    x = X;
    x = alpha * X + beta * M0(:, T) * B(T);  
    w = [w, x];

    x = X;
    x = alpha *X + beta * M0(:, F) * B(F);
    w = [w, x];
    
    x = X;
    x = alpha *X + beta * M0(:, :) * B(:);
    w = [w, x];

    ee = [];
    for i = 1:size(y, 2)
      ee = [ee, norm(y(:, i) - w(:, i))];
    endfor
    errs = [errs; ee];
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
	c = band_get_column(dmat, variable);
	v = band_searchdirection(dmat, variable);
	idx  = dmat.active;
	jdx = 1 - idx;
	ix  = find(idx!=0);
	jx  = find(idx==0);
	v0 = 0*v;
	v0(ix) = - M0(ix, ix) \ M0(ix, variable);
	eee = norm(v0-v);
	if ( eee > 1e-4 ) 
	  disp("Unreasonably large error in search_direction")
	  return
	endif
	errs = [errs; norm(v0-v) ];
      endif
    endfor
  endfor
  tests.search_direction = errs;
endif



if ( tests.get_column )
  errs = [];
  for i=1:1
    dmat.active = rand( size(dmat.active) )  > 0.4;
    for variable = 1:N		# the absolute value of the variables are
				# used: they must be even
      if ( dmat.active( variable ) )
	c = band_get_column(dmat, variable);
	ix  = find( dmat.active != 0 );
	jx  = find( dmat.active == 0 );
	c0 = 0*c;
	c0(ix) = M0(ix, variable);
	eee = norm(c0-c);
	errs = [errs; norm(c0-c) ];
      endif

    endfor
  endfor
  tests.get_column = errs;
endif
if ( tests.lcp)
  errs = [];
  sols = [];
  iterations = [];
  bad_problems = [];
  for i = 1:100
  #for i = 1:1
    lower = -0.2 - 10 * rand( N, 1 );
    upper =  0.2 + 10 * rand( N, 1 );
    q = ( 1 - 2 * rand( N, 1 ) );
    if ( 0 )
    q = -2 * ones(N, 1);
    lower = -5 * ones( N, 1 );
    upper =  5 * ones( N, 1 );
    q = -2 * ones(N, 1);
    endif
    if ( 0 )
      lower = [ -5.29401, -0.76842, -6.31233, -9.69345, -8.15717  ]';
      upper = [ 2.5920, 9.9494, 7.9570, 5.5935, 3.7517 ]';
      q     = [ -0.62122, -0.24212, 0.90153, 0.33088, -0.29648 ]';
    endif
    if ( 0 )

      lower =[ -0.42134 -2.75939 -5.57951 -0.65460 -4.43537]';

      upper =[ 4.3674 6.8733 9.7850 5.3303 1.0005 ]';

      q     =[ 0.78888 0.14983 0.32344 0.32973 -0.33679 ]';


    endif
    [z0, status, stats] = ...
    boxed_keller( M0, q, lower, upper, struct("max_it", 4*N) );
    w0 = stats.w;
    [z, w, err, it, idx]  = band_lcp(banded, q, lower, upper );
    iterations = [iterations;[stats.iterations, it]];
    errwz = norm(w-(m*z+q));
    nzz0 = norm(z-z0);
    nww0 = norm(w-w0);
    emin = 1e-8;
    if ( errwz >  emin || nzz0 > emin || nww0 > emin ) 
      bad_problems{end+1} = struct("l", lower, "u", upper, "q", q, ...
				  "z", z, "w", w, "z0", z0, "w0", w0, ...
				  "nzz0", nzz0, "nww0", nww0, "errwz", errwz, ...
				  "it", it, "errqp", err, "problem", i);
    endif
    errs = [errs; eps + [ errwz, norm(z-z0), norm(w-w0),  ... 
		    get_complementarity_error( z, w, lower, upper, 1e-9 ), ...
		    get_complementarity_error( z0, w0, lower, upper, 1e-9 ) ] ];
    
    sols{end+1} = [z,w,z0,w0]; 
  endfor
  tests.lcp = struct("errors", errs, "solutions", sols);
  tests.lcp_iterations = iterations;
endif

if ( 0 )
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

if ( tests.do_step )
  errs = [];
  #z0 = (1-2*rand( N, 1 ) );
  #w0 = (1-2*rand( N, 1 ) );
  #v = (1-2*rand( N, 1 ) );
  z0 = ones( N, 1 );
  w0 = ones( N, 1 );
  v  = ones( N, 1 );
  step = 2 ; # * rand();
  driving = 3; #ceil ( N  * rand() );
  dmat.active(:) = 1;
  T = find( dmat.active == 0 );
  F = find( dmat.active );
  [z, w]  = band_do_step( dmat, z0, w0, v, step, driving);
  [z1, w1] = keller_do_step(m, z0, w0, step, v, 0, driving, T, F);
  errs = [errs; [ norm(z-z1), norm(w-w1)]];
  
  tests.do_step = errs;
endif
