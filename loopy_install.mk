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
CLANG_DIR ${ROOT_DIR}/clang
CLANG_BUILD_DIR=${CLANG_DIR}/build

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
	mkdir -p ${CLANG_DIR}
	mkdir -p ${CLANG_DIR}/build

# install LLVM, require specific version
get_llvm: setup
	wget https://releases.llvm.org/3.7.0/llvm-3.7.0.src.tar.xz
	tar -xvf llvm-3.7.0.src.tar.xz
	mv llvm-3.7.0.src/ ${LLVM_DIR}/trunk

get_clang: setup
	wget https://releases.llvm.org/3.7.0/cfe-3.7.0.src.tar.xz
	tar -xvf cfe-3.7.0.src.tar.xz
	mv cfe-3.7.0.src ${CLANG_DIR}/trunk

# copy Loopy code
get_loopy: setup
	cp -R ${SRC_DIR}/ ${LLVM_DIR}/trunk/tools/polly

# build LLVM + loopy
build: setup get_loopy get_clang get_llvm 
	(cd ${LLVM_BUILD_DIR}; cmake -G 'Unix Makefiles' -DCMAKE_INSTALL_PREFIX=${LLVM_DIR} ${LLVM_DIR}/trunk; make -j ${PAR_BUILD})
	(cd ${CLANG_BUILD_DIR}; cmake -G 'Unix Makefiles' -DCMAKE_INSTALL_PREFIX=${LLVM_DIR} ${CLANG_DIR}/trunk; make -j ${PAR_BUILD})


# =========================== Test =========================== 
# run Loopy tests
test: 
	cd ${TESTS_DIR}; \
	sh clean-up.sh; \
	sh run-all-tests.sh ${LLVM_BUILD_DIR}

