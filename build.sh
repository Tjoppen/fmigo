echo
echo Lazy build script. Requires that FMILibrary-2.0.1 is installed in the system
echo

set -e

MD2HDR="`pwd`/../fmu-builder/bin/modeldescription2header"
FMUBUILDER="`pwd`/../fmu-builder/bin/fmu-builder -t `pwd`/templates/fmi2/ -i `pwd`/../FMILibrary-2.0.1/ThirdParty/FMI/default"

#Getting GSL to run on Windows is too much of a hassle right now
#TODO: use msys2?
if [ $WIN = 0 ]
then

    #New GSL interface
    for d in \
        gsl2/exp\
    ;do
        echo Building $d
        GSL="-t `pwd`/templates/gsl2/gsl-interface.c -t `pwd`/templates/gsl2/gsl-interface.h -l gsl,gslcblas,m"
        NAME=`sed -e 's/.*\///' <<< $d`
        pushd $d
            rm -f ${NAME}.fmu
            python ${MD2HDR} modelDescription.xml > sources/modelDescription.h
            CFLAGS="-Wall -O3" python ${FMUBUILDER} ${GSL}
        popd
    done

    # GSL FMUs
    for d in \
        gsl/clutch_ef\
        gsl/clutch\
        gsl/coupled_sho\
        gsl/mass_force\
        gsl/mass_force_fe\
    ;do
        echo Building $d
        GSL="-t `pwd`/templates/gsl/ -l gsl,gslcblas,m"
        NAME=`sed -e 's/.*\///' <<< $d`
        pushd $d
            rm -f ${NAME}.fmu
            python ${MD2HDR} modelDescription.xml > sources/modelDescription.h
            CFLAGS="-Wall -O3" python ${FMUBUILDER} ${GSL}
        popd
    done
fi

for d in \
    impulse\
    lumpedrod\
    kinematictruck/body\
    kinematictruck/engine\
    kinematictruck/gearbox2\
    kinematictruck/clutch\
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
