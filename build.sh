echo
echo Lazy build script. Requires that FMILibrary-2.0.1 is installed in the system
echo

set -e
for d in \
    simpletruck/body\
    simpletruck/engine\
    simpletruck/gearbox2\
    forcevelocitytruck/body\
    forcevelocitytruck/gearbox\
;do
    echo Building $d
    (
    set -e
    NAME=`sed -e 's/.*\///' <<< $d`
    cd $d
    rm -f ${NAME}.fmu
    CFLAGS="-Wall -O3" python ../../../fmu-builder/bin/fmu-builder -t ../../fmi2template/ -i `pwd`/../../../FMILibrary-2.0.1/ThirdParty/FMI/default
    )
done
