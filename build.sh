echo
echo Lazy build script. Requires that FMILibrary-2.0.1 is installed in the system
echo

set -e
for d in body engine gearbox gearbox2
do
    echo Building $d
    (
    set -e
    cd simpletruck/$d
    rm -f ${d}.fmu
    CFLAGS="-Wall -O3" python ../../fmu-builder/bin/fmu-builder -t ../../fmi2template/
    )
done
