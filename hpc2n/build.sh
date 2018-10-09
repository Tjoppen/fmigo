# To build for HPC2N (abisko.hpc2n.umu.se or kebnekaise.hpc2n.umu.se), cd into here and run this script-
# To be able to use the compiled fmigo-mpi you'll anso want to source boilerplate.sh into your shell.
set -e

# Separated out so we can bring the required variables
# into the shell when fiddling with the build.
source boilerplate.sh

cd ..
mkdir -p build
cd build

wget --no-verbose --continue https://github.com/zeromq/libzmq/releases/download/v4.2.3/zeromq-4.2.3.tar.gz
wget --no-verbose --continue https://github.com/protocolbuffers/protobuf/releases/download/v3.6.1/protobuf-all-3.6.1.tar.gz
wget --no-verbose --continue https://bootstrap.pypa.io/get-pip.py
wget --no-verbose --continue https://www.python.org/ftp/python/3.7.0/Python-3.7.0.tar.xz

sha256sum -c <<EOF
8f1e2b2aade4dbfde98d82366d61baef2f62e812530160d2e6d0a5bb24e40bc0  zeromq-4.2.3.tar.gz
fd65488e618032ac924879a3a94fa68550b3b5bcb445b93b7ddf3c925b1a351f  protobuf-all-3.6.1.tar.gz
b89554206d31aeadb8bea05afe53552ed1370ff416b7b49d1abccc0d60b3dca8  get-pip.py
0382996d1ee6aafe59763426cf0139ffebe36984474d0ec4126dd1c40a8b3549  Python-3.7.0.tar.xz
EOF

if true
then
	rm -rf zeromq-4.2.3
	tar xfvz zeromq-4.2.3.tar.gz
	pushd zeromq-4.2.3
	./configure --prefix=$PREFIX
	make -j12
	make install
	popd
fi

if true
then
	rm -rf protobuf-3.6.1
	tar xfvz protobuf-all-3.6.1.tar.gz
	pushd protobuf-3.6.1
	./configure --prefix=$PREFIX
	make -j12
	make install
	popd
fi

if true
then
	rm -rf Python-3.7.0
	tar xfvJ Python-3.7.0.tar.xz
	pushd Python-3.7.0
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
	python3 get-pip.py --prefix $PREFIX
fi


if true
then
	pip3 install -r ../Buildstuff/requirements.txt
	cmake .. -DCMAKE_CXX_FLAGS="-I$PREFIX/include" -DCMAKE_C_FLAGS="-I$PREFIX/include" -DCMAKE_EXE_LINKER_FLAGS="-L$PREFIX/lib" -DCMAKE_INSTALL_PREFIX=$PREFIX -DUSE_TRACEANALYZER=ON
	# call cmake a second time to tell it not to add any "impi" stuff to the link commands
	cmake .. -DMPI_CXX_LIBRARIES="" -DMPI_C_LIBRARIES="" -DMPI_CXX_LINK_FLAGS="" -DMPI_C_LINK_FLAGS="" -DMPI_EXTRA_LIBRARY=""
	make -j12
	make install
fi

