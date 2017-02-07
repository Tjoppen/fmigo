# Build and store this as umit/ubuntu:1 on each build machine, like so:
#
#  docker build -t umit/ubuntu:1 .
#
FROM ubuntu:xenial
RUN apt-get update
#
# Some notes on the packages installed here where it may not be obvious why
# they are installed:
#
#  build-essential: for gcc
#              git: for being able to fetch submodules, since gitlab doesn't
#                   do this for us yet
#           psmisc: for killall
#
RUN apt-get install --yes \
        build-essential cmake ninja-build git python bc psmisc \
        protobuf-compiler protobuf-c-compiler libprotobuf-dev \
        libsuitesparse-dev libzmqpp-dev libhdf5-dev \
        libopenmpi-dev libgsl-dev
#
# Create user, since mpiexec doesn't like to run as root
#
RUN useradd -ms /bin/bash gitlab-ci
USER gitlab-ci
WORKDIR /home/gitlab-ci
#
# Generate private/public key pair for SSH access, for being able to clone the submodules
# used in this project via a guest account on gitlab
#
RUN ssh-keygen -f ~/.ssh/id_rsa -P ""
RUN cat ~/.ssh/id_rsa.pub

