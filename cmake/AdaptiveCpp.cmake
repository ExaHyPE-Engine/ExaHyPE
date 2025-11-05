# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

include_guard(GLOBAL)

set(ACPP_VERSION "25.02.0" CACHE STRING "")
set(ACPP_TARGETS "generic" CACHE STRING "")
set(ACPP_COMPILER_FEATURE_PROFILE "full" CACHE STRING "")
set(ACPP_DEBUG_LEVEL 0 CACHE STRING "0=no debug, 1=errors, 2=warnings, 3=info")

find_package(AdaptiveCpp CONFIG QUIET)

if(NOT AdaptiveCpp_FOUND)
  message(STATUS "AdaptiveCpp ${ACPP_VERSION} not found; fetching sources...")
  include(FetchContent)
  set(FETCHCONTENT_QUIET ON)

  FetchContent_GetProperties(AdaptiveCpp)
  if(NOT AdaptiveCpp_POPULATED)
    FetchContent_Populate(AdaptiveCpp
      URL https://github.com/AdaptiveCpp/AdaptiveCpp/archive/refs/tags/v${ACPP_VERSION}.tar.gz
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
  endif()

  set(adaptivecpp_BINARY_DIR ${CMAKE_BINARY_DIR}/AdaptiveCpp-build)
  set(adaptivecpp_INSTALL_DIR ${CMAKE_BINARY_DIR}/AdaptiveCpp-install)
  file(MAKE_DIRECTORY ${adaptivecpp_BINARY_DIR})
  file(MAKE_DIRECTORY ${adaptivecpp_INSTALL_DIR})

  execute_process(
    COMMAND ${CMAKE_COMMAND}
      -G "${CMAKE_GENERATOR}"
      -S ${adaptivecpp_SOURCE_DIR}
      -B ${adaptivecpp_BINARY_DIR}
      -DCMAKE_INSTALL_PREFIX=${adaptivecpp_INSTALL_DIR}
      -DCMAKE_BUILD_TYPE=Release
      -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
    RESULT_VARIABLE result
    OUTPUT_QUIET
    ERROR_QUIET
  )
  if(result)
    message(FATAL_ERROR "CMake step for AdaptiveCpp failed: ${result}")
  endif()

  execute_process(
    COMMAND ${CMAKE_COMMAND} --build ${adaptivecpp_BINARY_DIR} --config Release --target install --parallel ${AVAILABLE_PROCESSOR_COUNT}
    RESULT_VARIABLE result
    OUTPUT_QUIET
    ERROR_QUIET
  )
  if(result)
    message(FATAL_ERROR "Build step for AdaptiveCpp failed: ${result}")
  endif()

  message(STATUS "AdaptiveCpp was successfully installed at ${adaptivecpp_INSTALL_DIR}")
  find_package(AdaptiveCpp CONFIG REQUIRED HINTS ${adaptivecpp_INSTALL_DIR})
endif()

message(STATUS "Found AdaptiveCpp: ${AdaptiveCpp_DIR}")
message(STATUS "Using AdaptiveCpp targets: ${ACPP_TARGETS}")

target_compile_definitions(PeanoOptions INTERFACE WITH_GPU_SYCL WITH_GPU)
