
# kinematic coupling of two clutches in a row
mpirun -np 1 fmi-mpi-master -t 15 \
  -p 0,21,10 -p 0,6,1000 -p 0,7,100 -p b,0,8,true -p b,0,17,false:b,1,17,false -p i,0,29,3:i,1,29,3 \
  -C shaft,0,1,34,35,36,26,30,31,32,22 \
  -p b,0,97,true \
  -p i,0,98,2:i,1,98,2 -p 0,13,0:0,15,0:1,13,0:1,15,0 -p 0,28,1:1,28,1  :\
  -np 1 fmi-mpi-server clutch2.fmu :\
  -np 1 fmi-mpi-server clutch2.fmu > out.csv
  
#octave --no-gui --persist --eval "d=load('out.csv'); plot(d(:,1), d(:,[2,6,10,14]));"
octave --no-gui --persist --eval "d=load('clutch2.m'); plot(d(:,1), d(:,2:end));"
