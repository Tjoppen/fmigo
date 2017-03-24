#!/bin/sh

mpirun \
    -np  5 fmigo-mpi -F headers_scania_model.txt -t 3 -d 0.001  \
    -c 0,omega,1,v_in \
    -c 1,force_clutch,0,tau \
    -c 1,v,2,w_shaft_in \
    -c 2,f_shaft_out,1,force_in \
    -c 3,omega,0,omega_l \
    -c 2,w_wheel_out,3,v_in \
    -c 3,f_c,2,f_wheel_in \
    -p 2,gear_ratio,1 \
    -p 2,tq_clutchMax,10 \
    -p 2,tq_brake,1 \
    -p 2,r_tire,0.5 \
    -p 2,m_vehicle,1 \
    -p 2,final_gear_ratio,1 \
    -p 2,J_neutral,1 \
    -p 2,ts,1 \
    -p 2,k1,1 \
    -p 3,r_w,1 \
    -p 3,c_d,0 \
    ./kinematictruck/engine/engine.fmu \
    ./gsl2/clutch/clutch.fmu \
    ./gsl2/scania-driveline/drivetrain_G5IO_m_function_only.fmu \
    ./gsl2/trailer/trailer.fmu #> /tmp/f.csv
