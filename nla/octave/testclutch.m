clear clutch_matrix clutch_matrix.oct ; 

clutch_params = struct( ...
		 "x", [1;0;0;0], ...
		 "v", [0;0;0;0], ...
		 "masses", [1;1;1;1], ...
		 "k1", 1, ... 
		 "k2", 1 , ... 
		 "force", 100000.1, ... 
		 "mu", 1.0, ... 
		 "lo", [- 0.6, -0.5], ...
		 "up", [  0.5,  0.5], ...
		 "torque_in", 1, ... 
		 "torque_out", 0, ... 
		 "step", 1/100, ... 
		 "n_steps", 1, ... 
		 "theta", 2.0, ... 
		 "compliances", [1e-10;1e-10;1e-10;1e-10;1e-10] ...
	       );

constraints = [2;3;5;6;8];
T = [0];
X = [clutch_params.x'];
V = [clutch_params.v'];
Z = [ 0* constraints'];
G = [ 0* constraints'];
N = 400;

for i = 1:N
  [b, q, l, u, z, x, v, g] = clutch_matrix( clutch_params ); 
  X = [X; clutch_params.x'];
  V = [V; clutch_params.v'];
  T = [T; i * clutch_params.step ];
  Z = [Z ;  z( constraints )' / clutch_params.step ];
  G = [G ;  g' ];
  clutch_params.x = x;
  clutch_params.v = v;
#  clutch_params.torque_in = torque  *  sin ( 3 * T(end));
#  clutch_params.torque_out = - torque  *  cos ( 3 * T(end));
endfor

[dphi, ix ]  = sort ( X(:, 1) - X(:, end) ) ; 
tau = Z(ix, end);

[dphi1, ix] = sort( X(1:end-1, 1) - X(1:end-1, 2) );
tau1  =   (diff( V(:, 1) ) / clutch_params.masses( 1 ) / clutch_params.step )(ix); 
[dphi2, ix] = sort( X(1:end-1, 2) - X(1:end-1, 3) );
tau2  =   (diff( V(:, 2) ) / clutch_params.masses( 2 ) / clutch_params.step )(ix); 
[dphi3, ix] = sort( X(1:end-1, 3) - X(1:end-1, 4) );
tau3  =   (diff( V(:, 3) ) / clutch_params.masses( 3 ) / clutch_params.step )(ix); 

if (  0 )
m = rebuildband( b ) ; 
z1 = boxed_keller(m, q, l, u);  norm(z-z1)
endif
