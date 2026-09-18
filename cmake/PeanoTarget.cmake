# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

include_guard(GLOBAL)

macro(peano_find_gpu_backend)
  _peano_resolve_gpu_backend(_peano_fgb_backend)

  if(_peano_fgb_backend STREQUAL "cuda")
    find_package(CUDA REQUIRED)
  elseif(_peano_fgb_backend STREQUAL "hip")
    find_package(HIP REQUIRED)
  elseif(_peano_fgb_backend STREQUAL "sycl")
    # The "sycl" backend is served either by AdaptiveCpp (PEANO_WITH_GPU_BACKEND=acpp) or by
    # Intel oneAPI (PEANO_WITH_GPU_BACKEND=native + spir64). In the core build the user
    # selection PEANO_WITH_GPU_BACKEND disambiguates; in an application PEANO_ACPP_TARGETS
    # (exported only for AdaptiveCpp builds) does.
    if((DEFINED PEANO_WITH_GPU_BACKEND AND PEANO_WITH_GPU_BACKEND STREQUAL "acpp")
       OR (DEFINED PEANO_ACPP_TARGETS AND NOT PEANO_ACPP_TARGETS STREQUAL ""))
      find_package(AdaptiveCpp REQUIRED)
    else()
      find_package(SYCL REQUIRED)
    endif()
  elseif(_peano_fgb_backend STREQUAL "kokkos")
    find_package(Kokkos REQUIRED)
  elseif(_peano_fgb_backend STREQUAL "ompt")
    find_package(OMPT REQUIRED)
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

  foreach(source IN LISTS ARG_SOURCES ARG_GENERATED_SOURCES)
    get_filename_component(source_extension "${source}" EXT)
    if(source_extension MATCHES "^\\.(c|cc|cpp|cxx)$")
      if(IS_ABSOLUTE "${source}")
        set(clang_tidy_source "${source}")
      else()
        get_filename_component(clang_tidy_source "${CMAKE_CURRENT_SOURCE_DIR}/${source}" ABSOLUTE)
      endif()
      set_property(GLOBAL APPEND PROPERTY PEANO_CLANG_TIDY_SOURCES "${clang_tidy_source}")
    endif()
  endforeach()

  _peano_resolve_gpu_backend(gpu_backend)

  if(target_type STREQUAL "LIBRARY" OR target_type STREQUAL "STATIC_LIBRARY")
    add_library(${target} STATIC)
    set(install_as_library TRUE)
    set(variant_scope PUBLIC)
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
  elseif(CMAKE_DISABLE_PRECOMPILE_HEADERS OR CMAKE_CXX_COMPILER_ID STREQUAL "NVHPC")
    set(supports_precompiled_headers FALSE)
  else()
    set(supports_precompiled_headers TRUE)
  endif()

  if(supports_precompiled_headers)
    target_precompile_headers(${target} PRIVATE "<tarch/StdAfx.h>")
    target_compile_options(${target} PRIVATE
      "$<$<COMPILE_LANG_AND_ID:CUDA,NVIDIA>:SHELL:-include tarch/StdAfx.h>"
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
    # PeanoOptions follows linked libraries so its diagnostic flags take precedence
    # over build-mode defaults.
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

  # PEANO_<MODE>_DEFINITIONS, PEANO_C_FLAGS_<MODE>, PEANO_CXX_FLAGS_<MODE> and
  # PEANO_Fortran_FLAGS_<MODE> are core-build cache variables. In a generated application
  # they are undefined and expand to nothing here; the application instead inherits the mode
  # definitions and flags transitively from the linked Peano::PeanoRuntime<MODE><DIM>D
  # target (a STATIC library, hence PUBLIC scope). These calls are therefore only
  # effective in the core build.
  target_compile_definitions(${target} ${variant_scope}
    ${PEANO_${mode_upper}_DEFINITIONS}
  )

  # Build-mode flags must reach every language so Fortran kernels and GPU kernels are
  # optimised/debugged like host code. Fortran uses its own -O/-g-only flag set (the C++
  # flags contain switches not all Fortran front-ends accept). HIP takes the host flags
  # verbatim (clang optimises host and device from a single -O). nvcc is split: The host
  # flags are forwarded via -Xcompiler, and the optimisation level is additionally forwarded
  # to the device compiler via -Xptxas, making the device optimisation level explicit.
  # Debug uses -G, which forces device -O0 plus debug info.
  set(_peano_cuda_mode_flags "")
  foreach(_peano_flag ${PEANO_CXX_FLAGS_${mode_upper}})
    list(APPEND _peano_cuda_mode_flags "$<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=${_peano_flag}>")
    if(_peano_flag MATCHES "^-O[0-9]" AND NOT mode_upper STREQUAL "DEBUG")
      list(APPEND _peano_cuda_mode_flags "$<$<COMPILE_LANGUAGE:CUDA>:-Xptxas=${_peano_flag}>")
    endif()
  endforeach()
  if(mode_upper STREQUAL "DEBUG")
    list(APPEND _peano_cuda_mode_flags "$<$<COMPILE_LANGUAGE:CUDA>:-G>")
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

  # GPU language setup sets one architecture in CMAKE_CUDA_ARCHITECTURES or
  # CMAKE_HIP_ARCHITECTURES; target properties must consume that single selection.
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

  # AdaptiveCpp is located up front by peano_find_gpu_backend() (core build and,
  # for the SYCL/AdaptiveCpp backend, the application package), so the toolchain
  # and add_sycl_to_target() are already available here.
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
