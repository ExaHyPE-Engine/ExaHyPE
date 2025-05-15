# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

# split “sm_70”, “gfx908”, “spir64” → prefix/suffix
string(REGEX MATCH "^(sm_|gfx|spir)(.+)$" _matched "${WITH_GPU_ARCH}")
if(NOT _matched)
  message(FATAL_ERROR "Invalid GPU architecture: ${WITH_GPU_ARCH}")
endif()
set(_ARCH_PREFIX "${CMAKE_MATCH_1}"  CACHE INTERNAL "")
set(_ARCH_SUFFIX "${CMAKE_MATCH_2}"  CACHE INTERNAL "")

# maps suffix→CMake tag
set(_KOKKOS_CUDA_MAP
        70 VOLTA70
        72 VOLTA72
        75 TURING75
        80 AMPERE80
        86 AMPERE86
        89 ADA89
        90 HOPPER90
)
set(_KOKKOS_HIP_MAP
        906 AMD_GFX906
        908 AMD_GFX908
        90a AMD_GFX90A
        942 AMD_GFX942
)
set(_KOKKOS_SYCL_MAP
        64 INTEL_PVC
)

# helper to handle both cache-sets and user-args
function(_kokkos__select prefix enable_var map_list name out_list_var use_cache)
  if(_ARCH_PREFIX STREQUAL prefix)
    # turn on backend
    if(use_cache)
      set(${enable_var} ON CACHE BOOL "Enable ${name} in Kokkos" FORCE)
    else()
      list(APPEND _USER_ARGS ${enable_var})
    endif()

    # find suffix in map
    list(FIND ${map_list} ${_ARCH_SUFFIX} _i)
    if(_i EQUAL -1)
      message(FATAL_ERROR "Unknown ${name} arch: ${WITH_GPU_ARCH}")
    endif()
    math(EXPR _tag_i "${_i} + 1")
    list(GET ${map_list} ${_tag_i} _tag)

    # set arch flag
    if(use_cache)
      set("Kokkos_ARCH_${_tag}" ON CACHE BOOL "Kokkos arch ${_tag}" FORCE)
    else()
      list(APPEND _USER_ARGS "-DKokkos_ARCH_${_tag}=ON")
    endif()

    set(_ARCH_CONFIGURED TRUE PARENT_SCOPE)
    if(NOT use_cache)
      set(${out_list_var} "${_USER_ARGS}" PARENT_SCOPE)
    endif()
  endif()
endfunction()

# public: configure CACHE variables
function(kokkos_configure_cache)
  set(Kokkos_ARCH_NATIVE   ON CACHE BOOL "Use host-native architecture" FORCE)
  set(Kokkos_ENABLE_OPENMP ON CACHE BOOL "Use OpenMP"                   FORCE)

  # process GPUs
  _kokkos__select("sm_"  Kokkos_ENABLE_CUDA  _KOKKOS_CUDA_MAP  "CUDA"  ""          TRUE)
  _kokkos__select("gfx"  Kokkos_ENABLE_HIP   _KOKKOS_HIP_MAP   "HIP"   ""          TRUE)
  _kokkos__select("spir" Kokkos_ENABLE_SYCL  _KOKKOS_SYCL_MAP  "SYCL"  ""          TRUE)

  if(NOT _ARCH_CONFIGURED)
    message(FATAL_ERROR "Unsupported GPU architecture prefix '${_ARCH_PREFIX}'")
  endif()
endfunction()

# public: build user-args list for FetchContent
function(kokkos_populate_user_args out_var)
  set(_USER_ARGS
    "-DKokkos_ENABLE_SERIAL=ON"
    "-DKokkos_ENABLE_OPENMP=ON"
    "-DKokkos_ARCH_NATIVE=ON"
  )

  _kokkos__select("sm_"  "-DKokkos_ENABLE_CUDA=ON"  _KOKKOS_CUDA_MAP  "CUDA"  _USER_ARGS  FALSE)
  _kokkos__select("gfx"  "-DKokkos_ENABLE_HIP=ON"   _KOKKOS_HIP_MAP   "HIP"   _USER_ARGS  FALSE)
  _kokkos__select("spir" "-DKokkos_ENABLE_SYCL=ON"  _KOKKOS_SYCL_MAP  "SYCL"  _USER_ARGS  FALSE)

  set(${out_var} "${_USER_ARGS}" PARENT_SCOPE)
endfunction()
