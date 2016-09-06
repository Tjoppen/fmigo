echo
echo Lazy build script. Requires that FMILibrary-2.0.1 is installed in the system
echo

set -e

MD2HDR="`pwd`/../fmu-builder/bin/modeldescription2header"
FMUBUILDER="`pwd`/../fmu-builder/bin/fmu-builder -t `pwd`/templates/fmi2/ -i `pwd`/../FMILibrary-2.0.1/ThirdParty/FMI/default"

for d in \
    lumpedrod\
;do
    echo Building $d
    NAME=`sed -e 's/.*\///' <<< $d`
    pushd $d
        rm -f ${NAME}.fmu
        python2 ${MD2HDR} modelDescription.xml > sources/modelDescription.h
        CFLAGS="-Wall -O3" python2 ${FMUBUILDER}
    popd
done
