# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

set(ADAPTIVE_CPP_VERSION 24.10.0)
set(ACPP_TARGETS generic)

# if(PEANO_NVIDIA_GPU)
#   set(ACPP_TARGETS cuda:${WITH_GPU_ARCH})
# elseif(PEANO_AMD_GPU)
#   set(ACPP_TARGETS hip:${WITH_GPU_ARCH})
# else()
#   set(ACPP_TARGETS generic)
# endif()

find_package(AdaptiveCpp CONFIG)

if(NOT ${AdaptiveCpp_FOUND})
  message(STATUS "Installing AdaptiveCpp from GitHub Release ${ADAPTIVE_CPP_VERSION} using ${AVAILABLE_PROCESSOR_COUNT} processors")

  cmake_policy(SET CMP0169 OLD)
  include(FetchContent)
  set(FETCHCONTENT_QUIET ON)
  FetchContent_Declare(
    AdaptiveCpp
    URL https://github.com/AdaptiveCpp/AdaptiveCpp/archive/refs/tags/v${ADAPTIVE_CPP_VERSION}.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  if(NOT AdaptiveCpp_POPULATED)
    FetchContent_Populate(AdaptiveCpp)
    FetchContent_GetProperties(AdaptiveCpp)
  endif()

  set(adaptivecpp_BINARY_DIR ${CMAKE_BINARY_DIR}/AdaptiveCpp-build)
  set(adaptivecpp_INSTALL_DIR ${CMAKE_BINARY_DIR}/AdaptiveCpp-install)

  file(MAKE_DIRECTORY ${adaptivecpp_BINARY_DIR})
  file(MAKE_DIRECTORY ${adaptivecpp_INSTALL_DIR})

  execute_process(
    COMMAND ${CMAKE_COMMAND}
      -G "${CMAKE_GENERATOR}" -S ${adaptivecpp_SOURCE_DIR} -B ${adaptivecpp_BINARY_DIR}
      -DCMAKE_INSTALL_PREFIX=${adaptivecpp_INSTALL_DIR}
      -DCMAKE_BUILD_TYPE=Release
      -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
      -DACPP_COMPILER_FEATURE_PROFILE=full
      RESULT_VARIABLE result
      OUTPUT_QUIET
      ERROR_QUIET
  )
  if(result)
    message(FATAL_ERROR "CMake step for AdaptiveCpp failed: ${result}")
  endif()

  execute_process(
    COMMAND ${CMAKE_COMMAND} --build ${adaptivecpp_BINARY_DIR} --config Release --target install -- -j ${AVAILABLE_PROCESSOR_COUNT}
    RESULT_VARIABLE result
    OUTPUT_QUIET
    ERROR_QUIET
  )
  if(result)
    message(FATAL_ERROR "Build step for AdaptiveCpp failed: ${result}")
  endif()

  message(STATUS "AdaptiveCpp was successfully installed at ${adaptivecpp_INSTALL_DIR}!")
  find_package(AdaptiveCpp CONFIG REQUIRED HINTS ${adaptivecpp_INSTALL_DIR})
endif()

set(USE_CCACHE OFF CACHE BOOL "Use Ccache" FORCE)
set(ACPP_DEBUG_LEVEL 0 CACHE STRING "Choose the debug level, options are: 0 (no debug), 1 (print errors), 2 (also print warnings), 3 (also print general information)")

message(STATUS "Found AdaptiveCpp: ${AdaptiveCpp_DIR}")
message(STATUS "Using AdaptiveCpp targets: ${ACPP_TARGETS}")
