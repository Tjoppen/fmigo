n = 15;				# number of particles
h = 1/100;
tau = 0;
c = 1e-1; 
gamma_v = 1 / ( 1 + 4  * tau );
gamma_x = - 4 * gamma_v / h ;
gamma_e = 4 * gamma_v / h / h ;
mass = 5;


dd = zeros(2*n-1, 1);
dd(1:2:end) = mass / n ;
dd(2:2:end) = - c * gamma_e / n;
sb = ones( 2*n-2, 1);
sb(2:2:end) = -1;

tri = spdiags( [[sb;0], dd, [0;sb]], [-1:1], sparse(2*n-1, 2*n-1));

M =  ( mass / n )* speye(n);
G = spdiags( [ ones(n-1, 1) , - ones(n-1,1)], [0,1], sparse(n-1, n));

H = [ M,  G'; G, - ( c * gamma_e / n ) * speye( n-1 ) ];

x = linspace(-1, 1, n)'; 
v = zeros(n, 1);
forces = zeros(n,1);
#forces( 1 )   = 10; 
#forces( end ) = -10; 

X = x';
V = v';
Z = [];
rhs1 = zeros( size(H, 1), 1);

RHS = [];
for i = 2:2000
  rhs = [M * v + h * forces; gamma_x * G * x  + gamma_v * G * v ] ;
  rhs1(1:2:end) = rhs(1:n);
  rhs1(2:2:end) = rhs(n+1:end);
  y   = tri  \  rhs1;
  v1 = y(1:2:end);
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
N = size(X, 1);

samples = min(200, N);
stride = N / samples;
X = X(1:stride:end, :);
xlabel('n')
ylabel('time')
zlabel('x(n)')
