# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
file(GLOB_RECURSE SOURCES ${SOURCE_EXTENSIONS_TO_GLOB})
file(GLOB_RECURSE HEADERS ${HEADER_EXTENSIONS_TO_GLOB})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS})

foreach(mode IN LISTS CMAKE_BUILD_TYPE)
  string(TOLOWER ${mode} mode_lower)
  string(TOUPPER ${mode} mode_upper)

  foreach(dimension IN LISTS WITH_DIMENSIONS)
    add_library(Peano4Core${dimension}d_${mode_lower} STATIC)
    target_sources(Peano4Core${dimension}d_${mode_lower} PRIVATE ${SOURCES} ${HEADERS})
    target_link_libraries(Peano4Core${dimension}d_${mode_lower} PRIVATE ProjectOptions Tarch_${mode_lower})
    target_compile_definitions(Peano4Core${dimension}d_${mode_lower} PRIVATE ${PEANO_${mode_upper}_DEFINITIONS} Dimensions=${dimension})
    target_compile_options(Peano4Core${dimension}d_${mode_lower} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${PEANO_CXX_FLAGS_${mode_upper}}>)

    install(TARGETS Peano4Core${dimension}d_${mode_lower}
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
