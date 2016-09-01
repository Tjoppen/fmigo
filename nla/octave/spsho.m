h = 1/100;
N = 10000;
mass = [5;5;5;5000];
G = [1,-1, 0, 0; 0, 1, -1, 0];	#basic spring constraints

K = diag([ 539, 4.3927e4]);

compliances = 1e-8 * [1;1;1;1;1];

x = [0;0; 0;0];
v = [8;0; 0;4];
theta = 2;
gamma = 1/ ( 1 + 4 * theta );
mu = 1;
force = 1e10;
torque_in = 100;
torque_out = 0;
lo = [-0.5, -0.5];
up = [ 0.5,  0.5];

#constraints for the bounds etc. 
J = [
     1  , -1 , 0,  0; ...
    -1  ,  1 , 0,  0; ...
     0  ,  1 ,-1,  0; ...
     0  , -1 , 1,  0; ...
     0  ,  0 , 1, -1;		# velocity constraint
];



ee = ( gamma * 4 / h / h ) * inv( K ) ;


## the big matrix with all redundant equations included.  
MBIG = [ diag( mass ),                       -G',                             -J'; ...
	 G        ,                           ee, zeros( size(G, 1), size(J, 1) ); ...
	 J        ,zeros(size(J,1),  size(G, 1)), gamma*(4/h/h)*diag(compliances); ...
       ];



X = [ x'];
V = [ v'];
Z = [ 0 ];
X0 = [ x'];
V0 = [ v'];

T = h * (0:N-1)';

## silly tortuous redundant way
alpha = 4 * gamma / ( h * h );
ee =  alpha * inv( K ); 

if ( 0 )
iM = inv( [ diag(mass), -G'; G, ee ] );

torque = h * [ torque_in; 0 ; 0; torque_out];
for i = 2:N
  g = [ x(1)-x(2); x(2)-x(3)];
  z   = iM * [ mass.*v + torque; gamma * ( -( 4/h ) * g + G * v ) ];
  v = z(1:4);
  x += h * v ; 
  X = [X; x'];
  V = [V; v'];
  Z = [ Z; z(5:end)];
endfor
endif

x = X(1, :)';
v = V(1, :)';
X1=[x'];
V1=[v'];
Z1=[Z(1,:)];

torque = h * [ torque_in; 0 ; 0; torque_out];
## limits for the multipliers for redu
lower = -inf * ones( size( MBIG, 1 ), 1 ); 
upper =  inf * ones( size( MBIG, 1 ), 1 ); 

start = length( mass ) + size( G, 1);
lower( start + 1 ) = 0;
lower( start + 2 ) = 0;
lower( start + 3 ) = 0;
lower( start + 4 ) = 0;


lower( end ) = -mu * force;
upper( end ) =  mu * force;

if ( 0 )
for i = 2:N
  ## for the springs
  g  = [ x(1)-x(2); x(2)-x(3)];
  ## bounds and stuff
  g1 = [ x( 1 ) - x( 2 ) - lo( 1 ); ...
	 x( 2 ) - x( 1 ) + up( 1 ); ...
	 x( 2 ) - x( 3 ) - lo( 2 );
	 x( 3 ) - x( 2 ) + up( 2 ); 
	v(3)-v(4)];
  q = - [ mass.*v + torque; ...
	  gamma * ( -( 4/h ) * g  + G * v ) 
	  gamma * ( -( 4/h ) * g1 + J * v ) 
	];
  q( end ) = 0;
#  fprintf(stderr(), "step = %d\n", i);
  z   = boxed_keller( MBIG, q, lower, upper);
  v = z(1:4);
  x += h * v ; 
  X1 = [X1; x'];
  V1 = [V1; v'];
  Z1 = [ Z1; z(5:end)];
endfor

endif

## main matrix with no bounds, using mass modification for springs. 
M0 = [ diag(mass) + ( h*h / 4  ) * (1+ 4 * theta )  * G' * K * G  ];
## hard clutch constraint
GG = [0,0,1,-1];
M0 = [M0, -GG'; GG, 0];
iM0 = inv ( M0 );

x = X(1, :)';
v = V(1, :)';

KK = h * h * K / 4;

torque = h * [ torque_in; 0 ; 0; torque_out];
for i = 2:N
  g = [ x(1)-x(2); x(2)-x(3)];
  v   = iM0 * [ mass.*v  + torque - h * G' * K * g ...
		+ ( h * h / 4 ) *  G' * K * G * v; 0];
  v = v(1:4);
  x += h * v; 
  X0 = [X0; x'];
  V0 = [V0; v'];
endfor


## here for the real clutch model 
figure(1)
plot(T, log10( abs( [diff(V0, 1, 2), V0(:,1)-V0(:,end)] ) + eps ) );#, T, X0);
legend('1', '2', '3', 'end to end');
title('log of differences')
figure(2)
tt = find( T < 4 );
last = tt(end);
plot(T(1:last), V0(1:last, :));
legend('1', '2', '3', '4')
title('velocities')
