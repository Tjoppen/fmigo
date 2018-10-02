set -e

#source boilerplate.sh

if false
then
	wget https://github.com/protocolbuffers/protobuf/releases/download/v3.6.1/protobuf-all-3.6.1.tar.gz
fi


if false
then
	pushd cmake
	rm -rf cmake-3.12.2
	tar xfvz cmake-3.12.2.tar.gz
	cd cmake-3.12.2
	./bootstrap --prefix=$PREFIX
	make -j12
	make install
	popd
fi

if false
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

if false
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

if false
then
	#pushd cpython
	#rm -rf cpython-master
	#unzip cpython-master.zip
	#cd cpython-master
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

if false
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
	cmake .. -DCMAKE_CXX_FLAGS="-I$PREFIX/include" -DCMAKE_C_FLAGS="-I$PREFIX/include" -DCMAKE_EXE_LINKER_FLAGS="-L$PREFIX/lib" -DCMAKE_INSTALL_PREFIX=$PREFIX
	make -j12
	make install
	# fmigo-mpi will have references to "impi" in its link command. mpi-speed-test does not.
	# making fmigo-mpi dump traces requires manually editing the impi stuff out, for now
	popd
fi

