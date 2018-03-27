#!/bin/bash
# Runs gitlab-ci locally
# For some reason there isn't a convenient command for running
# all jobs in the proper order (build -> test), hence this script.
set -e
for t in xenial trusty trusty-cmake3.5.0
do
  gitlab-ci-multi-runner exec shell build:$t && gitlab-ci-multi-runner exec shell test:$t || exit $?
done
gitlab-ci-multi-runner exec shell build:xenial-fsanitize
