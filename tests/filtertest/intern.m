x = load('clutch2.m'); 
t = x(:, 1); 
x = x(:, 2:end) ; 
plot(t, x(:, 2))
t = [0;t];
x = [0*x(1,:);x];

H = 0.001;			#communication step
kc = 1e9;
dc = 44271.887242357305;

## we are mostly interested in first mass.
ts = linspace(0, t(end), t(end)/H)';
xs = interp1(t, x, ts);
