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

  foreach(dimension IN LISTS PEANO_DIMENSIONS)
    add_library(ToolboxBlockStructured${mode}${dimension}d STATIC)
    target_sources(ToolboxBlockStructured${mode}${dimension}d PRIVATE ${SOURCES} ${HEADERS})
    target_link_libraries(ToolboxBlockStructured${mode}${dimension}d PRIVATE PeanoOptions Peano4${mode}${dimension}d)
    target_compile_definitions(ToolboxBlockStructured${mode}${dimension}d PUBLIC ${PEANO_${mode_upper}_DEFINITIONS} Dimensions=${dimension})
    target_compile_options(ToolboxBlockStructured${mode}${dimension}d PUBLIC $<$<COMPILE_LANGUAGE:CXX>:${PEANO_CXX_FLAGS_${mode_upper}}>)

    install(TARGETS ToolboxBlockStructured${mode}${dimension}d
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
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/toolbox
    FILES_MATCHING
    PATTERN ${pattern}
  )
endforeach()
