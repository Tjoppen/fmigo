ALPHA=10
mpiexec -np 3 fmigo-mpi -D -t 30 -p 0,j1,1 -p 0,j2,10 -p 0,alpha,$ALPHA -p 0,tau_e,1 -p 0,gap,1 -C shaft,0,1,theta_l,omega_l,omegadot_l,tau_l,theta,omega,alpha,tau \
 gearbox_gap.fmu \
 ../body/body.fmu > out.csv
# Scale engine speed down by gear ratio so we can plot all three velocities on top of eachother
octave --no-gui --quiet --persist --eval "d=csvread('out.csv'); plot(d(:,1),d(:,[3,6,9])*diag([1.0/$ALPHA,1,1])); legend('engine','middle','load','location','eastoutside')"

