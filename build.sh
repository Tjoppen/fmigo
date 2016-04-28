echo
echo Lazy build script. Requires that FMILibrary-2.0.1 is installed in the system
echo

set -e

MD2HDR="`pwd`/../fmu-builder/bin/modeldescription2header"
FMUBUILDER="`pwd`/../fmu-builder/bin/fmu-builder -t `pwd`/templates/fmi2/ -i `pwd`/../FMILibrary-2.0.1/ThirdParty/FMI/default"

# GSL FMUs
for d in \
    gsl/coupled_sho\
;do
    echo Building $d
    GSL="-t `pwd`/templates/gsl/"
    pushd $d
        rm -f $d.fmu
        python ${MD2HDR} modelDescription.xml > sources/modelDescription.h
        CFLAGS="-Wall -O3" python ${FMUBUILDER} ${GSL}
    popd
done

for d in \
    simpletruck/body\
    simpletruck/engine\
    simpletruck/gearbox2\
    simpletruck/clutch\
    forcevelocitytruck/body\
    forcevelocitytruck/gearbox\
;do
    echo Building $d
    NAME=`sed -e 's/.*\///' <<< $d`
    pushd $d
        rm -f ${NAME}.fmu
        python ${MD2HDR} modelDescription.xml > sources/modelDescription.h
        CFLAGS="-Wall -O3" python ${FMUBUILDER}
    popd
done
