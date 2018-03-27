#  -p 0,26,-200 \


# kinematic coupling of two clutches in a row

set -e

mpirun -np 3 fmigo-mpi -t 10 \
  -p 0,21,10 -p 0,6,100000 -p 0,7,100 -p b,0,8,true \
  -p b,0,97,true \
  -C shaft,0,1,34,35,36,26,0,1,2,3 \
  -p i,0,98,2 -p 0,13,0:0,15,0 -p 0,12,1:0,14,1 -p 0,28,1  \
  -p 1,4,1 \
  clutch.fmu ../../kinematictruck/body/body.fmu > out.csv

octave --no-gui --persist --eval "
d=load('clutch.m');
d2=load('out.csv');
%plot(d(:,1),d(:,2:end));
%axis([0,1,-2,20]);
hold on;
plot(d2(:,1),d2(:,[2,3,6,7]),'k-');
plot(d2(:,1),d2(:,[10,11]),'rx-')

%velocities
%figure(1)
%plot(d2(:,1),d2(:,[3,7,11]))

%angles
%figure(2)
%plot(d2(:,1),d2(:,[3,7,11]-1))
"

exit 0

# kinematic coupling of two clutches in a row
mpirun -np 3 fmigo-mpi -t 150 \
  -p 0,21,10 -p 0,6,1000 -p 0,7,100 -p b,0,8,true -p b,0,17,false:b,1,17,false -p i,0,29,3:i,1,29,3 \
  -C shaft,0,1,34,35,36,26,30,31,32,22 \
  -p b,0,97,true \
  -p 1,14,10000 \
  -p i,0,98,2:i,1,98,2 -p 0,13,1:0,15,1:1,13,1:1,15,1 -p 0,28,1:1,28,1  \
  clutch.fmu clutch.fmu > out.csv
  
octave --no-gui --persist --eval "d=load('out.csv'); plot(d(:,1), d(:,[2,6,10,14]));"
#octave --no-gui --persist --eval "d=load('clutch.m'); plot(d(:,1), d(:,2:end));"
