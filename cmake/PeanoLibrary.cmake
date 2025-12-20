# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

include_guard(GLOBAL)

function(install_peano_headers)
  set(options)
  set(oneValueArgs INSTALL_SUBDIR)
  set(multiValueArgs PATTERNS)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(ARG_INSTALL_SUBDIR)
    set(header_install_dest "${CMAKE_INSTALL_INCLUDEDIR}/${ARG_INSTALL_SUBDIR}")
  else()
    set(header_install_dest "${CMAKE_INSTALL_INCLUDEDIR}")
  endif()

  foreach(pattern IN LISTS ARG_PATTERNS)
    install(
      DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      DESTINATION ${header_install_dest}
      FILES_MATCHING
      PATTERN ${pattern}
    )
  endforeach()
endfunction()

function(add_peano_library)
  set(options)
  set(oneValueArgs NAME MODE DIMENSION)
  set(multiValueArgs
    SOURCES
    HEADERS
    LINK_LIBRARIES
  )

  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  string(TOLOWER ${ARG_MODE} mode_lower)
  string(TOUPPER ${ARG_MODE} mode_upper)

  set(target "${ARG_NAME}")

  add_library(${target} STATIC)
  target_sources(${target} PRIVATE ${ARG_HEADERS} ${ARG_SOURCES})

  source_group(
    TREE ${CMAKE_CURRENT_SOURCE_DIR}
    FILES "$<TARGET_PROPERTY:${target},SOURCES>"
  )

  target_link_libraries(${target} PRIVATE PeanoOptions)

  foreach(lib IN LISTS ARG_LINK_LIBRARIES)
    target_link_libraries(${target} PRIVATE ${lib})
  endforeach()

  if(ARG_DIMENSION)
    target_compile_definitions(${target} PUBLIC
      DIMENSIONS=${ARG_DIMENSION}
    )
  endif()

  target_compile_definitions(${target} PUBLIC
    ${PEANO_${mode_upper}_DEFINITIONS}
  )

  target_compile_options(${target} PUBLIC
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

  install(TARGETS ${target}
    EXPORT ${PROJECT_NAME}Targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
endfunction()
