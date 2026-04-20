# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

include_guard(GLOBAL)

set(ACPP_VERSION "25.10.0" CACHE STRING "")
set(ACPP_TARGETS "" CACHE STRING "")
set(ACPP_COMPILER_FEATURE_PROFILE "full" CACHE STRING "")
set(ACPP_DEBUG_LEVEL 0 CACHE STRING "0=no debug, 1=errors, 2=warnings, 3=info")

function(_adaptivecpp_build_parallel_args out_var out_jobs_var)
  if(DEFINED AVAILABLE_PROCESSOR_COUNT AND AVAILABLE_PROCESSOR_COUNT GREATER 0)
    set(_build_jobs "${AVAILABLE_PROCESSOR_COUNT}")
  else()
    include(ProcessorCount)
    ProcessorCount(_build_jobs)
  endif()

  if(DEFINED _build_jobs AND _build_jobs GREATER 0)
    set(${out_var} --parallel ${_build_jobs} PARENT_SCOPE)
    set(${out_jobs_var} ${_build_jobs} PARENT_SCOPE)
  else()
    set(${out_var} "" PARENT_SCOPE)
    set(${out_jobs_var} "" PARENT_SCOPE)
  endif()
endfunction()

if(ACPP_TARGETS STREQUAL "")
  if(WITH_GPU_ARCH MATCHES "^sm_")
    set(ACPP_TARGETS "cuda:${WITH_GPU_ARCH}" CACHE STRING "" FORCE)
  elseif(WITH_GPU_ARCH MATCHES "^gfx")
    set(ACPP_TARGETS "hip:${WITH_GPU_ARCH}" CACHE STRING "" FORCE)
  else()
    set(ACPP_TARGETS "generic" CACHE STRING "" FORCE)
  endif()
endif()

find_package(AdaptiveCpp CONFIG QUIET)

if(NOT AdaptiveCpp_FOUND)
  message(STATUS "AdaptiveCpp ${ACPP_VERSION} not found; fetching sources...")
  include(FetchContent)
  set(FETCHCONTENT_QUIET ON)
  _adaptivecpp_build_parallel_args(AdaptiveCpp_BUILD_PARALLEL_ARGS AdaptiveCpp_BUILD_JOBS)

  if(AdaptiveCpp_BUILD_JOBS)
    set(AdaptiveCpp_CONFIGURE_PARALLEL_ARGS
      -DACPP_SUBPROJECT_PARALLEL_JOBS=${AdaptiveCpp_BUILD_JOBS}
    )
  else()
    set(AdaptiveCpp_CONFIGURE_PARALLEL_ARGS "")
  endif()

  FetchContent_GetProperties(AdaptiveCpp)
  if(NOT AdaptiveCpp_POPULATED)
    FetchContent_Populate(AdaptiveCpp
      URL https://github.com/AdaptiveCpp/AdaptiveCpp/archive/refs/tags/v${ACPP_VERSION}.tar.gz
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
  endif()

  set(adaptivecpp_BINARY_DIR ${CMAKE_BINARY_DIR}/adaptivecpp-build)
  set(adaptivecpp_INSTALL_DIR ${CMAKE_BINARY_DIR}/adaptivecpp-install)
  file(MAKE_DIRECTORY ${adaptivecpp_BINARY_DIR})
  file(MAKE_DIRECTORY ${adaptivecpp_INSTALL_DIR})

  execute_process(
    COMMAND ${CMAKE_COMMAND}
      -G ${CMAKE_GENERATOR}
      -S ${adaptivecpp_SOURCE_DIR}
      -B ${adaptivecpp_BINARY_DIR}
      -DCMAKE_INSTALL_PREFIX=${adaptivecpp_INSTALL_DIR}
      -DCMAKE_BUILD_TYPE=Release
      -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
      -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
      -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
      ${AdaptiveCpp_CONFIGURE_PARALLEL_ARGS}
    RESULT_VARIABLE result
    OUTPUT_QUIET
    # ERROR_QUIET
  )
  if(result)
    message(FATAL_ERROR "CMake step for AdaptiveCpp failed: ${result}")
  endif()

  execute_process(
    COMMAND ${CMAKE_COMMAND} --build ${adaptivecpp_BINARY_DIR} --config Release --target install ${AdaptiveCpp_BUILD_PARALLEL_ARGS}
    RESULT_VARIABLE result
    OUTPUT_QUIET
    # ERROR_QUIET
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
