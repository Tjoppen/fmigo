#!/bin/bash
# Runs gitlab-ci locally
# For some reason there isn't a convenient command for running
# all jobs in the proper order (build -> test), hence this script.
set -e
for t in bionic focal jammy buster bullseye bookworm mantic noble
do
  gitlab-ci-multi-runner exec shell build:$t && gitlab-ci-multi-runner exec shell test:$t || (echo "stop @ $t, exit=$?" ; exit 1)
done
