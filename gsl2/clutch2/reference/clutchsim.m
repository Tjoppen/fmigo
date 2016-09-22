k_coupling_1 = 1e5;
d_coupling_1 = 100;
j1_inv = 1;
j2_inv = 1 / 1;
j3_inv = 1 / 1e4;
k_coupling_2 = 1e4;
d_coupling_2 = 1e2;
#j2_inv = 1 / ( 1/j2_inv + 1/ j3_inv );
omega_in = 10;
c_damping = 1;
b = [ -0.087266462599716474; -0.052359877559829883; 0.0; 0.09599310885968812; 0.17453292519943295 ];
c = [ -1000; -30; 0; 50; 3500 ];
K = diff( c ) ./ diff( b );

simclutch = @(t, x ) [ ... 
		       x( 2 );  ... 
		       j1_inv * ( - d_coupling_1 * ( x( 2 ) - omega_in ) - k_coupling_1 *  x( 3 ) ... 
				  - fclutch( x( 1 ) - x( 4 ), x( 2 ) - x( 5 ), c_damping ) ); ...
		       x( 2 ) - omega_in; ...
		       x( 5 ); ...
		       j2_inv * ( + fclutch( x( 1 ) - x( 4 ), x( 2 ) - x( 5 ), c_damping  ) -k_coupling_2 * ( x( 4 ) - x( 6 ) ) ... 
				  - d_coupling_2 * ( x( 5 ) - x( 7 ) )  ); ...
		       x( 7 ); ... 
		       j3_inv * (  k_coupling_2 * ( x( 4 ) - x( 6 ) ) + d_coupling_2 * ( x( 5 ) - x( 7 ) )  );
		     ];

if 0 

A1 =  ...
[ ... 
  0, 1,  0, 0, 0, 0; ...
  -K(1), -d_coupling_1-c_damping, K(1), c_damping, 0, 0; ...
  0, 0, 0, 1, 0, 0; ...
  K(1), c_damping,  -K(1)-k_coupling_2, -c_damping-d_coupling_2, k_coupling_2, d_coupling_2; ...
  0, 0, 0, 0, 0, 1;...
  0, 0, k_coupling_2, d_coupling_2, -k_coupling_2, -d_coupling_2; ...
];
  
A1(2, :) *= j1_inv;
A1(4, :) *= j2_inv;
A1(6, :) *= j3_inv;

A2 = A1;
A2( 2, 1 ) =  -j1_inv * K(2);
A2( 2, 4 ) =   j1_inv * K(2);
A2( 4, 1 ) =   j2_inv *  K(2);
A2( 4, 4 ) =  -j2_inv *  ( K(2) + k_coupling_2 ); 

A3 = A1;
A3( 2, 1 ) =  -j1_inv * K(3);
A3( 2, 4 ) =   j1_inv * K(3);
A3( 4, 1 ) =   j2_inv *  K(3);
A3( 4, 4 ) =  -j2_inv *  ( K(3) + k_coupling_2 ); 

A4 = A1;
A4( 2, 1 ) =  -j1_inv * K(4);
A4( 2, 4 ) =   j1_inv * K(4);
A4( 4, 1 ) =   j2_inv *  K(4);
A4( 4, 4 ) =  -j2_inv *  ( K(4) + k_coupling_2 ); 

M = {A1, A2, A3, A4};
R = {A1, A2, A3, A4};
T = cell();

IA  = eye(size(A1));
h = 1e-3;
E = [];
for i = 1:length(M)
  a0 = h * M{i};
  a = a0;
  r = IA;
  for k=1:5
    r += a / factorial( k );
    a = a * a0;
  endfor
  R{ i } = r;
  E = [E, eigs( r ) ];
#  T{end+1} = M{i} \ ( r - IA );
endfor

else 



simclutch = @(t, x ) [ ... 
		       x( 2 );  ... 
		       j1_inv * ( - d_coupling_1 * ( x( 2 ) - omega_in ) - k_coupling_1 *  x( 3 ) ... 
				  - fclutch( x( 1 ) - x( 4 ), x( 2 ) - x( 5 ), c_damping ) ); ...
		       x( 2 ) - omega_in; ...
		       x( 5 ); ...
		       j2_inv * ( + fclutch( x( 1 ) - x( 4 ), x( 2 ) - x( 5 ), c_damping  ) -k_coupling_2 * ( x( 4 ) - x( 6 ) ) ... 
				  - d_coupling_2 * ( x( 5 ) - x( 7 ) )  ); ...
		       x( 7 ); ... 
		       j3_inv * (  k_coupling_2 * ( x( 4 ) - x( 6 ) ) + d_coupling_2 * ( x( 5 ) - x( 7 ) )  );
		     ];


vopt = odeset ('RelTol', 1e-6, 'AbsTol', 1e-6, 'NormControl', 'on', 'InitialStep', 1e-2, 'MaxStep', 0.1);
sol = ode78( simclutch, [0, 10 ], [ 0;0;0;0;0;0;0], vopt );


figure(1)
H =  plot(sol.x, [sol.y(:,1)-sol.y(:, 4), sol.y(:,2)-sol.y(:,5), sol.y(:, 7)] );
set(findall(H, '-property', 'linewidth'), 'linewidth', 3);
legend("x1-x2", "v1-v2","v3", "location", "northwest")


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

endif
