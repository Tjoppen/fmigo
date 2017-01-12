k1 = 539;				# first spring constant
k2 = 43927;			# second spring constant
c1 =  0*1e-3;			# compliance up/lo limit 1
c2 =  0*1e-3;			# compliance up/lo limit 2
c3 =  0*1e-3;			# plate slip
masses =  [5;5;5;5000];		# obvious
torque_in = 100;		# obvious
torque_out = 0;			# obvious
force = 100;			# force on clutch plates
friction = 1;			# friction coeff between plates
step    = 1/20;
tau = 2 * step ;
lo_range = [-0.5; -1.0];	# ranges for the springs
up_range = [ 0.5;  1.0];	


gamma = 1 / ( 1 + 4 * tau / step );
etilde1 = 4 * c1

kk1 = gamma * 4 * k1 / step / step ; 
kk2 = gamma * 4 * k2 / step / step ; 



D = [ masses( 1 ) + kk1; -c1; -c1; masses( 2 ) + kk1 + kk2; ... 
      -c2; -c2; masses( 3 ) +  kk2; -c3; masses( 4 ) ];


S1 = [  1.0;  0.0;   1.0;  1.0;  0.0;  1.0; 1.0 ; 1.0 ];

S2 = [ -1.0; -1.0;   0.0; -1.0; -1.0;  0.0; 0.0 ] ;

S3 = [  kk1;  0.0;   0.0;   kk2; 0.0;  0.0;  ];


M = spdiags( [ [S3; 0; 0; 0], [S2; 0; 0], [ S1;  0], D, [ 0; S1 ], [0; 0; S2] , [0;0;0;S3] ], ...
	      [ -3:3], sparse( length(D), length( D ) ) );
 
signs = [ 1; -1; -1; 1; -1; -1; 1; -1; 1 ] ;
MM = [D, [S1;0], [S2;0;0], [S3;0;0;0]];
active = ones(length(D), 1);
bandmatrix = struct("matrix", MM, "signs", signs, "active", active);
MB = M * diag( signs );


## v = [0;0;0;0];
## x = [0;0;0;0];


##     double KK1X = step * k1;
##     double KK1V = step * step * k1 / Real(4.0); 
##     double KK2X = step * k2;
##     double KK2V = step * step * k2 / Real(4.0); 

##     double dx1 = x[0] - x[1];
##     double dv1 = v[0] - v[1];
##     double dx2 = x[1] - x[2];
##     double dv2 = v[1] - v[2];
    
##      ** first mass
##     rhs[ 0 ] = - ( mass[ 0 ] * v[ 0 ]  - KK1X * dx1 - KK1V * dv1  + step * torque_in );

## /// first lower bound distance
##     rhs[ 1 ] = - gamma * ( -( 4 / step ) *  ( dx1     - lo[ 0 ] )  + dv1 );
## /// first upper bound distance
##     rhs[ 2 ] = - gamma * ( -( 4 / step ) *  ( up[ 0 ] - dx1     )  - dv1 );
    
##     rhs[ 3 ] = - ( mass[ 1 ] * v[ 1 ] + KK1X * dx1 - KK2X * dx2 + KK1V * dv1 - KK2V * dv2 );
    
##     rhs[ 4 ] = - gamma * ( -( 4 / step ) *  (  dx2    - lo[ 1 ] ) + dv2 ); 
## /// second upper bound distance
##     rhs[ 5 ] = - gamma * ( -( 4 / step ) *  ( up[ 1 ] - dx2     ) - dv2 );

 
## /// third mass
##     rhs[ 6 ] = - ( mass[ 2 ] * v[ 2 ] + KK1X * dx2 + KK1V * dv2 );

##     rhs[ 7 ] = 0;
    
##     rhs[ 8 ] = - ( mass[ 3 ] * v[ 3 ] + step * torque_out );
    
##     return;
    
##   }
