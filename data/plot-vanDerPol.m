y = load("~/work/umit/data/resultFile.mat");
[tt,ss] = makepulses(y(:,1));

figure (1)
subplot(2,1,1)
plot(y(:,1),y(:,2))
subplot(2,1,2);
semilogy(tt,ss)
