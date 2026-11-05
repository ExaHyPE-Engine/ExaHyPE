# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

include_guard(GLOBAL)

macro(peano_find_gpu_backend)
  _peano_resolve_gpu_backend(_peano_fgb_backend)

  if(_peano_fgb_backend STREQUAL "cuda")
    find_package(PeanoCUDA REQUIRED)
  elseif(_peano_fgb_backend STREQUAL "hip")
    find_package(PeanoHIP REQUIRED)
  elseif(_peano_fgb_backend STREQUAL "sycl")
    # In an application, PEANO_WITH_GPU_BACKEND is sycl for AdaptiveCpp and Intel oneAPI alike. A
    # non-empty PEANO_ACPP_TARGETS marks AdaptiveCpp.
    if((DEFINED PEANO_WITH_GPU_BACKEND AND PEANO_WITH_GPU_BACKEND STREQUAL "acpp")
       OR (DEFINED PEANO_ACPP_TARGETS AND NOT PEANO_ACPP_TARGETS STREQUAL ""))
      find_package(PeanoAdaptiveCpp REQUIRED)
    else()
      find_package(PeanoSYCL REQUIRED)
    endif()
  elseif(_peano_fgb_backend STREQUAL "kokkos")
    find_package(PeanoKokkos REQUIRED)
  elseif(_peano_fgb_backend STREQUAL "ompt")
    find_package(PeanoOMPT REQUIRED)
  endif()
endmacro()

function(peano_add_target)
  set(options NO_INSTALL)
  set(oneValueArgs
    TYPE
    NAME
    MODE
    DIMENSION
    OUTPUT_DIRECTORY
  )
  set(multiValueArgs
    SOURCES
    HEADERS
    GENERATED_SOURCES
    COMPILE_DEFINITIONS
    COMPILE_OPTIONS
    INCLUDE_DIRECTORIES
    LINK_LIBRARIES
    LINK_OPTIONS
  )

  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  foreach(required_arg IN ITEMS TYPE NAME)
    if(NOT ARG_${required_arg})
      message(FATAL_ERROR "peano_add_target() requires ${required_arg}")
    endif()
  endforeach()

  if(NOT ARG_MODE)
    message(FATAL_ERROR "peano_add_target() requires MODE")
  endif()

  string(TOUPPER "${ARG_TYPE}" target_type)
  set(target "${ARG_NAME}")
  set(target_sources ${ARG_HEADERS} ${ARG_SOURCES} ${ARG_GENERATED_SOURCES})
  set(install_as_library FALSE)
  set(install_as_executable FALSE)

  _peano_resolve_gpu_backend(gpu_backend)

  if(target_type STREQUAL "LIBRARY" OR target_type STREQUAL "STATIC_LIBRARY")
    add_library(${target} STATIC)
    set(install_as_library TRUE)
    set(variant_scope PUBLIC)
  elseif(target_type STREQUAL "OBJECT_LIBRARY")
    add_library(${target} OBJECT)
    set(variant_scope PRIVATE)
  elseif(target_type STREQUAL "EXECUTABLE")
    add_executable(${target})
    set(install_as_executable TRUE)
    set(variant_scope PRIVATE)
  else()
    message(FATAL_ERROR "Unsupported peano_add_target TYPE ${ARG_TYPE}")
  endif()

  if(ARG_NO_INSTALL)
    set(install_as_library FALSE)
    set(install_as_executable FALSE)
  endif()

  target_sources(${target} PRIVATE ${target_sources})

  if(gpu_backend STREQUAL "kokkos" OR gpu_backend STREQUAL "sycl" OR gpu_backend STREQUAL "ompt")
    set(supports_precompiled_headers FALSE)
  elseif(CMAKE_DISABLE_PRECOMPILE_HEADERS
    OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
    OR CMAKE_CXX_COMPILER_ID STREQUAL "NVHPC")
    set(supports_precompiled_headers FALSE)
  else()
    set(supports_precompiled_headers TRUE)
  endif()

  if(supports_precompiled_headers)
    target_precompile_headers(${target} PRIVATE "<tarch/StdAfx.h>")
    target_compile_options(${target} PRIVATE
      "$<$<COMPILE_LANG_AND_ID:CUDA,NVIDIA>:SHELL:-include tarch/StdAfx.h>"
      "$<$<COMPILE_LANGUAGE:HIP>:SHELL:-include tarch/StdAfx.h>"
    )
  else()
    target_compile_options(${target} PRIVATE
      "$<$<COMPILE_LANGUAGE:C,CXX,CUDA,HIP>:SHELL:-include tarch/StdAfx.h>"
    )
  endif()

  if(TARGET Peano::PeanoOptions)
    set(peano_options_target Peano::PeanoOptions)
  elseif(TARGET PeanoOptions)
    set(peano_options_target PeanoOptions)
  else()
    message(FATAL_ERROR "PeanoOptions target is not available")
  endif()

  target_link_libraries(${target} PRIVATE
    ${ARG_LINK_LIBRARIES}
    # PeanoOptions follows the linked libraries. Its diagnostic flags then override the build-mode
    # defaults.
    ${peano_options_target}
  )

  target_compile_options(${target} PRIVATE ${ARG_COMPILE_OPTIONS})
  target_compile_definitions(${target} PRIVATE ${ARG_COMPILE_DEFINITIONS})
  target_include_directories(${target} PRIVATE ${ARG_INCLUDE_DIRECTORIES})
  target_link_options(${target} PRIVATE ${ARG_LINK_OPTIONS})

  set_target_properties(${target} PROPERTIES
    C_EXTENSIONS OFF
    CXX_EXTENSIONS OFF
    CXX_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_INLINES_HIDDEN YES
    POSITION_INDEPENDENT_CODE ON
  )

  if(ARG_OUTPUT_DIRECTORY)
    set_target_properties(${target} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${ARG_OUTPUT_DIRECTORY}"
      LIBRARY_OUTPUT_DIRECTORY "${ARG_OUTPUT_DIRECTORY}"
      ARCHIVE_OUTPUT_DIRECTORY "${ARG_OUTPUT_DIRECTORY}"
    )
  endif()

  string(TOUPPER "${ARG_MODE}" mode_upper)

  if(ARG_DIMENSION)
    target_compile_definitions(${target} ${variant_scope}
      DIMENSIONS=${ARG_DIMENSION}
    )
  endif()

  # The PEANO_<MODE>_DEFINITIONS and PEANO_<LANGUAGE>_FLAGS_<MODE> cache variables exist in the core
  # build only. An application inherits them through the PUBLIC scope of the static libraries that
  # Peano::PeanoRuntime<MODE><DIM>D links.
  target_compile_definitions(${target} ${variant_scope}
    ${PEANO_${mode_upper}_DEFINITIONS}
  )

  # Fortran front-ends reject some C++ switches. Fortran gets its own build-mode flags. The CUDA
  # compiler passes -Xcompiler flags to the host compiler and -Xptxas flags to the device assembler.
  set(_peano_cuda_mode_flags "")
  foreach(_peano_flag ${PEANO_CXX_FLAGS_${mode_upper}})
    list(APPEND _peano_cuda_mode_flags "$<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=${_peano_flag}>")
    if(_peano_flag MATCHES "^-O[0-9]" AND NOT mode_upper STREQUAL "DEBUG")
      list(APPEND _peano_cuda_mode_flags "$<$<COMPILE_LANGUAGE:CUDA>:-Xptxas=${_peano_flag}>")
    endif()
  endforeach()
  if(mode_upper STREQUAL "DEBUG")
    #list(APPEND _peano_cuda_mode_flags "$<$<COMPILE_LANGUAGE:CUDA>:-G>")
    list(APPEND _peano_cuda_mode_flags "$<$<COMPILE_LANGUAGE:CUDA>:-lineinfo>")
  endif()
  target_compile_options(${target} ${variant_scope}
    $<$<COMPILE_LANGUAGE:C>:${PEANO_C_FLAGS_${mode_upper}}>
    $<$<COMPILE_LANGUAGE:CXX>:${PEANO_CXX_FLAGS_${mode_upper}}>
    $<$<COMPILE_LANGUAGE:HIP>:${PEANO_CXX_FLAGS_${mode_upper}}>
    $<$<COMPILE_LANGUAGE:Fortran>:${PEANO_Fortran_FLAGS_${mode_upper}}>
    ${_peano_cuda_mode_flags}
  )

  source_group(
    TREE ${CMAKE_CURRENT_SOURCE_DIR}
    FILES "$<TARGET_PROPERTY:${target},SOURCES>"
  )

  if(IPO_SUPPORTED AND NOT gpu_backend STREQUAL "kokkos")
    set_target_properties(${target} PROPERTIES
      INTERPROCEDURAL_OPTIMIZATION TRUE
    )
  endif()

  if(gpu_backend STREQUAL "cuda" AND CMAKE_CUDA_COMPILER)
    set_target_properties(${target} PROPERTIES
      CUDA_SEPARABLE_COMPILATION OFF
      CUDA_RESOLVE_DEVICE_SYMBOLS OFF
      CUDA_ARCHITECTURES "${CMAKE_CUDA_ARCHITECTURES}"
    )
  elseif(gpu_backend STREQUAL "hip" AND CMAKE_HIP_COMPILER)
    set_target_properties(${target} PROPERTIES
      HIP_ARCHITECTURES "${CMAKE_HIP_ARCHITECTURES}"
    )
  endif()

  # The macro peano_find_gpu_backend() loads AdaptiveCpp and its add_sycl_to_target() before any
  # target exists.
  if(DEFINED AdaptiveCpp_FOUND AND AdaptiveCpp_FOUND)
    add_sycl_to_target(TARGET ${target} SOURCES "$<TARGET_PROPERTY:${target},SOURCES>")
  endif()

  if(install_as_library)
    install(TARGETS ${target}
      EXPORT ${PROJECT_NAME}Targets
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
  elseif(install_as_executable)
    install(TARGETS ${target}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
  endif()
endfunction()
