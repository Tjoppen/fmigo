#!/bin/bash
set -e

MD2HDR="`pwd`/../fmu-builder/bin/modeldescription2header"
FMUBUILDER="`pwd`/../fmu-builder/bin/fmu-builder -t `pwd`/templates/fmi2/ -i `pwd`/../FMILibrary-2.0.1/ThirdParty/FMI/default"

# GSL FMUs
for d in \
    gsl2/trailer\
;do
    echo Building $d
    GSL="-t `pwd`/templates/gsl2/ -l gsl,gslcblas,m"
    NAME=`sed -e 's/.*\///' <<< $d`
    pushd $d
        rm -f ${NAME}.fmu
        python2 ${MD2HDR} modelDescription.xml > sources/modelDescription.h
        CFLAGS="-Wall -O3" python2 ${FMUBUILDER} ${GSL}
    popd
done
