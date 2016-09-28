# Run all Polybench tests
# Invoke as 
#    sh run-all-tests.sh ${LLVM_BUILD_DIR}
#

# SET from external variables
LLVM_BUILD_DIR=$1
POLYBENCH_DIR=$(dirname "$0")/polybench-c-4.1

# Loop over all tests
TESTS=$(find ${POLYBENCH_DIR}/datamining -name "*.c")
TESTS="$TESTS $(find ${POLYBENCH_DIR}/linear-algebra -name "*.c")"
TESTS="$TESTS $(find ${POLYBENCH_DIR}/stencils -name "*.c")"
TESTS="$TESTS $(find ${POLYBENCH_DIR}/medley -name "*.c")"

for f in ${TESTS}; do 
     echo ">> Testing $f"
     sh run-polybench-test.sh ${LLVM_BUILD_DIR} $f
done
