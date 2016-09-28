# Test script for Polybench benchmarks
#
# Invoke as 
#   sh ./run-loopy.sh ${LLVM_BUILD_DIR} <progfile> <optfile>
#   ex: sh run-loopy.sh ../llvm/build matmul.c  matmul.t

if [ "$1" == "" ] || [ "$2" == "" ] || [ "$3" == "" ]; then
    echo "usage: sh ./run-loopy.sh <llvm_build_dir> <program_file> <transform_script>";
    exit;
fi


## SET LLVM build directory
llvm_build=$1

## SET progfile
progfile=$(dirname $2)/$(basename -s .c $2)

## SET optfile 
optfile=$3

# Debug Output from Loopy
debug_loopy= #-debug-only=polly-pwaff

# Debug Output for resultant Abstract Syntax Tree
debug_ast= #-debug-only=polly-ast

# =============================================================================


# LLVM IR
$llvm_build/bin/clang -S -emit-llvm $progfile.c -o $progfile.s
alias opt="$llvm_build/bin/opt -load $llvm_build/lib/LLVMPolly.so"
opt -S -polly-canonicalize $progfile.s > $progfile.preopt.ll

# Loopy Executable
echo "Compiling Loopy version ..."
opt $debug_loopy $debug_ast -polly-pwaff -polly-codegen -polly-trans=$optfile -S $progfile.preopt.ll | opt -O3 > $progfile.loopy.ll 
$llvm_build/bin/llc $progfile.loopy.ll -o $progfile.loopy.s
gcc $progfile.loopy.s -o $progfile.loopy.exe 

# Polly Executable
echo "Compiling Polly version ..."
opt -O3 -polly $progfile.preopt.ll > $progfile.polly.ll
$llvm_build/bin/llc $progfile.polly.ll -o $progfile.polly.s
gcc $progfile.polly.s -o $progfile.polly.exe

# LLVM Executable
echo "Compiling LLVM version ..."
$llvm_build/bin/clang -O3 $progfile.c -o $progfile.llvm.exe

rm *.s *.ll 
