X = load('data.dat');

x = X(:, 2:5);
x1 = X(:, 2);
x2 = X(:, 3);
x3 = X(:, 4);
x4 = X(:, 5);
states = X(:, 10:14 );
lambda = X(:, 15:19 );
g = X(:, 20:24 );
impact = X(:, 25 );
tau = X(:, end);
dphi = x1 - x4;
dphi1 = x1- x2;
dphi2 = x2 - x3;
dphi3 = x3 - x4;
v = X(:, 6:9);
v1 = v(:, 1);
v2 = v(:, 2);
v3 = v(:, 3);
v4 = v(:, 4);
dv = v1 - v4;
dv1 = v1-v2;
dv2  = v2-v3;
dv3 = v3-v4;
t = X(:, 1);

figure(1);
plot(t, dphi, t, dphi1, t, dphi2);
legend('end to end', 'first' , 'second')
title('angular differences')
figure(2);
plot(t, v);
legend('1', '2', '3', '4');
title('speeds')
figure(3)
plot(log10(abs([diff(v,1,2), v(:,1)-v(:,end) ] ) + eps ));
