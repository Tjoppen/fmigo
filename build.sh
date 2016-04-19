echo
echo Lazy build script. Requires that FMILibrary-2.0.1 is installed in the system
echo

set -e

FMUBUILDER="`pwd`/../fmu-builder/bin/fmu-builder -t `pwd`/fmi2template/ -i `pwd`/../FMILibrary-2.0.1/ThirdParty/FMI/default"

for d in \
    simpletruck/body\
    simpletruck/engine\
    simpletruck/gearbox2\
    simpletruck/clutch\
    forcevelocitytruck/body\
    forcevelocitytruck/gearbox\
    lumpedrod\
;do
    echo Building $d
    NAME=`sed -e 's/.*\///' <<< $d`
    pushd $d
        rm -f ${NAME}.fmu
        CFLAGS="-Wall -O3" python ${FMUBUILDER}
    popd
done
