# This runs all test scripts we have scattered all over the place
set -e

(cd articles/work-reports       && ./run_tests.sh)
(cd fmu-examples/co_simulation  && ./run_tests.sh)
