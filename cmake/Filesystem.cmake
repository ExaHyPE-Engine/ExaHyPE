# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

include(CheckCXXSourceRuns)
include(CMakePushCheckState)

set(_FILESYSTEM_TEST_PROGRAM "
#include <iostream>

#ifdef USE_EXPERIMENTAL_FS
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

int main(int argc, char* argv[]) {
  fs::path p{argv[0]};
  std::cout << fs::canonical(p) << std::endl;
  return 0;
}")

function(check_filesystem_support out_libs_var out_defines_var out_success_var use_experimental)
  cmake_push_check_state(RESET)

  set(${out_success_var} TRUE PARENT_SCOPE)

  if(use_experimental)
    set(CMAKE_REQUIRED_DEFINITIONS -DUSE_EXPERIMENTAL_FS)
    set(TEST_SUFFIX EXPERIMENTAL)
  else()
    set(TEST_SUFFIX NATIVE)
  endif()

  set(lib_candidates "")
  list(APPEND lib_candidates "") # First try: no explicit link
  list(APPEND lib_candidates "stdc++fs" "c++fs") # Fallbacks

  set(lib_found "")
  foreach(lib ${lib_candidates})
    if(lib)
      set(CMAKE_REQUIRED_LIBRARIES ${lib})
    else()
      unset(CMAKE_REQUIRED_LIBRARIES)
    endif()

    check_cxx_source_runs("${_FILESYSTEM_TEST_PROGRAM}" _FS_TEST_${TEST_SUFFIX}_${lib})

    if(_FS_TEST_${TEST_SUFFIX}_${lib})
      set(lib_found "${lib}")
      break()
    endif()
  endforeach()

  if(lib_found STREQUAL "" AND NOT _FS_TEST_${TEST_SUFFIX}_)
    set(${out_success_var} FALSE PARENT_SCOPE)
  else()
    set(${out_libs_var} "${lib_found}" PARENT_SCOPE)
    if(use_experimental)
      set(${out_defines_var} "USE_EXPERIMENTAL_FS" PARENT_SCOPE)
    else()
      set(${out_defines_var} "" PARENT_SCOPE)
    endif()
  endif()

  cmake_pop_check_state()
endfunction()

set(Filesystem_LIBS "")
set(Filesystem_DEFINES "")
set(FILESYSTEM_SUPPORTED FALSE)

check_filesystem_support(Filesystem_LIBS Filesystem_DEFINES FILESYSTEM_SUPPORTED FALSE)

if(NOT FILESYSTEM_SUPPORTED)
  check_filesystem_support(Filesystem_LIBS Filesystem_DEFINES FILESYSTEM_SUPPORTED TRUE)
endif()

if(NOT FILESYSTEM_SUPPORTED)
  message(FATAL_ERROR "Your compiler does not support std::filesystem or std::experimental::filesystem.")
endif()

if(Filesystem_LIBS)
  set_property(TARGET PeanoOptions APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${Filesystem_LIBS})
endif()
if(Filesystem_DEFINES)
  set_property(TARGET PeanoOptions APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS ${Filesystem_DEFINES})
endif()
