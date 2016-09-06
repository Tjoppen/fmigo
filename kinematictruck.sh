MASTER=fmi-mpi-master
SERVER=fmi-mpi-server
FMUS_DIR=`pwd`

:<<'END'
mpiexec -np 1 ${MASTER} -t 100 -d 0.01 \
        -C shaft,0,1,0,1,2,3,0,1,2,3 \
        -C shaft,1,2,5,6,7,8,0,1,2,3 \
        -C shaft,2,3,6,7,8,9,0,1,2,3 \
        -c 3,1,0,6 :\
        -np 1 ${SERVER} -5 engine.h5    ${FMUS_DIR}/kinematictruck/engine/engine.fmu :\
        -np 1 ${SERVER} -5 clutch.h5    ${FMUS_DIR}/kinematictruck/clutch/clutch.fmu :\
        -np 1 ${SERVER} -5 gearbox2.h5  ${FMUS_DIR}/kinematictruck/gearbox2/gearbox2.fmu :\
        -np 1 ${SERVER} -5 body.h5      ${FMUS_DIR}/kinematictruck/body/body.fmu  > out.csv
END

## the shaft parameters are as follows (see man page)
## -C shaft,FMU1,FMU2,PARAMS
## PARAMS=shaftAngle0,angularVelocity0,angularAcceleration0,torque0,shaftAngle1,angularVelocity1,angularAcceleration1,torque1

#FIXME: lumpedrod explodes for some reason
:<<'END'
mpiexec -np 1 ${MASTER} -t 100 -d 0.1 \
        -C shaft,0,1,0,1,2,3,0,1,2,3 \
        -C shaft,1,2,5,6,7,8,0,1,2,3 \
        -C shaft,2,3,6,7,8,9,0,2,4,10 \
        -C shaft,3,4,1,3,5,11,0,1,2,3 \
        -c 4,1,0,6 :\
        -np 1 ${SERVER} -5 engine.h5    ${FMUS_DIR}/kinematictruck/engine/engine.fmu :\
        -np 1 ${SERVER} -5 clutch.h5    ${FMUS_DIR}/kinematictruck/clutch/clutch.fmu :\
        -np 1 ${SERVER} -5 gearbox2.h5  ${FMUS_DIR}/kinematictruck/gearbox2/gearbox2.fmu :\
        -np 1 ${SERVER} -5 lumpedrod.h5 ${FMUS_DIR}/lumpedrod/lumpedrod.fmu :\
        -np 1 ${SERVER} -5 body.h5      ${FMUS_DIR}/kinematictruck/body/body.fmu  > out.csv
END

mpiexec -np 1 ${MASTER} -t 100 -d 0.1 \
        -C shaft,0,1,0,1,2,3,0,1,2,3 \
        -C shaft,1,2,5,6,7,8,0,1,2,3 \
        -C shaft,2,3,6,7,8,9,0,2,4,10 \
        -C shaft,3,4,1,3,5,11,0,1,2,3 \
        -c 4,1,0,6 :\
        -np 1 ${SERVER} -5 engine.h5    ${FMUS_DIR}/kinematictruck/engine/engine.fmu :\
        -np 1 ${SERVER} -5 clutch.h5    ${FMUS_DIR}/kinematictruck/clutch/clutch.fmu :\
        -np 1 ${SERVER} -5 gearbox2.h5  ${FMUS_DIR}/kinematictruck/gearbox2/gearbox2.fmu :\
        -np 1 ${SERVER} -5 lumpedrod.h5 ${FMUS_DIR}/lumpedrod/lumpedrod.fmu :\
        -np 1 ${SERVER} -5 body.h5      ${FMUS_DIR}/kinematictruck/body/body.fmu  > out.csv
