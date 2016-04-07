echo
echo Lazy build script. Requires that FMILibrary-2.0.1 is installed in the system
echo

set -e
for d in  lumpedrod
do
    echo Building $d
    (
    set -e
    NAME=`sed -e 's/.*\///' <<< $d`
    cd $d
    rm -f ${NAME}.fmu
    CFLAGS="-Wall -O3" python2 ../fmu-builder/bin/fmu-builder   -i `pwd`/../../FMILibrary-2.0.1/ThirdParty/FMI/default
    )
done
