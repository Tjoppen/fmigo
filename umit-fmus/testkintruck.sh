FMUS_DIR=`pwd`


mpiexec -np 5 fmigo-mpi -t 100 -d 0.1 \
        -C shaft,0,1,theta_out,omega_out,alpha_out,tau_in,x_e,v_e,a_e,force_in_e \
        -C shaft,1,2,x_s,v_s,a_s,force_in_s,x_e,v_e,a_e,force_in_e \
        -C shaft,2,3,x_s,v_s,a_s,force_in_s,phi,omega,alpha,tau_d \
        -c 3,v,0,omega_in \
        ${FMUS_DIR}/gsl2/engine2/engine2.fmu \
        ${FMUS_DIR}/gsl2/clutch2/clutch2.fmu \
        ${FMUS_DIR}/gsl2/clutch2/clutch2.fmu \
        ${FMUS_DIR}/gsl2/trailer/trailer.fmu  > out.csv


# half of constraint from engine to clutch:
engine=0			# FMU id
clutch=0			# FMU id
-C shaft,$engine,$clutch,phi_engine,w_engine,a_flywheel,tau_c

