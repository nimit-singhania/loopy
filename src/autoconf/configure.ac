dnl **************************************************************************
dnl * Initialize
dnl **************************************************************************
AC_INIT([Polly],[0.01],[polly-dev@googlegroups.com])

dnl Identify where LLVM source tree is
LLVM_SRC_ROOT="`(cd $srcdir/../..; pwd)`"
LLVM_OBJ_ROOT="`(cd ../..; pwd)`"

dnl Tell autoconf that this is an LLVM project being configured
dnl This provides the --with-llvmsrc and --with-llvmobj options
LLVM_CONFIG_PROJECT($LLVM_SRC_ROOT,$LLVM_OBJ_ROOT)

dnl Tell autoconf that the auxiliary files are actually located in
dnl the LLVM autoconf directory, not here.
AC_CONFIG_AUX_DIR($LLVM_SRC/autoconf)

dnl Indicate that we require autoconf 2.59 or later. Ths is needed because we
dnl use some autoconf macros only available in 2.59.
AC_PREREQ(2.59)

dnl Verify that the source directory is valid
AC_CONFIG_SRCDIR(["lib/Analysis/ScopInfo.cpp"])

dnl Configure a common Makefile
AC_CONFIG_FILES(Makefile.config)
AC_CONFIG_FILES(Makefile.common)

dnl Configure project makefiles
dnl List every Makefile that exists within your source tree

dnl Quit if the source directory has already been configured.
dnl NOTE: This relies upon undocumented autoconf behavior.
if test ${srcdir} != "." ; then
  if test -f ${srcdir}/include/polly/Config/config.h ; then
    AC_MSG_ERROR([Already configured in ${srcdir}])
  fi
fi

dnl **************************************************************************
dnl * Determine which system we are building on
dnl **************************************************************************

dnl **************************************************************************
dnl * Check for programs.
dnl **************************************************************************

dnl AC_PROG_CPP
dnl AC_PROG_CC(gcc)
dnl AC_PROG_CXX(g++)

dnl **************************************************************************
dnl * Check for libraries.
dnl **************************************************************************

dnl **************************************************************************
dnl * Checks for header files.
dnl **************************************************************************

dnl **************************************************************************
dnl * Checks for typedefs, structures, and compiler characteristics.
dnl **************************************************************************

dnl **************************************************************************
dnl * Checks for library functions.
dnl **************************************************************************

dnl **************************************************************************
dnl * Enable various compile-time options
dnl **************************************************************************

dnl **************************************************************************
dnl * Set the location of various third-party software packages
dnl **************************************************************************

dnl Check if CUDA lib there
dnl Disable the build of polly, even if it is checked out into tools/polly.
AC_ARG_ENABLE(polly_gpu_codegen,
              AS_HELP_STRING([--enable-polly-gpu-codegen],
                             [Enable GPU code generation in Polly(default is NO)]),,
                             enableval=default)
case "$enableval" in
  yes) AC_DEFINE([GPU_CODEGEN],[1], [Define if gpu codegen is enabled]) ;;
  no) ;;
  default) ;;
  *) AC_MSG_ERROR([Invalid setting for --enable-polly-gpu-codegen. Use "yes" or "no"]) ;;
esac

find_lib_and_headers([cuda], [cuda.h], [cuda])

AS_IF([test "x$cuda_found" = "xyes"],
  [AC_DEFINE([CUDALIB_FOUND],[1],[Define if cudalib found])])

dnl **************************************************************************
dnl * Create the output files
dnl **************************************************************************

dnl Let ISL's configure generate isl_config.h and gitversion.h
AC_CONFIG_SUBDIRS(lib/External/isl)
AC_CONFIG_COMMANDS_POST([
  dnl Configure ISL with small integer optimization, but do not add the
  dnl option to config.status as it is mandatory for Polly.
  ac_configure_args="$ac_configure_args --with-int=imath-32"
])

dnl This must be last
AC_CONFIG_HEADERS(include/polly/Config/config.h)
AC_OUTPUT
