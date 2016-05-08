M = diag([1; 3]);
K = 1e3;
force = 100;
G = [1, -1];
gamma  = 1/ (1 + 8);
h = 1 / 100;

ML = M + ( h * h / 4 / gamma ) * G' * K * G ;


N   = 400;
step = 1/ 100; 
x = [0;0];
v = [0;0];

T = [0];
X = [x'];
V = [v'];
force = [100; 0];


for i = 1 : N 

  g = x(1) - x(2);
  v = ML \ ( M * v - h * G' * g  + ( h^2 / 4 ) * G' * K *  G * v   + h * force  );
  x += h * v; 
  T = [T; h * i];
  X = [X; x'];
  V = [V; v'];
endfor

