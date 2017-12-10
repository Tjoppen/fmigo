%%simple gear 

engaged = 1;  %reduces chatter
alpha  = 1;  %gear ratio
m1 = 1;
m2 = 10;
tend = 30;
h = 1/20;     %time step
N = tend/h;
T =  linspace(0,tend,N)';
ee = 0.0;     %restitution coefficient

fe =   2;     %engine force
fs =  -1;     %shaft torque
%%f = @(t) 2*[ cos(3*t); -cos(2*t) ];
f = @(t) [ fe+0*t;fs+0*t];

fig = 1;
for limit = 1
  X = [0,0];  %positions
  D = [0];    %angle difference
  V = [0,0];  %velocities
  x = [0;0];
  v = [0;0];
  L = [];
  TAU = [];
  dx = 0;
  M = diag([m1,m2]);
  G   = [1, -alpha];
  H   = [M -G';G, 0*1e-1];
  tau =  2;
  gamma = 1/(1+4*tau);
  tol =  1e-4;  %this to avoid chatter and maintain a good
                %contact. if 0, we'll probably have chatter
  %% simplify linear algebra
  schur1 = ( 1/m1 + alpha*alpha/m2 );

  engaged = 1;
  t = 0;
  S = [];
  state = 0;
  u = limit;
  l = -limit;
  for i=2:N
    F =  f(t);
    if ( (l +engaged * tol) < dx && dx < u - ( engaged * tol ) )
      %% solve for free case
      v  =  v + M \ (h*F);
      engaged = 1;
    else
      %% 2 more cases: impact or engaged
      if ( ~ engaged )
        %% impact the thing
        z = H \ [M*v; -ee*G*v];
	v = z(1:2);
	%% we could take a step here, but not needed
	engaged = 1;
	L = [L;z(3)];
	S = [S;2];
      end

      if ( engaged )
	%% we are at bounds or a bit passed, or we just impacted
	cl = dx - l;
	cu = dx  -u;
	% pick the limit we are closest to.
	if( 0 && ( abs(cl) > abs(cu) ) )
	  c = cl;
	else
	  c = cu;
	end
	z = H \ [M*v + h*F ; -4*cl*gamma/h + gamma *G * v];
	v0 = z(1:2);
	%% verify the complementarity conditions
	dx0  = dx + h*G*v0;
	if ( z(3) < 0 &&  (dx0 >= u || dx0 <= l  ) ) 
	  %disp('releasing');
	  v = v + M \ (h*F);
	  engaged = 1;
	else
	  L = [L;z(3)];
	  %disp('coucou')
	  v  = v0;
	  engaged = 1;
	end
      end
    end
    dx = dx + h * G * v;
    x  = x + h * v;
    X = [X;x'];
    V = [V;v'];
    D = [D;dx];
    TAU = [TAU; F(1)-F(2)];
    t = t + h;
    S = [S;engaged];


  end

  figure(fig);  fig = fig+1;
hh = plot( T, V, T, D, T, reshape(f(T), length(T), 2)); 
set(findall(hh, '-property', 'linewidth'), 'linewidth', 2);
legend('w1', 'w2', 'dx', 'f1', 'f2'); 
axis([0, 35, -20, 20])

hh=figure(fig);  fig = fig+1;
plot(T, D,  T, limit+0*T, T, -limit+0*T)
legend('dx')
set(findall(hh, '-property', 'linewidth'), 'linewidth', 2);
title('angle difference')
axis([0, 35, -2,2])

hh=figure(fig);  fig = fig+1;
plot(T,V*G')
title('incident velocity')
set(findall(hh, '-property', 'linewidth'), 'linewidth', 2);
axis([0, 35, -20, 20])
end
