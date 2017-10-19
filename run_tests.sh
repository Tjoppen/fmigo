# This runs all test scripts we have scattered all over the place
set -e
source boilerplate.sh

# Grab configuration, for figuring if we have GPL enabled or not
for e in $(fmigo-mpi -e); do export "$e"; done

if [ $USE_GPL -eq 1 ]
then
  (cd ${FMUS_DIR}/me            && ( ./test_me.sh ||  ( echo "failed modelExchange" && exit 1 ) ) )
  (cd articles/truck            && ( python test_gsl_trucks.py  || ( echo "failed GSL truck test" && exit 1 ) ) )
fi
(cd articles/work-reports       && ( ./run_tests.sh  || ( echo "failed tests in work-reports" && exit 1 ) ) )
(cd ${FMUS_DIR}/tests           && ( ./run_tests.sh ||  ( echo "failed umit-fmus tests" && exit 1 ) ) )
(cd ${BUILD_DIR}                && ( ctest || ( echo "ctest failed" && exit 1 ) ) )
(cd ${FMUS_DIR}/meWrapper && pwd &&( ./test_wrapper.sh ||  ( echo "failed wrapper" && exit 1 ) ) )

# Check -f none
touch empty_file
mpiexec -np 2 fmigo-mpi -f none ${FMUS_DIR}/gsl2/clutch2/clutch2.fmu | diff empty_file -
rm empty_file
echo Option \"-f none\" works correctly

# Check that bad parameters give expected failure
# First clutch2 is run with setting VRs which don't exist, but with proper values
# Then it is run with values which are just plain wrong
# The log is grep'd for the expected errors
for t in "r,0,1234,111" "i,0,1234,111" "b,0,1234,true" "s,0,1234,111"
do
  #                                                                          Success = failure
  mpiexec -np 2 fmigo-mpi -p $t ${FMUS_DIR}/gsl2/clutch2/clutch2.fmu &> temp && exit 1         || echo -n
  grep "Couldn't find variable"   temp > /dev/null
  echo Setting incorrect $t fails as expected
done
for t in "0,x0_e,notreal" "0,gear,notint" "0,octave_output,notbool"
do
  #                                                                          Success = failure
  mpiexec -np 2 fmigo-mpi -p $t ${FMUS_DIR}/gsl2/clutch2/clutch2.fmu &> temp && exit 1         || echo -n
  grep "does not look like a"   temp > /dev/null
  echo Setting incorrect $t fails as expected
done
rm temp

# Test wrapper, both Debug and Release
python umit-fmus/wrapper.py -t Debug   umit-fmus/me/bouncingBall/bouncingBall.fmu ${BUILD_DIR}/bouncingBall_wrapped_Debug.fmu
python umit-fmus/wrapper.py -t Release umit-fmus/me/bouncingBall/bouncingBall.fmu ${BUILD_DIR}/bouncingBall_wrapped_Release.fmu

# Test alternative MPI command line
mpiexec -np 1 fmigo-mpi -f none \
  : -np 1 fmigo-mpi ${FMUS_DIR}/gsl2/clutch2/clutch2.fmu
  : -np 1 fmigo-mpi ${FMUS_DIR}/gsl2/clutch2/clutch2.fmu

echo All tests OK
