# Install and build the Loopy system
# Please customize settings in the section immediately below. 

# =========================== Customize =========================== 

# Loopy root directory
ROOT_DIR=/home/loopy

# Number of processors to run make on 
PAR_BUILD=2


# =========================== Derived =========================== 
# LLVM root directory
LLVM_DIR=${ROOT_DIR}/llvm
LLVM_BUILD_DIR=${LLVM_DIR}/build

# Loopy source directory
SRC_DIR=${ROOT_DIR}/src

# Tests directory
TESTS_DIR=${ROOT_DIR}/tests

# =========================== Build rules =========================== 
.PHONY: all setup get_llvm get_clang get_loopy build test

all: setup get_llvm get_loopy get_clang build 

setup: 
	mkdir -p ${LLVM_DIR}
	mkdir -p ${LLVM_DIR}/build

# install LLVM, require specific version
get_llvm: setup
	svn co -r 241394 http://llvm.org/svn/llvm-project/llvm/trunk ${LLVM_DIR}/trunk

get_clang: setup
	 svn co -r 241394 http://llvm.org/svn/llvm-project/cfe/trunk ${LLVM_DIR}/trunk/tools/clang

# copy Loopy code
get_loopy: setup
	cp -R ${SRC_DIR}/ ${LLVM_DIR}/trunk/tools/polly

# build LLVM + loopy
build: setup get_loopy get_clang get_llvm 
	(cd ${LLVM_BUILD_DIR}; cmake -G 'Unix Makefiles' -DCMAKE_INSTALL_PREFIX=${LLVM_DIR} ${LLVM_DIR}/trunk; make -j ${PAR_BUILD})


# =========================== Test =========================== 
# run Loopy tests
test: 
	cd ${TESTS_DIR}; \
	sh clean-up.sh; \
	sh run-all-tests.sh ${LLVM_BUILD_DIR}

