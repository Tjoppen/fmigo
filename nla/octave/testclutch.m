clear clutch_matrix clutch_matrix.oct ; 

clutch_params = struct( ...
		 "x", [0;0;0;0], ...
		 "v", [8;0;0;4], ...
		 "masses", [5;5;5;5000], ...
		 "k1", 539, ... 
		 "k2", 4.3927e4 , ... 
		 "force", 200, ... 
		 "mu", 1.0, ... 
		 "lo",    [ -0.052359877559829883, -0.087266462599716474], ...
		 "up",   [0.09599310885968812 , 0.17453292519943295 ], ...
		 "torque_in", 100, ... 
		 "torque_out", 0, ... 
		 "step", 1/300, ... 
		 "n_steps", 1, ... 
		 "theta", 1.1, ... 
		 "compliances", 1e-14*[1;1;0] ...
	       );


constraints = [2;3;5;6;8];
T = [0];
X = [clutch_params.x'];
V = [clutch_params.v'];
Z = [ 0* constraints'];
G = [ ];
N = 1600;

for i = 1:N
  [b, q, l, u, z, x, v, g] = clutch_matrix( clutch_params ); 
  X = [X; clutch_params.x'];
  V = [V; clutch_params.v'];
  T = [T; i * clutch_params.step ];
  Z = [Z ;  z( constraints )' / clutch_params.step ];
  G = [G ;  g' ];
  clutch_params.x = x;
  clutch_params.v = v;
endfor
#G = [zeros(size(G, 2)); G];

figure(3)
tt = find( T < 0.2 );
last = tt(end);
plot(T(1:last), V(1:last, :));
title('transient velocities c++')
legend('1','2','3','4');

figure(4)
plot(T, log10( abs( [diff(V, 1, 2), V(:,1)-V(:,end)] ) + eps ) );
title('log10 velocity differences c++')
legend('1','2','3','end to end')

figure(5)
plot(T,  [diff(X, 1, 2), X(:,1)-X(:,end)]   );
title('angle differences')
legend('1-2','2-3','3-4','end to end')

figure(6)
tt = find( T < 0.4 );
last = tt(end);
plot(T(1:last),  [diff(X(1:last,:), 1, 2), X(1:last,1)-X(1:last,end)]   );
title('transient angle differences')
legend('1-2','2-3','3-4','end to end')

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
