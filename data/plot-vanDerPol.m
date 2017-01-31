y = load("~/work/umit/data/resultFile.mat");
figure (4)
plot(y(:,1),y(:,2))
%axis([2.5586,2.5588,-0.0000000021,0.0000000021]);
pause();

%[tt,ss] = makepulses(y(:,1));
%subplot(2,1,2);
%figure (2)
%plot(y(:,1),y(:,2))
%semilogy(tt,ss)

