##simple gear 

engaged = 0;			#reduces chatter
g  = 20;				#gear ratio
m1 = 1;
m2 = 10;
tend = 30;
h = 1/20;			#time step
N = tend/h;
T =  linspace(0,tend,N)';
l = -2;				#lower limit
u =  2;				#upper limit
ee = 0.0;			#restitution coefficient

fe =   2;			#engine force
fs =  -1;			#shaft torque
f = @(t) [ fe*cos(t/4); fs*cos(t/8) ];
##f = @(t) [ fe;fs];

fig = 1;
for limit = [1,4]
  X = [0,0];				#positions
  D = [0];				#angle difference
  V = [0,0];				#velocities
  x = [0;0];
  v = [0;0];
  dx = 0;
  M = diag([m1,m2]);
  G   = [1, -g];
  H   = [M -G';G, 0*1e-1];
  tau = 2;
  gamma = 1/(1+4*tau/h);
  tol =  1e-4;			#this to avoid chatter and maintain a good
				#contact. if 0, we'll probably have chatter
  ## simplify linear algebra
  schur1 = ( 1/m1 + g*g/m2 );

  t = 0;
  S = [];
  state = 0;
  u = limit;
  l = -limit;
  for i=2:N
    F =  f(t);
    if ( (l +engaged * tol) < dx && dx < u - ( engaged * tol ) )
      ## solve for free case
      w = v +  M \ (h*F);
      ddx = dx + h * (w(1)-w(2));
      ## check to see if we stepped outside
      if ( ddx > u || ddx < l )
	## yes!  impact the thing
	state = 1;
	z = H \ [M*v; -ee*G*v];
	v = z(1:2);
	c = min(u-ddx, ddx-l);
	## continue the step
	z = H \ [M*v + h*F; -4*c*gamma/h  + gamma * G * v];
	v = z(1:2);
	dx += h * G * v;
	engaged = 1;
      else 
	## still free: continue
	engaged = 0;
	state = 0;
	v   = w;
	dx += h *  (v(1)-v(2));
      endif
    else
      state = 2;
      ## gear is active
      z = H \ [M*v + h*F ; -4*dx*gamma/h + gamma *G * v];
      v = z(1:2);
      dx += h * G * v;
    endif
    x += h*v;
    X = [X;x'];
    V = [V;v'];
    D = [D;dx];
    t += h;
    S = [S;state];


  endfor

  figure(fig++); 
  hh = plot( T, V, T, D, T, reshape(f(T), length(T), 2)); 
  set(findall(hh, '-property', 'linewidth'), 'linewidth', 2);
  legend("w1", "w2", "dx", "f1", "f2"); 
  hh = figure(fig++); plot(T, D, ';dx;', T, limit+0*T, T, -limit+0*T)
  set(findall(hh, '-property', 'linewidth'), 'linewidth', 2);
endfor
