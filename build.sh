echo
echo Lazy build script. Requires that FMILibrary-2.0.1 is installed in the system
echo

set -e

MD2HDR="`pwd`/../fmu-builder/bin/modeldescription2header"
FMUBUILDER="`pwd`/../fmu-builder/bin/fmu-builder -t `pwd`/templates/fmi2/ -i `pwd`/../FMILibrary-2.0.1/ThirdParty/FMI/default"

#Getting GSL to run on Windows is too much of a hassle right now
#TODO: use msys2?
if [[ $WIN -ne 1 ]]
then
    # Warnings during compilation may go unnoticed without -Werror, leading to
    # hard-to-debug problems
    export CFLAGS="-Wall -Werror -O3"

    #New GSL interface
    for d in \
        gsl2/chained_sho\
        gsl2/exp\
        gsl2/clutch_ef\
        gsl2/clutch\
        gsl2/coupled_sho\
        gsl2/mass_force\
        gsl2/mass_force_fe\
    ;do
        echo Building $d
        GSL="-t `pwd`/templates/gsl2/gsl-interface.c -t `pwd`/templates/gsl2/gsl-interface.h -l gsl,gslcblas,m"
        NAME=`sed -e 's/.*\///' <<< $d`
        pushd $d
            rm -f ${NAME}.fmu
            python ${MD2HDR} modelDescription.xml > sources/modelDescription.h
            python ${FMUBUILDER} ${GSL}
        popd
    done
fi

for d in \
    typeconvtest\
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
        python ${FMUBUILDER}
    popd
done
