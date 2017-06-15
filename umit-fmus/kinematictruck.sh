FMUS_DIR=`pwd`

:<<'END'
mpiexec -np 5 fmigo-mpi -t 100 -d 0.01 \
        -C shaft,0,1,0,1,2,3,0,1,2,3 \
        -C shaft,1,2,5,6,7,8,0,1,2,3 \
        -C shaft,2,3,6,7,8,9,0,1,2,3 \
        -c 3,1,0,6 \
        ${FMUS_DIR}/kinematictruck/engine/engine.fmu \
        ${FMUS_DIR}/kinematictruck/clutch/clutch.fmu \
        ${FMUS_DIR}/kinematictruck/gearbox2/gearbox2.fmu \
        ${FMUS_DIR}/kinematictruck/body/body.fmu  > out.csv
END

## the shaft parameters are as follows (see man page)
## -C shaft,FMU1,FMU2,PARAMS
## PARAMS=shaftAngle0,angularVelocity0,angularAcceleration0,torque0,shaftAngle1,angularVelocity1,angularAcceleration1,torque1

#FIXME: lumpedrod explodes for some reason
:<<'END'
mpiexec -np 6 fmigo-mpi -t 100 -d 0.1 \
        -C shaft,0,1,0,1,2,3,0,1,2,3 \
        -C shaft,1,2,5,6,7,8,0,1,2,3 \
        -C shaft,2,3,6,7,8,9,0,2,4,10 \
        -C shaft,3,4,1,3,5,11,0,1,2,3 \
        -c 4,1,0,6 \
        ${FMUS_DIR}/kinematictruck/engine/engine.fmu \
        ${FMUS_DIR}/kinematictruck/clutch/clutch.fmu \
        ${FMUS_DIR}/kinematictruck/gearbox2/gearbox2.fmu \
        ${FMUS_DIR}/lumpedrod/lumpedrod.fmu \
        ${FMUS_DIR}/kinematictruck/body/body.fmu  > out.csv
END

:<<'END'
mpiexec -np 6 fmigo-mpi -t 100 -d 0.1 \
        -C shaft,0,1,0,1,2,3,0,1,2,3 \
        -C shaft,1,2,5,6,7,8,0,1,2,3 \
        -C shaft,2,3,6,7,8,9,0,2,4,10 \
        -C shaft,3,4,1,3,5,11,0,1,2,3 \
        -c 4,1,0,6 \
        ${FMUS_DIR}/kinematictruck/engine/engine.fmu \
        ${FMUS_DIR}/kinematictruck/clutch/clutch.fmu \
        ${FMUS_DIR}/kinematictruck/gearbox2/gearbox2.fmu \
        ${FMUS_DIR}/lumpedrod/lumpedrod.fmu \
        ${FMUS_DIR}/kinematictruck/body/body.fmu  > out.csv

END
mpiexec -np 5 fmigo-mpi -t 100 -d 0.1 \
        -C shaft,0,1,theta_out,omega_out,alpha_out,tau_in,x_e,v_e,a_e,force_in_e \
        -C shaft,1,2,x_s,v_s,a_s,force_in_s,x_e,v_e,a_e,force_in_e \
        -C shaft,2,3,x_s,v_s,a_s,force_in_s,phi,omega,alpha,tau_d \
        -c 3,v,0,omega_in \
        ${FMUS_DIR}/gsl2/engine2/engine2.fmu \
        ${FMUS_DIR}/gsl2/clutch2/clutch2.fmu \
        ${FMUS_DIR}/gsl2/clutch2/clutch2.fmu \
        ${FMUS_DIR}/gsl2/trailer/trailer.fmu  > out.csv

:<<'END'
mpiexec -np 5 fmigo-mpi -t 100 -d 0.1 \
        -C shaft,0,1,theta_out,omega_out,alpha_out,tau_in,x_e,v_e,a_e,force_in_e \
        -C shaft,1,2,x_s,v_s,a_s,force_in_s,x_e,v_e,a_e,force_in_e \
        -C shaft,2,3,x_s,v_s,a_s,force_in_s,theta1_e,omega1_alpha1,tau1 \
        -C shaft,3,4,theta2,omega2,a_s,force_in_s,phi,omega,alpha,tau_d \
        -c 4,v,0,omega_in \
        ${FMUS_DIR}/gsl2/engine2/engine2.fmu \
        ${FMUS_DIR}/gsl2/clutch2/clutch2.fmu \
        ${FMUS_DIR}/gsl2/clutch2/clutch2.fmu \
        ${FMUS_DIR}/lumpedrod/lumpedrod.fmu \
        ${FMUS_DIR}/gsl2/trailer/trailer.fmu  > out.csv
END
