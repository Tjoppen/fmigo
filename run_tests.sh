# This runs all test scripts we have scattered all over the place
set -e

if [ "`uname`" != "Windows_NT" -a "`uname`" != "MINGW64_NT-6.1" ]
then
    # TODO: set up MPI on Windows
    (cd articles/work-reports   && ./run_tests.sh)
fi

(cd fmu-examples/co_simulation  && ./run_tests.sh)
