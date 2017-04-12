N = 5;
D = 40 * ones(N, 1); 
S = -1 * ones(N-1,1);
SS = -2 * ones(N-2,1);
M0 = spdiags( [ [SS; 0; 0], [ S;  0], D, [ 0; S ] , [ 0;0;SS] ], ...
	      [ -2:2], sparse( length(D), length( D ) ) );

#x = ones(length( D ), 1);

#D = M0(1,1);
#v =  M0(2:end, 1);
#M11 = M0(2:end, 2:end);
#M11 = M11 - v * ( (1/D) * v' );
#full( M11 )

