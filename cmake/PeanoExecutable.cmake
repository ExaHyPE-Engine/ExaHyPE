# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

function(add_peano_executable)
  set(options)
  set(oneValueArgs NAME MODE DIMENSION)
  set(multiValueArgs
      SOURCES
      LINK_LIBRARIES
  )

  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  string(TOLOWER ${ARG_MODE} mode_lower)
  string(TOUPPER ${ARG_MODE} mode_upper)

  set(target "${ARG_NAME}")

  add_executable(${target})
  target_sources(${target} PRIVATE ${ARG_SOURCES})

  target_link_libraries(${target} PRIVATE PeanoOptions)

  foreach(lib IN LISTS ARG_LINK_LIBRARIES)
    target_link_libraries(${target} PRIVATE ${lib})
  endforeach()

  if(ARG_DIMENSION)
    target_compile_definitions(${target} PRIVATE
      DIMENSIONS=${ARG_DIMENSION}
    )
  endif()

  target_compile_definitions(${target} PRIVATE
    ${PEANO_${mode_upper}_DEFINITIONS}
  )

  target_compile_options(${target} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:${PEANO_CXX_FLAGS_${mode_upper}}>
  )

  if(IPO_SUPPORTED)
    set_target_properties(${target} PROPERTIES
      INTERPROCEDURAL_OPTIMIZATION TRUE
    )
  endif()

  if(AdaptiveCpp_FOUND)
    add_sycl_to_target(
      TARGET ${target}
      SOURCES "$<TARGET_PROPERTY:${target},SOURCES>"
    )
  endif()

  if(WITH_CUDA)
    add_cuda_to_target(${target})
  endif()

  if(WITH_HIP)
    add_hip_to_target(${target})
  endif()

  install(TARGETS ${target}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endfunction()
