set -e

MD2HDR="`pwd`/../fmu-builder/bin/modeldescription2header"
GENERATOR="`pwd`/../fmu-builder/bin/cmake-generator -t `pwd`/templates/fmi2/ -i `pwd`/../FMILibrary-2.0.1/ThirdParty/FMI/default -m ${MD2HDR}"

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
    gsl2/engine2
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
    loopsolvetest/add
    loopsolvetest/sub
    loopsolvetest/mul
"

cat <<END>CMakeLists.txt
cmake_minimum_required(VERSION 2.8)

if (WIN32)
    link_directories(\${CMAKE_CURRENT_SOURCE_DIR}/wingsl/lib)
    include_directories(\${CMAKE_CURRENT_SOURCE_DIR}/wingsl/include)
    set(CMAKE_SHARED_LINKER_FLAGS "/SAFESEH:NO")
    set(CMAKE_EXE_LINKER_FLAGS "/SAFESEH:NO")
endif ()

END

# Warnings during compilation may go unnoticed without -Werror, leading to
# hard-to-debug problems
export CFLAGS="-Wall -Werror -O3"

#New GSL interface
for d in $GSLFMUS
do
    echo "add_subdirectory($d)" >> CMakeLists.txt
    GSL="-t `pwd`/templates/gsl2/gsl-interface.c -t `pwd`/templates/gsl2/gsl-interface.h -l gsl,gslcblas,m -c"
    pushd $d
        python ${MD2HDR} modelDescription.xml > sources/modelDescription.h
        python ${GENERATOR} ${GSL}
    popd
done

for d in $FMUS
do
    echo "add_subdirectory($d)" >> CMakeLists.txt
    pushd $d
        python ${MD2HDR} modelDescription.xml > sources/modelDescription.h
        python ${GENERATOR}
    popd
done
