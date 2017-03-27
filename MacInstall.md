# HomeBrew

```
brew install protobuf \
             hdf5 \
             jsoncpp \
             mpich \
             zeromq \
             gsl \
             homebrew/science/suite-sparse
```

# MacPorts

* Does not look for umfpack, needs to be globally available
```
sudo port install SuiteSparse
```
* Does not look for json, requires jsoncpp library
```
sudo port install jsoncpp
```
* Requires mpicc
```
sudo port select --set mpi mpich-mp-fortran
sudo port install mpich
```
