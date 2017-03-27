# This runs all test scripts we have scattered all over the place
set -e
source boilerplate.sh

(cd articles/work-reports       && ( ./run_tests.sh  || ( echo "failed tests in work-reports" && exit 1 ) ) )
(cd articles/truck              && ( python test_gsl_trucks.py  || ( echo "failed loop solver test" && exit 1 ) ) )
(cd fmu-examples/co_simulation  && ( ./run_tests.sh ||  ( echo "failted test in co_simulation" && exit 1 ) ) )
(cd ${FMUS_DIR}/testfmus/typeconvtest    && ( ./test_typeconv.sh ||  ( echo "failed typeconvtest" && exit 1 ) ) )
(cd ${FMUS_DIR}/testfmus/loopsolvetest   && ( ./test_loops.sh ||  ( echo "failed loopsolvetest" && exit 1 ) ) )
(cd ${FMUS_DIR}/testfmus/stringtest      && ( ./test_strings.sh ||  ( echo "failed stringtest" && exit 1 ) ) )
(cd build                       && ( ctest || ( echo "ctest failed" && exit 1 ) ) )

echo All tests OK
