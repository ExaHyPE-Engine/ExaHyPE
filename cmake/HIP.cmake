# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

include_guard(GLOBAL)

set(HIP_ARCH "${WITH_GPU_ARCH}")
set(HIP_PLATFORM "amd" CACHE STRING "" FORCE)
set(CMAKE_HIP_PLATFORM "amd" CACHE STRING "HIP platform" FORCE)

message(STATUS "Using HIP_ARCH=${HIP_ARCH}")
set(CMAKE_HIP_HOST_COMPILER "${CMAKE_CXX_COMPILER}")

enable_language(HIP)
set(CMAKE_HIP_ARCHITECTURES "${HIP_ARCH}" CACHE STRING "" FORCE)
find_package(hip CONFIG REQUIRED)
target_link_libraries(PeanoOptions INTERFACE hip::host)

target_compile_options(PeanoOptions INTERFACE
  -fPIC
  $<$<COMPILE_LANGUAGE:HIP>:--offload-arch=${HIP_ARCH}>
  $<$<COMPILE_LANGUAGE:HIP>:-O3>
)

target_compile_features(PeanoOptions INTERFACE hip_std_17)
set(GPU_FIND_DEPENDENCY "find_dependency(hip CONFIG)" CACHE INTERNAL "")
target_compile_definitions(PeanoOptions INTERFACE
  WITH_GPU_HIP
  WITH_GPU
)

if(CMAKE_HIP_COMPILER)
  set(WITH_HIP ON CACHE INTERNAL "HIP support is enabled")
else()
  set(WITH_HIP OFF CACHE INTERNAL "HIP support is disabled")
endif()

function(add_hip_to_target target)
  set_target_properties(${target} PROPERTIES
    POSITION_INDEPENDENT_CODE ON
  )
  if(CMAKE_HIP_ARCHITECTURES)
    set_target_properties(${target} PROPERTIES
      HIP_ARCHITECTURES "${CMAKE_HIP_ARCHITECTURES}"
    )
  endif()
endfunction()
