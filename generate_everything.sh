set -e

MD2HDR="`pwd`/../fmu-builder/bin/modeldescription2header"
GENERATOR="`pwd`/../fmu-builder/bin/cmake-generator -t `pwd`/templates/fmi2/ -i `pwd`/../FMILibrary-2.0.1/ThirdParty/FMI/default"

GSLFMUS="
    gsl2/clutch2
    gsl2/chained_sho
    gsl2/exp
    gsl2/clutch_ef
    gsl2/clutch
    gsl2/coupled_sho
    gsl2/mass_force
    gsl2/mass_force_fe
    gsl2/trailer
"

FMUS="
    typeconvtest
    impulse
    lumpedrod
    kinematictruck/body
    kinematictruck/engine
    kinematictruck/gearbox2
    kinematictruck/kinclutch
    forcevelocitytruck/fvbody
    forcevelocitytruck/gearbox
"

cat <<END>CMakeLists.txt
cmake_minimum_required(VERSION 2.8)

#Getting GSL to run on Windows is too much of a hassle right now
#TODO: use msys2?
if (NOT WIN32)
END

# Warnings during compilation may go unnoticed without -Werror, leading to
# hard-to-debug problems
export CFLAGS="-Wall -Werror -O3"

#New GSL interface
for d in $GSLFMUS
do
    echo "    add_subdirectory($d)" >> CMakeLists.txt
    GSL="-t `pwd`/templates/gsl2/gsl-interface.c -t `pwd`/templates/gsl2/gsl-interface.h -l gsl,gslcblas,m"
    pushd $d
        python ${MD2HDR} modelDescription.xml > sources/modelDescription.h
        python ${GENERATOR} ${GSL}
    popd
done

echo "endif ()" >> CMakeLists.txt

for d in $FMUS
do
    echo "add_subdirectory($d)" >> CMakeLists.txt
    pushd $d
        python ${MD2HDR} modelDescription.xml > sources/modelDescription.h
        python ${GENERATOR}
    popd
done
