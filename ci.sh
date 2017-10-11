#!/bin/bash
# Runs gitlab-ci locally
# For some reason there isn't a convenient command for running
# all jobs in the proper order (build -> test), hence this script.
set -e
for t in trusty trusty-cmake3.5.0 xenial
do
  gitlab-ci-multi-runner exec shell build:$t && gitlab-ci-multi-runner exec shell test:$t || break
done
gitlab-ci-multi-runner exec shell build:xenial-fsanitize
