# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

include(CheckCXXSourceRuns)
include(CMakePushCheckState)

set(_FILESYSTEM_TEST_RPOGRAM "
#include <iostream>

#ifdef EXPERIMENTAL_FS
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

function(test_program filesystem_libraries use_experimental_fs is_ok)
  cmake_push_check_state(RESET)
  set(${is_ok} TRUE PARENT_SCOPE)
  if(${use_experimental_fs})
    set(CMAKE_REQUIRED_DEFINITIONS "-DEXPERIMENTAL_FS")
    set(TEST_SUFFIX "EXPERIMENTAL")
  else()
    set(TEST_SUFFIX "NATIVE")
  endif()

  check_cxx_source_runs("${_FILESYSTEM_TEST_RPOGRAM}" _FILESYSTEM_${TEST_SUFFIX})

  if(_FILESYSTEM_${TEST_SUFFIX})
    # Compiler has a native support for std::filesystem
    set(${filesystem_libraries} "" PARENT_SCOPE)
  else()
    set(CMAKE_REQUIRED_LIBRARIES "stdc++fs")
    check_cxx_source_runs("${_FILESYSTEM_TEST_RPOGRAM}" _FILESYSTEM_STDCPPFS_${TEST_SUFFIX})

    if(_FILESYSTEM_STDCPPFS_${TEST_SUFFIX})
      set(${filesystem_libraries} "stdc++fs" PARENT_SCOPE)
    else()
      set(CMAKE_REQUIRED_LIBRARIES "c++fs")
      check_cxx_source_runs("${_FILESYSTEM_TEST_RPOGRAM}" _FILESYSTEM_CPPFS_${TEST_SUFFIX})

      if(_FILESYSTEM_CPPFS_${TEST_SUFFIX})
        set(${filesystem_libraries} "c++fs" PARENT_SCOPE)
      else()
        set(${is_ok} FALSE PARENT_SCOPE)
      endif()
    endif()
  endif()
  cmake_pop_check_state()
endfunction()

set(Filesystem_LIBRARIES "")
set(IS_OK FALSE)
test_program(Filesystem_LIBRARIES FALSE IS_OK)

IF(IS_OK)
  set(IS_STD_FILESYSTEM_SUPPORT TRUE)
else()
  set(Filesystem_LIBRARIES "")
  set(USE_EXPERIMENTAL_FS TRUE)
  test_program(Filesystem_LIBRARIES TRUE IS_OK)

  if(IS_OK)
    set(IS_STD_FILESYSTEM_SUPPORT FALSE)
  else()
    message(FATAL_ERROR "C++ compiler does not support std[::experimental]::filesystem")
  endif()
endif()

if(DEFINED Filesystem_LIBRARIES)
  if(Filesystem_LIBRARIES)
    set_property(TARGET PeanoOptions APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${Filesystem_LIBRARIES})
  endif()

  if(NOT IS_STD_FILESYSTEM_SUPPORT)
    set_property(TARGET PeanoOptions APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS EXPERIMENTAL_FS)
  endif()
endif()
