# Run in 32-bit MSYS2 console ("MSYS2 MinGW 32-bit")
set -e

mkdir -p /mingw32
pacman -S mingw32/mingw-w64-i686-toolchain
pacman -S mingw32/mingw-w64-i686-gcc        # toolchain might already install this compiler, not sure

#make clean && make distclean
# Using the right compiler here is key
# We're only interested in static libraries
CC=i686-w64-mingw32-gcc ./configure --disable-dynamic --enable-static
make -j
mkdir -p winlibs
# The .a files are legit .lib files, so just rename them
cp .libs/libgsl.a winlibs/gsl.lib && cp cblas/.libs/libgslcblas.a winlibs/gslcblas.lib

# NOTE: Windows does not define hypot() any more, so you need to define your own.
#See: http://stackoverflow.com/questions/6809275/unresolved-external-symbol-hypot-when-using-static-library#10051898

# Below: attempts at 64-bit and dynamic libraries. Not successful
#pacman -S mingw64/mingw-w64-x86_64-toolchain   #64-bit
# Prepend _ to lines not starting with gsl or cblas
#/^gsl\|cblas/!s/^/_/
#buildit() {
#NAME=$1 && \
#    rm -f ${NAME}.lib && \
#    echo EXPORTS > ${NAME}.def && \
#    nm lib${NAME}*.dll | grep ' [DT] [^\\.]' | grep -v security_cookie | sed 's/.* [DT] //;s/^_//;/^gsl\|^cblas\|^Dll/!s/^/_/;s/\(.*\)/    \1 = \1/' >> ${NAME}.def && \
#    egrep "step_alloc|calloc" ${NAME}.def && \
#    dlltool --def ${NAME}.def --dllname lib${NAME}*.dll --output-lib ${NAME}.lib -m i386
#}
#ROOT=`pwd` && (cd .libs/ && buildit gsl && cp *.lib *.dll ${ROOT}/dlls) && (cd cblas/.libs/ && buildit gslcblas && cp *.lib *.dll ${ROOT}/dlls)
