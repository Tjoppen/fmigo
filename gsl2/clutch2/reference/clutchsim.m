k_coupling_1 = 1e5;
d_coupling_1 = 100;
j1_inv = 1;
j2_inv = 1 / 10;
j3_inv = 1 / 1e4;
k_coupling_2 = 1e6;
d_coupling_2 = 1e4;
j2_inv = 1 / ( 1/j2_inv + 1/ j3_inv );
omega_in = 10;
c_damping = 1;


simclutch = @(t, x ) [ ... 
		       x( 2 );  ... 
		       j1_inv * ( - d_coupling_1 * ( x( 2 ) - omega_in ) - k_coupling_1 *  x( 3 ) ... 
				  - fclutch( x( 1 ) - x( 4 ), x( 2 ) - x( 5 ), c_damping ) ); ...
		       x( 2 ) - omega_in; ...
		       x( 5 ); ...
		       j2_inv * ( + fclutch( x( 1 ) - x( 4 ), x( 2 ) - x( 5 ), c_damping  ) -k_coupling_2 * ( x( 4 ) - x( 6 ) ) ... 
				  - d_coupling_2 * ( x( 5 ) - x( 7 ) ) ); ...
		       x( 7 ); ... 
		       j3_inv * (  k_coupling_2 * ( x( 4 ) - x( 6 ) ) + d_coupling_2 * ( x( 5 ) - x( 7 ) ) );
		     ];


vopt = odeset ('RelTol', 1e-9, 'AbsTol', 1e-9, 'NormControl', 'on', 'InitialStep', 1e-2, 'MaxStep', 0.1);

sol = ode78( simclutch, [0, 10 ], [ 0;0;0;0;0;0;0], vopt );


figure(1)
H =  plot(sol.x, sol.y);
set(findall(H, '-property', 'linewidth'), 'linewidth', 3);
legend("x1", "v1", "dx", "x2", "v2", "x3", "v3")


figure(2);
H = plot(sol.y(:, 1) - sol.y(:, 4 ), sol.y(:, 5), '.-')
set(findall(H, '-property', 'linewidth'), 'linewidth', 3);
title("phase plot: angle difference  vs speed")
xlabel("dx");
ylabel("v2");

figure(3);
H = plot(sol.x, sol.y(:, 1) - sol.y(:, 4 ) );
set(findall(H, '-property', 'linewidth'), 'linewidth', 3);
title("Angle differences")
