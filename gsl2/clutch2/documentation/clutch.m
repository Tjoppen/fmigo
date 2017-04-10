
b = [ -0.087266462599716474*1.5; -0.052359877559829883; 0.0; 0.09599310885968812; 0.17453292519943295 ];
c = [ -2000*1.5; -30; 0; 50; 3500 ];

x = linspace(b(1), b(end), 200)';

y = interp1(b, c, x);

H = plot(x, y,'r');
set(findall(H, '-property', 'linewidth'), 'linewidth', 2);
title('Piecewise linear clutch')
xlabel('$\delta\phi$')
ylabel('$\tau$')

z = [x,y]
save "-ascii" "clutch.dat" z;

#print( 'clutch.tikz', '-dtikz', '-F:10', '-r100');
