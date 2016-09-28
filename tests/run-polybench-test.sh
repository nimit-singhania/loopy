# Test script for Polybench benchmarks
#
# Invoke as 
#   sh ./run-polybench-test.sh ${LLVM_BUILD_DIR} <file>

## SET LLVM build directory
llvm_build=$1

## SET testfile
testfile=$(dirname $2)/$(basename -s .c $2)

## SET testdir
testdir=$(dirname $2)

## Polybench home directory
polybench=$(dirname "$0")/polybench-c-4.1


# =============================================================================
## Flags 
# Dump outputs from benchmarks?
dump= #-DPOLYBENCH_DUMP_ARRAYS

# Time the kernel execution?
time=-DPOLYBENCH_TIME

# Debug Output from Loopy
debug_loopy= #-debug-only=polly-pwaff

# Debug Output for target AST
debug_ast= #-debug-only=polly-ast


## Compile bechmarks
# LLVM IR
$llvm_build/bin/clang -I $polybench/utilities -S -emit-llvm $dump $time $testfile.c -o $testfile.s
alias opt="$llvm_build/bin/opt -load $llvm_build/lib/LLVMPolly.so"
opt -S -polly-canonicalize $testfile.s > $testfile.preopt.ll

# Loopy Executable
echo "Compiling Loopy version ..."
opt $debug_loopy $debug_ast -polly-pwaff -polly-codegen -polly-trans=$testdir/opt.t  -S $testfile.preopt.ll | opt -O3 > $testfile.loopy.ll 
$llvm_build/bin/llc $testfile.loopy.ll -o $testfile.loopy.s
gcc -I $polybench/utilities $testfile.loopy.s $polybench/utilities/polybench.c $dump $time -o $testfile.loopy.exe -lm

# Polly Executable
echo "Compiling Polly version ..."
opt -O3 $debug_ast -polly $testfile.preopt.ll > $testfile.polly.ll
$llvm_build/bin/llc $testfile.polly.ll -o $testfile.polly.s
gcc -I $polybench/utilities $testfile.polly.s $polybench/utilities/polybench.c $time $dump -o $testfile.polly.exe -lm

# LLVM Executable
echo "Compiling LLVM version ..."
$llvm_build/bin/clang -O3 -I $polybench/utilities $polybench/utilities/polybench.c $dump $time $testfile.c -o $testfile.llvm.exe -lm

## ICC Executable
# echo "Compiling ICC version ..."
# icc -O3 -I $polybench/utilities $polybench/utilities/polybench.c $dump $time $testfile.c -o $testfile.icc.exe 


## Execute optimized benchmarks
ltime=`$testfile.loopy.exe 2>$testfile.loopy.log`
#itime=`$testfile.icc.exe 2>$testfile.icc.log`
ptime=`$testfile.polly.exe 2>$testfile.polly.log`
lltime=`$testfile.llvm.exe 2>$testfile.llvm.log`


## Output execution times
echo
echo "---------------------"
echo "Execution time"
echo "---------------------"
echo "Loopy: $ltime s"
echo "Polly: $ptime s"
#echo "\nICC: $itime"
echo "LLVM:  $lltime s"
echo "---------------------"
echo
