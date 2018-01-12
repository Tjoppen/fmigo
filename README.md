<meta http-equiv='Content-Type' content='text/html; charset=utf-8' />

# FmiGo

FmiGo is a backend for the Functional Mockup Interface (FMI) standard.
It connects multiple Functional Mockup Units (FMUs) over ZeroMQ or MPI in a master-server architecture.
There is support for stepping both Model Exchange and Co-Simulation FMUs.
The latter can be connected with typical weak coupling or using an algebraic constraint solver based on Claude Lacoursière's SPOOK solver.

What follows are some brief build and test instructions.
There is also a section on how to set up GitLab CI.

# Pre-requisites

CMake is meta build system required for creating the build system on all platforms.
We support versions down to 2.8.12.2 in GitLab CI, but recommend version 3.5.1 or later.

We recommend using Ninja for building on platforms where it is available,
since it is much faster than msbuild and make (at least when using CMake generated Makefiles).

Python 2.7 or 3.x is required for running the tests.
You will also need to install some Python packages specified in requirements.txt using pip and/or pip3:

```bash
pip install -r requirements.txt
pip3 install -r requirements.txt
```

Finally, bash is also required for running tests.
Any version released in the last ten or so years should work fine.
bash 4.3.11 and 4.3.48 are confirmed working via GitLab CI.
win-bash 0.8.5 is confirmed to *not* work however, due to the lack of subprocesses.
More information about Windows specifics will be given in the Windows section.

# GNU/Linux

Ubuntu and Arch are our primary development platforms, and thus are the most supported.
Ubuntu derivatives such as Lubuntu are also supported.
Debian testing is also likely to work.

After getting the required packages installed, you build as you would any other CMake project.
For example, using Ninja:

```bash
mkdir -p build && cd build && cmake -G Ninja .. && ninja install
```

## Ubuntu / Debian

Inspect the relevant Dockerfile in Dockerfiles/ to find the list of required packages and potential distribution quirks that need attention.

## Arch

Ask Claude ;)

# Windows

There are two major paths to building on Windows: Microsoft Visual Studio 2015 and msys2.

## Microsoft Visual Studio 2015

Install Microsoft Visual Studio 2015 and [MS-MPI](https://msdn.microsoft.com/en-us/library/windows/desktop/bb524831%28v=vs.85%29.aspx).
For MS-MPI you should download and install both msmpisdk.msi and MSMpiSetup.exe.

If you have everything set up correctly then the following commands should work in cmd.exe: cmake, msbuild, mpiexec.
If not then you need to point %PATH% to where these programs are located.

Finally, run the appropriate .bat file for 32-bit or 64-bit builds:

* build\_msvc14.bat for a 32-bit build
* build\_msvc14\_win64.bat for a 64-bit build (can't be tested currently)

The build will take quite a bit of time due to having to build all dependencies.
This may change if Windows ever gets a package manager.
NuGet shows some promise in this direction, but has not been experimented with so far.

Note that tests only work for the 32-bit build due to the lack of a working 64-bit GNU GSL build,
which is required for building our test FMUs.

## msys2

msys2 is a GNU-like system for Windows.
Once set up, it may be easier to keep working than the Visual Studio approach.
It is not yet covered by CI however.

Since msys2 uses pacman, see Arch above for instruction.
GNU GSL may require special attention.
You may also need MS-MPI as with the Visual Studio build.

## Testing

You will need bash and some basic tools like dd, seq and bc.
These ship with msys2, so it's a good idea to always install it,
even if you're doing a Visual Studio build.
These tools also ship with [win-bash](https://sourceforge.net/projects/win-bash/),
however win-bash itself is much too old so care must be taken that msys2's bash is used instead in that case.

Once properly set up, just run `bash run_tests_msvc14.sh` to run tests on the 32-bit build.
As mentioned before, 64-bit tests do not yet work.

# Mac

You have two options: HomeBrew or MacPorts.

## HomeBrew

```
brew install protobuf \
             hdf5 \
             jsoncpp \
             mpich \
             zeromq \
             gsl \
             homebrew/science/suite-sparse
```

## MacPorts

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
