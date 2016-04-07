n = 10;				# number of particles
h = 1/10;
tau = 2;
c = 1e-6; 
gamma_v = 1 / ( 1 + 4  * tau );
gamma_x = - 4 * gamma_v / h ;
gamma_e = 4 * gamma_v / h / h ;
mass = 2;

diag1 = ones( 2 * n - 1, 1);
diag1(1:2:end) = mass / n ; 
diag1(2:2:end) = -c * gamma_e / ( n - 1 ) ;
sub1 = ones( 2 * n - 2, 1 );
sub1( 2:2:end ) = -1;

M =  ( mass / n )* speye(n);
G = spdiags( [ ones(n-1, 1) , - ones(n-1,1)], [0,1], sparse(n-1, n));

H = [ M,  G'; G, - ( c * gamma_e / n ) * speye( n-1 ) ];

x = zeros(n, 1);
v = zeros(n, 1);
forces = zeros(n,1);
forces( 1 )   = 10; 
forces( end ) = -10; 

X = x';
V = v';
Z = [];
rhs1 = zeros( size(H, 1), 1);

RHS = [];
for i = 2:2
  rhs = [M * v + h * forces; gamma_x * G * x  + gamma_v * G * v ] ;
  rhs1(1:2:end) = rhs(1:n);
  rhs1(2:2:end) = rhs(n+1:end);
  [y, D, S]  = tridiag(diag1, sub1, rhs1);
  RHS = [RHS;rhs'];
  z = H \ rhs;
  v = z(1:n);
  x += h * v;
  X = [X; x'];
  V = [V; v'];
  w = z;
  w(1:2:end) = z(1:n);
  w(2:2:end) = z(n+1, end);
  Z = [Z; w'];
endfor
