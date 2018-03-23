#!/bin/bash
set -e
for d in typeconvtest loopsolvetest stringtest directionaltests multiwaytest
do
  (cd $d && ./run_tests.sh || (echo "failed umit-fmus/tests/$d" && exit 1 ))
done
