<meta http-equiv='Content-Type' content='text/html; charset=utf-8' />

# FmiGo

FmiGo is a backend for the Functional Mockup Interface (FMI) standard.
It connects multiple Functional Mockup Units (FMUs) over ZeroMQ or MPI in a master-server architecture.
There is support for stepping both Model Exchange and Co-Simulation FMUs.
The latter can be connected with typical weak coupling or using an algebraic constraint solver based on Claude Lacoursière's SPOOK solver.

Note that support for this project is very limited until we receive further funding.

What follows are some brief build and test instructions (GNU/Linux, Windows and Mac),
followed by a section on how to set up GitLab CI,
followed by examples.
There is also some stuff from the old README, which we haven't turned into tickets yet.

# Building and test requirements

## Pre-requisites

CMake is a meta build system required for creating the build system on all platforms.
We support versions down to 3.10.2 in GitLab CI.

We recommend using Ninja for building on platforms where it is available,
since it is much faster than `msbuild` and `make` (at least when using CMake generated Makefiles).

Python 3 is required for running the tests.
Python versions down to 3.6.9 are supported.
You will also need to install some Python packages specified in requirements.txt using pip and/or pip3:

```bash
pip3 install -r Buildstuff/requirements.txt
```

bash is required for running tests.
Any version released in the last ten or so years should work fine.
bash 4.4.20 is confirmed working via GitLab CI.
win-bash 0.8.5 is confirmed to *not* work however, due to the lack of subprocesses.
More information about Windows specifics will be given in the Windows section.

## GNU/Linux

Debian and Arch are our primary development platforms, and thus are the most supported.
Ubuntu is tested with Gitlab CI.

After getting the required packages installed, you build as you would any other CMake project.
For example, using Ninja:

```bash
mkdir -p build && cd build && cmake -G Ninja .. && ninja install
```

### Ubuntu / Debian

Inspect the relevant Dockerfile in Buildstuff/ to find the list of required packages and potential distribution quirks that need attention.

### Arch

Ask Claude ;)

### Tools required for testing on GNU/Linux

Make sure bash and coreutils are installed.
This should be the case on any GNU-ish system.

## Windows

There are two major paths to building on Windows: Microsoft Visual Studio 2022 or Intel's C/C++ compiler (icx).

### Microsoft Visual Studio 2022

The 2022 build is somewhat "minimal" compared to the old 2015 build.
It only supports 64-bit builds.
Because of issues with compiling and linking SuiteSparse, strong coupling must be disabled (`-DENABLE_SC=OFF`).
Due to GSL issues all FMUs must also be disabled, as well as loop solving. `-DBUILD_FMUS=OFF -DUSE_GPL=OFF` accomplishes this.
libmatio is a hassle on Windows, so it too must be disabled with `-DUSE_MATIO=OFF`.
In the future some of these features may be brough back up into working condition.

Install either MS-MPI or Intel MPI and set `USE_INTEL_MPI` accordingly.
By default MS-MPI is assumed.

The following command will work in `cmd.exe` with MS-MPI installed:

```
mkdir build
cd build
cmake .. -DENABLE_SC=OFF -DUSE_GPL=OFF -DBUILD_FMUS=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release --target install
```

The build artifacts will be copied to `build/install`.

### Intel icx

Intel's C/C++ compiler icx can be used to compile FMIGo on Windows.
After installing it, you must set up its environment variables using `setvars.bat` in the oneAPI directory:

```
cd "\Program Files (x86)\Intel\oneAPI"
setvars
```

After this, in the same terminal, `cd` back into FMIGo's directory and configure in Release mode with the Ninja generator.
You must build `protobuf-ext` explicitly before running `ninja install`.

```
mkdir build-icx
cd build-icx
cmake .. -DENABLE_SC=OFF -DUSE_GPL=OFF -DBUILD_FMUS=OFF -DCMAKE_BUILD_TYPE=Release -GNinja
ninja protobuf-ext
ninja install
```

As with MSVC, build artifacts will end up in `build/install`.

### Testing

Testing is currently not done on Windows.
Only compilation is done.

## Mac

You have two options: HomeBrew or MacPorts.

### HomeBrew

```
brew install protobuf \
             hdf5 \
             jsoncpp \
             mpich \
             zeromq \
             gsl \
             homebrew/science/suite-sparse
```

### MacPorts

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

# GitLab CI

To set up this project with GitLab CI, docker and gitlab-runner are needed.
In general, follow the instructions in GitLab for help with how to add a new
runner. Perhaps the simplest way is to issue the command

```bash
gitlab-runner register
```

then follow the instruction given on screen.

You can give the CI a test run locally by calling `ci.sh`.
This is a good idea to do prior to pushing anything into GitLab.

Each build will create three images which will use about 1 GiB of disk space
each. Since they can be recreated from .gitlab-ci.yml and the Dockerfiles, it
is recommended to purge these build images from docker periodically to save
disk space. This can be done with a cron job that runs once per week or once
per month.

On a suitably permissioned user create a script docker-cleanup.sh in ~/:

```bash
docker ps --filter "status=exited" --no-trunc -aq       | \
    xargs --no-run-if-empty docker rm
docker images --no-trunc | grep umit | awk '{print $3}' | \
    xargs --no-run-if-empty docker rmi -f
```

Then use crontab -e to add a crontab entry for that user:

```
# Clean docker images on Sunday nights
0   0  *   *   0     /home/<username>/docker-cleanup.sh
```

The downside to this is that the first GitLab CI run on the following week will
take a couple of minutes longer than usual, due to the need to re-do apt-get
updates and installs. If this is too often then scheduling once per month may
be nicer:

```
# Clean docker images at midnight on the first day of every month
0   0  1   *   *     /home/<username>/docker-cleanup.sh
```

# Examples / how to make use of the programs

There are multiple ways of making use of the tools in this repository.

## ssp-launcher.py

*System Structure and Parameterization of Components for Virtual System Design* (SSP)
is a way to parametrize, connect and bundle multiple FMUs in self-contained units called SSPs.
This project includes a script called *ssp-launcher.py* under *tools/ssp*, which can be used to launch SSPs.
We have also extended the standard to support kinematic shaft constraints and to being able to supply the *fmigo* master with extra command-line options.
See *FmiGo.xsd* for more details on these extensions.

The directory also includes some example SSPs to look at,
and the CMake build scripts for them.
The build scripts further depend on the example FMUs which we use
for testing, so it might also be good to take a look at those
(*tests/umit-fmus/kinematictruck* and *tests/umit-fmus/gsl*).

It is possible to use *ssp-launcher.py* in either ZeroMQ or MPI mode.
MPI is used by default.
The -p option is used to switch to TCP aka ZeroMQ mode,
and can be given zero or more explicit ports to use.

## pygo

*pygo* is a set of Python scripts which can be used to abstract
and simplify connecting and running FMUs together.
The abstraction is also useful when one has multiple FMUs that all implement the same model, but with different names for inputs and outputs (as is often the case in larger institutions).
In this way it is quite similar to SSP,
but instead of writing XML you write Python.

## Raw fmigo / scripts

To interact with fmigo directly, reading the manual is highly recommended.
See doc/fmigo.pdf or invoke *man fmigo* after installing.
Many of the scripts under *tests/* make use of this method of calling *fmigo*,
but it is not necessarily the easiest way to do so.

The short story on using *fmigo* directly is that each FMU gets an index starting at zero, based on its position in the master's command line.
The master is either given some filenames to FMUs if using MPI (*fmigo-mpi*) or URLs to FMU servers if using ZeroMQ (*fmigo-master* and *fmigo-server*).
Compare these two:

```bash
$ mpiexec -n 3 fmigo-mpi fmu0.fmu fmu1.fmu
```

versus:

```bash
$ fmigo-server -p 1234 fmu0.fmu & \
  fmigo-server -p 1235 fmu1.fmu & \
  fmigo-master tcp://localhost:1234 tcp://localhost:1235
```

In both cases fmu0.fmu gets index 0 and fmu1.fmu gets index 1.
Note the *-n 3* in the arguments to mpiexec.
The MPI world needs to be one larger than the number of FMUs involved,
since node zero is the master and nodes 1..N are the FMUs.

Connections are made between FMUs based on their indices,
which can be two or more,
plus a number of arguments that depends on the type of connection
being made.
See the manual for the exact syntax involved.

# Old README

Here is the contents of the old README:

Things to do:

* cleanup, code review (Tomas, Jonas, Claude)

* check performance on high performance switches, clusters, etc.
  Abisko for TCP/IP and MPI  (Tomas, when there's free time)

* Master stepper
         
    - support for NEPCE from Benedikt
    - Support for TLM
    - extrapolation + interpolation
    - iterative methods
    - multirate
    - model based extrapolation model reduction and identification (Benedikt)
      inside FMU ModelExchange wrapper
    - Grand mix of model exchange and co-sim: master stepper
      takes change of several ME FMUs as well as cosim ones, and
      time-integrates the ME FMUs together with a sensible
      integrator.  (See Ziegler and Vangheluwe for theory)
    - define kinematic couplings in SSP

* Simulation master

    - weather report
    - database to store and communicate data

             
* publication plan

    - Edo's filter
    - large comparisons of different weak masters
    - kinematic coupling
    - theory of linear stability
