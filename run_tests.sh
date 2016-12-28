# This runs all test scripts we have scattered all over the place
set -e
source boilerplate.sh

(cd articles/work-reports       && ( ./run_tests.sh  || ( echo "failed tests in work-reports" && exit -1 ) ) )
(cd articles/truck              && ( python run_gsl_trucks.py --test  || ( echo "failed loop solver test" && exit -1 ) ) )
(cd fmu-examples/co_simulation  && ( ./run_tests.sh ||  ( echo "failted test in co_simulation" && exit -1 ) ) )
(cd ${FMUS_DIR}/typeconvtest    && ( ./test_typeconv.sh ||  ( echo "failed tests in typeconvtest" && exit -1 ) ) )
(cd ${FMUS_DIR}/loopsolvetest   && ( ./test_loops.sh ||  ( echo "failed tests in loopsolvetest" && exit -1 ) ) )
echo All tests OK
