# To build this you'll want to put this and boilerplate.sh one level above the fmigo directory
# and also download the dependencies that each step uses and put them in appropriate directories.
# The second part hasn't been automated yet.
# An earlier version of this script would download and build cmake, but version 3.5.1 is fine.

set -e

# Separated out so we can bring the required variables
# into the shell when fiddling with the build.
source boilerplate.sh

if true
then
	pushd zmq
	rm -rf zeromq-4.2.3
	tar xfvz zeromq-4.2.3.tar.gz
	cd zeromq-4.2.3
	./configure --prefix=$PREFIX
	make -j12
	make install
	popd
fi

if true
then
	pushd protobuf
	rm -rf protobuf-3.6.1
	tar xfvz protobuf-all-3.6.1.tar.gz
	cd protobuf-3.6.1
	./configure --prefix=$PREFIX
	make -j12
	make install
	popd
fi

if true
then
	pushd python
	rm -rf Python-3.7.0
	tar xfvJ Python-3.7.0.tar.xz
	cd Python-3.7.0
	./configure --prefix=$PREFIX
	make -j12
	make install
	pushd $PREFIX/bin
	ln -s python3 python
	popd
	popd
fi

if true
then
	pushd pip
	python3 get-pip.py --prefix $PREFIX
	popd
fi


if true
then
	pushd fmigo
	pip3 install -r Buildstuff/requirements.txt
	rm -rf build
	mkdir build
	cd build
	cmake .. -DCMAKE_CXX_FLAGS="-I$PREFIX/include" -DCMAKE_C_FLAGS="-I$PREFIX/include" -DCMAKE_EXE_LINKER_FLAGS="-L$PREFIX/lib" -DCMAKE_INSTALL_PREFIX=$PREFIX -DUSE_TRACEANALYZER=ON
	# call cmake a second time to tell it not to add any "impi" stuff to the link commands
	cmake .. -DMPI_CXX_LIBRARIES="" -DMPI_C_LIBRARIES="" -DMPI_CXX_LINK_FLAGS="" -DMPI_C_LINK_FLAGS="" -DMPI_EXTRA_LIBRARY=""
	make -j12
	make install
	popd
fi

