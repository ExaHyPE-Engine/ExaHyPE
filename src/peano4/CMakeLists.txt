# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
file(GLOB_RECURSE SOURCES ${SOURCE_EXTENSIONS_TO_GLOB})
file(GLOB_RECURSE HEADERS ${HEADER_EXTENSIONS_TO_GLOB})
list(FILTER SOURCES EXCLUDE REGEX "\\.template\\.")
list(FILTER HEADERS EXCLUDE REGEX "\\.template\\.")
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS})

foreach(mode IN LISTS PEANO_BUILD_TYPES)
  string(TOLOWER ${mode} mode_lower)
  string(TOUPPER ${mode} mode_upper)

  foreach(dimensions IN LISTS PEANO_DIMENSIONS)
    add_library(Peano4${mode}${dimensions}d STATIC)
    target_sources(Peano4${mode}${dimensions}d PRIVATE ${SOURCES} ${HEADERS})
    target_link_libraries(Peano4${mode}${dimensions}d PRIVATE PeanoOptions Tarch${mode})
    target_compile_definitions(Peano4${mode}${dimensions}d PUBLIC ${PEANO_${mode_upper}_DEFINITIONS} DIMENSIONS=${dimensions})
    target_compile_options(Peano4${mode}${dimensions}d PUBLIC $<$<COMPILE_LANGUAGE:CXX>:${PEANO_CXX_FLAGS_${mode_upper}}>)

    if(${AdaptiveCpp_FOUND})
      add_sycl_to_target(TARGET Peano4${mode}${dimensions}d SOURCES ${SOURCES})
    endif()

    install(TARGETS Peano4${mode}${dimensions}d
      EXPORT                      ${PROJECT_NAME}Targets
      RUNTIME DESTINATION         ${CMAKE_INSTALL_BINDIR}
              COMPONENT           ${PROJECT_NAME}_Runtime
      LIBRARY DESTINATION         ${CMAKE_INSTALL_LIBDIR}
              COMPONENT           ${PROJECT_NAME}_Runtime
              NAMELINK_COMPONENT  ${PROJECT_NAME}_Devel
      ARCHIVE DESTINATION         ${CMAKE_INSTALL_LIBDIR}
              COMPONENT           ${PROJECT_NAME}_Devel
      PUBLIC_HEADER DESTINATION   ${CMAKE_INSTALL_INCLUDEDIR}
      PRIVATE_HEADER DESTINATION  ${CMAKE_INSTALL_INCLUDEDIR}
    )
  endforeach()
endforeach()

foreach(pattern IN LISTS HEADER_EXTENSIONS_TO_GLOB)
  install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
    PATTERN ${pattern}
  )
endforeach()
