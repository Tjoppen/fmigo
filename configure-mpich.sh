# Script for configuring mpich-3.2 with oversubscribe support (--with-device=ch3:sock)
# Flags extracted by running mpirun --version on the version of mpich that ships with Ubuntu
CPPFLAGS=" -Wdate-time -D_FORTIFY_SOURCE=2 -I/build/mpich-jQtQ8p/mpich-3.2/src/mpl/include -I/build/mpich-jQtQ8p/mpich-3.2/src/mpl/include -I/build/mpich-jQtQ8p/mpich-3.2/src/openpa/src -I/build/mpich-jQtQ8p/mpich-3.2/src/openpa/src -D_REENTRANT -I/build/mpich-jQtQ8p/mpich-3.2/src/mpi/romio/include"
CFLAGS=" -g -O2 -fstack-protector-strong -Wformat -Werror=format-security -O2"
CXXFLAGS=" -g -O2 -fstack-protector-strong -Wformat -Werror=format-security -O2"
FFLAGS=" -g -O2 -fstack-protector-strong -O2"
FCFLAGS=" -g -O2 -fstack-protector-strong -O2 build_alias=x86_64-linux-gnu"
MPICHLIB_CFLAGS="-g -O2 -fstack-protector-strong -Wformat -Werror=format-security"
MPICHLIB_CPPFLAGS="-Wdate-time -D_FORTIFY_SOURCE=2 "
MPICHLIB_CXXFLAGS="-g -O2 -fstack-protector-strong -Wformat -Werror=format-security"
MPICHLIB_FFLAGS="-g -O2 -fstack-protector-strong"
MPICHLIB_FCFLAGS="-g -O2 -fstack-protector-strong"
LDFLAGS="-Wl,-Bsymbolic-functions -Wl,-z,relro"
FC="gfortran"
F77="gfortran"
MPILIBNAME="mpich"
CC="gcc"
LIBS="-lpthread"

./configure --with-device=ch3:sock --disable-option-checking --prefix=/usr --build=x86_64-linux-gnu --includedir=/include --mandir=/share/man --infodir=/share/info --sysconfdir=/etc --localstatedir=/var --disable-silent-rules --libdir=/lib/x86_64-linux-gnu --libexecdir=/lib/x86_64-linux-gnu --disable-maintainer-mode --disable-dependency-tracking --enable-shared --enable-fortran=all --disable-rpath --disable-wrapper-rpath --sysconfdir=/etc/mpich --libdir=/usr/lib/x86_64-linux-gnu --includedir=/usr/include/mpich --docdir=/usr/share/doc/mpich --with-hwloc-prefix=system --enable-checkpointing --with-hydra-ckpointlib=blcr --cache-file=/dev/null --srcdir=. 

