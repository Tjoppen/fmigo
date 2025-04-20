#!/bin/bash
# Runs gitlab-ci locally
# For some reason there isn't a convenient command for running
# all jobs in the proper order (build -> test), hence this script.
WINDOWS_HOSTNAME=desktop
set -e
for t in bionic focal jammy buster bullseye bookworm noble
do
  gitlab-ci-multi-runner exec shell build:$t && gitlab-ci-multi-runner exec shell test:$t || (echo "stop @ $t, exit=$?" ; exit 1)
done

for CMAKE_BUILD_TYPE in Debug Release
do
  # Build with msbuild and cl
  ssh $WINDOWS_HOSTNAME "\
  cd fmigo &&\
  git reset --hard &&\
  rd /s /q build-cl &&\
  mkdir build-cl &&\
  cd build-cl &&\
  cmake .. -DENABLE_SC=OFF -DUSE_GPL=OFF -DBUILD_FMUS=OFF -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} &&\
  cmake --build . --config ${CMAKE_BUILD_TYPE} --target install"

  # Build with ninja and icx
  ssh $WINDOWS_HOSTNAME "\
  cd \"\\Program Files (x86)\\Intel\\oneAPI\" &&\
  setvars &&\
  cd %HOME%\\fmigo &&\
  git reset --hard &&\
  rd /s /q build-icx &&\
  mkdir build-icx &&\
  cd build-icx &&\
  cmake .. -DENABLE_SC=OFF -DUSE_GPL=OFF -DBUILD_FMUS=OFF -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -GNinja &&\
  ninja protobuf-ext &&\
  ninja install"
done
