# Loopy
Loopy is a system for programming loop transformations. Current practice is either to manually rewrite loops, which can be tedious and error prone, or apply automatic optimizations built into a compiler, which have varying performance benefits. Loopy occupies a middle ground: programmers specify a sequence of transformations in a source-level script, which is then interpreted fully automatically to generate the transformed program. While other such systems exist, the key distinguishing features of Loopy are:

1. Transformations are specified at source-level, which makes it easy for a programmer to describe and reason about the transformation.

2. Transformation scripts are automatically verified for correctness, to ensure that incorrect specifications are not implemented. 

Loopy is designed as an interpreter which extends Polly, a polyhedral library for LLVM. It reads in each operation of the transformation script and applies that on a polyhedral model of the program. Finally, it checks whether all program dependencies from the old model are preserved in the new model, and generates the new code only if this condition is met. 

## Thanks!
   Loopy is built by extending the Polly and ISL libraries. We are deeply grateful to the authors of these libraries, it would have been a far more difficult task to implement Loopy otherwise.  

   This work was supported, in part, by DARPA under agreement number FA8750-12-C-0166. The U.S. Government is authorized to reproduce and distribute reprints for Governmental purposes notwithstanding any copyright notation thereon. The views and conclusions contained herein are those of the authors and should not be interpreted as necessarily representing the official policies or endorsements, either expressed or implied, of DARPA or the U.S. Government.  


##Example
As an example, consider the following matrix multiplication program:
```c
for (i = 0; i < N; i++) {
  for (j = 0; j < N; j++) {
    {
      Init:  C[i][j] = 0;
    }
    for (k = 0; k < N; k++) {
      Mult:  C[i][j] += A[i][k] * B[k][j];
    }
  }
}
```
Note that the code is split into two components: `Init`, where matrix `C` is initialized, and `Mult`, where `C` is computed from `A` and `B`. 
Now, consider the following script, which optimizes the cache performance of the program: 
```c
realign(Init, Mult, 0)
affine(Mult, { [i, j, k] -> [i, k, j] } )
affine(Mult, { [i, j, k] -> [i1, j1, k1, i2, j2, k2]: i1 = [i/32] and i2 = i%32 and j1 = [j/32] and j2 = j%32 and k1 = [k/32] and k2 = k%32 } )
```
It consists of 3 operations. The first operation splits components `Init` and `Mult` into independent loop nests. The second interchanges loop indices `j` and `k` (which improves spatial locality of accesses to `B`). The last operation tiles the loop nest of `Mult` (which improves temporal locality of accesses to `A` and `B`). The loop handles, `Init` and `Mult`, make it easy to specify the operations at the source-level. 

The above example can be found in **/example** directory of the repository. Instructions to obtain Loopy, Polly and LLVM versions are provided below. 

## Usage
### Installation
1. Checkout loopy  
    * `LOOPY_ROOT=any directory you like`
    * `cd $LOOPY_ROOT`   
    * `git clone https://github.com/nimit-singhania/loopy.git`   
2. Set variable `ROOT_DIR=$LOOPY_ROOT` in `loopy_install.mk`
3. Execute:  
    `make -f loopy_install.mk`

### Executing matrix multiplication example 
To run Loopy on the matrix multiplication example, execute

* `cd example`
* `run-loopy.sh ../llvm/build matmul.c matmul.t`

The script `run-loopy.sh` can be used to run Loopy on other programs. See `example/matmul.t` for detailed instructions on writing the transformation script.

### Launching Polybench tests
Loopy has been evaluated against Polly and LLVM on Polybench, a popular polyhedral benchmark suite. The test-suite is located in **/tests** directory. To re-run these tests, execute  

*  `make -f loopy_install.mk test`

The transformation script for each benchmark is located in the source directory of the benchmark in a file named `opt.t`. For example, the script for the benchmark **tests/polybench-c-4.1/linear-algebra/blas/gemm** is located in the file `tests/polybench-c-4.1/linear-algebra/blas/gemm/opt.t`.

#### Notes
Loopy has been built and tested on MacOSX and OpenSUSE Linux. Please note that in our tests on OpenSUSE, Loopy fails on two of the Polybench programs, 'cholesky' and 'gramschmidt'. Loopy seems to generate an incorrect polyhedral model for these programs, which prevents transformations from being applied correctly.

## References
Loopy is described in more detail in:     

* K.S. Namjoshi, N. Singhania. *Loopy: Programmable and Formally Verified Loop Transformations*. Static Analysis Symposium (SAS), 2016


## The tool [Loo.py](https://mathema.tician.de/software/loopy/) generates OpenCL/CUDA code from descriptions of computations

