# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
include(KokkosArch)
kokkos_configure_cache()

set(KOKKOS_VERSION 4.5.01)

find_package(Kokkos CONFIG)

if(NOT Kokkos_FOUND)
  message(STATUS "Installing Kokkos from GitHub Release ${KOKKOS_VERSION} using ${AVAILABLE_PROCESSOR_COUNT} processors")

  kokkos_populate_user_args(Kokkos_USER_CMAKE_ARGS)

  include(FetchContent)
  set(FETCHCONTENT_QUIET ON)
  FetchContent_Declare(
    Kokkos
    GIT_REPOSITORY https://github.com/kokkos/kokkos.git
    GIT_TAG ${KOKKOS_VERSION}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  if(NOT Kokkos_POPULATED)
    FetchContent_Populate(Kokkos)
    FetchContent_GetProperties(Kokkos)
  endif()

  set(kokkos_BINARY_DIR ${CMAKE_BINARY_DIR}/Kokkos-build)
  set(kokkos_INSTALL_DIR ${CMAKE_BINARY_DIR}/Kokkos-install)
  file(MAKE_DIRECTORY ${kokkos_BINARY_DIR})
  file(MAKE_DIRECTORY ${kokkos_INSTALL_DIR})

  execute_process(
    COMMAND ${CMAKE_COMMAND}
      -G "${CMAKE_GENERATOR}"
      -S ${kokkos_SOURCE_DIR}
      -B ${kokkos_BINARY_DIR}
      -DCMAKE_INSTALL_PREFIX=${kokkos_INSTALL_DIR}
      ${Kokkos_USER_CMAKE_ARGS}
    RESULT_VARIABLE result
    OUTPUT_QUIET
    ERROR_QUIET
  )
  if(result)
    message(FATAL_ERROR "CMake step for Kokkos failed: ${result}")
  endif()

  execute_process(
    COMMAND ${CMAKE_COMMAND} --build ${kokkos_BINARY_DIR} --target install -- -j ${AVAILABLE_PROCESSOR_COUNT}
    RESULT_VARIABLE rc2
    OUTPUT_QUIET
    ERROR_QUIET
  )
  if(result)
    message(FATAL_ERROR "Build step for Kokkos failed: ${result}")
  endif()

  message(STATUS "Kokkos was successfully installed at ${kokkos_INSTALL_DIR}!")
  find_package(Kokkos CONFIG REQUIRED HINTS ${kokkos_INSTALL_DIR})

  # Hide new Kokkos variables in ccmake
  get_cmake_property(allCacheVars CACHE_VARIABLES)
  foreach(var ${allCacheVars})
    if(var MATCHES "^Kokkos_")
      set_property(CACHE ${var} PROPERTY ADVANCED 1)
    endif()
  endforeach()
endif()

set(ONLY_IF_GPU_FIND_DEPENDENCY "find_dependency(Kokkos)")
target_link_libraries(PeanoOptions INTERFACE Kokkos::kokkos)
target_compile_definitions(PeanoOptions INTERFACE WITH_GPU_KOKKOS WITH_GPU)
