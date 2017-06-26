# This runs all test scripts we have scattered all over the place
set -e
source boilerplate.sh

(cd ${FMUS_DIR}/me      && ( ./test_me.sh ||  ( echo "failed modelExchange" && exit 1 ) ) )
(cd articles/work-reports       && ( ./run_tests.sh  || ( echo "failed tests in work-reports" && exit 1 ) ) )
(cd articles/truck              && ( python test_gsl_trucks.py  || ( echo "failed GSL truck test" && exit 1 ) ) )
(cd ${FMUS_DIR}/tests           && ( ./run_tests.sh ||  ( echo "failed umit-fmus tests" && exit 1 ) ) )
(cd build                       && ( ctest || ( echo "ctest failed" && exit 1 ) ) )
(cd ${FMUS_DIR}/meWrapper && pwd &&( ./test_wrapper.sh ||  ( echo "failed wrapper" && exit 1 ) ) )

# Check -f none
touch empty_file
mpiexec -np 2 fmigo-mpi -f none ${FMUS_DIR}/gsl2/clutch2/clutch2.fmu | diff empty_file -
rm empty_file
echo Option \"-f none\" works correctly

# Check that bad parameters give expected failure
# First clutch2 is run with bad parameters of all types
# Then the log is grep'd for an error related to this
for t in r i b s
do
  #                                                                                     Success = failure
  mpiexec -np 2 fmigo-mpi -p $t,0,1234,111 ${FMUS_DIR}/gsl2/clutch2/clutch2.fmu &> temp && exit 1         || echo -n
  grep "Couldn't find variable"   temp > /dev/null
  echo Setting incorrect $t fails as expected
done
rm temp

echo All tests OK
