# This runs all test scripts we have scattered all over the place
set -e
source boilerplate.sh

(cd ${FMUS_DIR}/me      && ( ./test_me.sh ||  ( echo "failed modelExchange" && exit 1 ) ) )
(cd articles/work-reports       && ( ./run_tests.sh  || ( echo "failed tests in work-reports" && exit 1 ) ) )
(cd articles/truck              && ( python test_gsl_trucks.py  || ( echo "failed loop solver test" && exit 1 ) ) )
(cd fmu-examples/co_simulation  && ( ./run_tests.sh ||  ( echo "failted test in co_simulation" && exit 1 ) ) )
(cd ${FMUS_DIR}/testfmus/typeconvtest    && ( ./test_typeconv.sh ||  ( echo "failed typeconvtest" && exit 1 ) ) )
(cd ${FMUS_DIR}/testfmus/loopsolvetest   && ( ./test_loops.sh ||  ( echo "failed loopsolvetest" && exit 1 ) ) )
(cd ${FMUS_DIR}/testfmus/stringtest      && ( ./test_strings.sh ||  ( echo "failed stringtest" && exit 1 ) ) )
(cd build                       && ( ctest || ( echo "ctest failed" && exit 1 ) ) )

# Check -f none
touch empty_file
mpiexec -np 2 fmigo-mpi -f none ${FMUS_DIR}/gsl2/clutch2/clutch2.fmu | diff empty_file -
rm empty_file
echo Option \"-f none\" works correctly

echo All tests OK
