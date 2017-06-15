k_coupling_1 = 1e8;
d_coupling_1 = 100;
j1_inv = 1;
j2_inv = 1 / 10;
j3_inv = 1 / 1e4;
k_coupling_2 = 1e6;
d_coupling_2 = 1e4;
j2_inv = 1 / ( 1/j2_inv + 1/ j3_inv );
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







mob = [];
dts = [];

dt = 0.1;

for i = 1:8
  
  dt = dt / 2;
  dts=[dts;dt];
  vopt = odeset ('RelTol', 1e-12, 'AbsTol', 1e-12, 'NormControl', 'on', 'InitialStep', 1e-8, 'MaxStep', dt/10);

  fe = 0.1 * eye(3,3);
  fe = [ [0;0;0], fe ];
  x0 = [ 0;0;0;0;0;0;0];
  
  sol = cell();
  for  j=1:4
    
    simclutch = @(t, x ) [ ... 
			   x( 2 );  ... 
			   j1_inv * ( - d_coupling_1 * ( x( 2 ) - omega_in ) - k_coupling_1 *  x( 3 ) ... 
				      - fclutch( x( 1 ) - x( 4 ), x( 2 ) - x( 5 ), c_damping ) + fe(1, j) ); ...
			   x( 2 ) - omega_in; ...
			   x( 5 ); ...
			   j2_inv * ( + fclutch( x( 1 ) - x( 4 ), x( 2 ) - x( 5 ), c_damping  ) -k_coupling_2 * ( x( 4 ) - x( 6 ) ) ... 
				      - d_coupling_2 * ( x( 5 ) - x( 7 ) )   + fe(2, j) ); ...
			   x( 7 ); ... 
			   j3_inv * (  k_coupling_2 * ( x( 4 ) - x( 6 ) ) + d_coupling_2 * ( x( 5 ) - x( 7 ) ) + fe(3, j)  );
			 ];
    
    sol{end+1}= ode78( simclutch, [0, dt ], x0, vopt );
  endfor
  
  M = zeros(3,3);
  for l = 1:3
    M(:, l) = ( sol{l+1}.y(end, [2,5,7]) - sol{1}.y(end, [2,5,7]) ) / dt  / ( norm( fe(:, l+1) ) - norm( fe(:, 1) ) ) ;
  endfor
  mob = [mob; M];
endfor

semilogx( dts, mob);

if ( 0 )

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
endif
