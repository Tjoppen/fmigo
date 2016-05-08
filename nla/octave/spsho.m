h = 1/100;
N = 400;
mass = [1;1;1;1];
G = [1,-1, 0, 0; 0, 1, -1, 0];	#basic spring constraints
K = diag([1,1]);
compliances = 1e-8 * [1;1;1;1;1];
x = [1;0; 0;0];
v = [0;0; 0;0];
theta = 2;
gamma = 1/ ( 1 + 4 * theta );
mu = 1;
force = 1e10;
torque_in = 1;
torque_out = 0;
lo = [-0.5, -0.5];
up = [ 0.5,  0.5];

#constraints for the bounds etc. 
J = [
     1  , -1 , 0,  0; ...
    -1  ,  1 , 0,  0; ...
     0  ,  1 ,-1,  0; ...
     0  , -1 , 1,  0; ...
     0  ,  0 , 1, -1;
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
  fprintf(stderr(), "step = %d\n", i);
  z   = boxed_keller( MBIG, q, lower, upper);
  v = z(1:4);
  x += h * v ; 
  X1 = [X1; x'];
  V1 = [V1; v'];
  Z1 = [ Z1; z(5:end)];
endfor


M0 = [ diag(mass) + (1/ alpha )  * G' * K * G  ];
iM0 = inv ( M0 );

x = X(1, :)';
v = V(1, :)';

KK = h * h * K / 4;

torque = h * [ torque_in; 0 ; 0; torque_out];
for i = 2:N
  g = [ x(1)-x(2); x(2)-x(3)];
  v   = iM0 * [ mass.*v  + torque - h * G'  * g  + G' * KK * G * v];
  x += h * v ; 
  X0 = [X0; x'];
  V0 = [V0; v'];
endfor


## here for the real clutch model 
figure(1)
plot(T, X);#, T, X0);
legend('one', 'two', 'three', 'four')
title('Spook')
