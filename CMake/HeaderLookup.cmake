include(CheckIncludeFiles)

check_include_files(dlfcn.h HAVE_DLFCN_H)
check_include_files(inttypes.h HAVE_INTTYPES_H)
check_include_files(nvToolsExt.h HAVE_NVTOOLSEXT_H)
check_include_files(omp.h HAVE_OMP_H)
check_include_files(stdint.h HAVE_STDINT_H)
check_include_files(stdio.h HAVE_STDIO_H)
check_include_files(stdlib.h HAVE_STDLIB_H)
check_include_files(strings.h HAVE_STRINGS_H)
check_include_files(string.h HAVE_STRING_H)
check_include_files(sys/stat.h HAVE_SYS_STAT_H)
check_include_files(sys/types.h HAVE_SYS_TYPES_H)
check_include_files(unistd.h HAVE_UNISTD_H)
check_include_files(CL/sycl.hpp HAVE_CL_SYCL_HPP)

# TODO:
# Additionally check the headers defined above have C90 standards
# Requires "stddef.h;float.h"
# We also need free in stdlib, memchr in string
# An example where how to implement the check is here:
# https://github.com/LuaDist/libgd/tree/master/cmake/modules
