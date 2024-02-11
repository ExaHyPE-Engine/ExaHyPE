# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
file(GLOB_RECURSE SOURCES ${SOURCES_TO_GLOB})
file(GLOB_RECURSE HEADERS ${HEADERS_TO_GLOB})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS})

foreach(mode IN LISTS CMAKE_BUILD_TYPE)
  string(TOLOWER ${mode} mode_lower)
  string(TOUPPER ${mode} mode_upper)

  foreach(dimension IN LISTS WITH_DIMENSIONS)
    add_library(ToolboxFiniteElements${dimension}d_${mode_lower} STATIC)
    #display_header("Creating Makefile of ToolboxFiniteElements${dimension}d_${mode_lower}")

    target_sources(ToolboxFiniteElements${dimension}d_${mode_lower} PRIVATE ${SOURCES} ${HEADERS})
    target_include_directories(ToolboxFiniteElements${dimension}d_${mode_lower} PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_compile_definitions(ToolboxFiniteElements${dimension}d_${mode_lower} PUBLIC ${PEANO_${mode_upper}_DEFINITIONS} Dimensions=${dimension})
    target_link_options(ToolboxFiniteElements${dimension}d_${mode_lower} PUBLIC ${PEANO_${mode_upper}_LDFLAGS})
    target_link_libraries(ToolboxFiniteElements${dimension}d_${mode_lower} PUBLIC project_options Peano4Core${dimension}d_${mode_lower})
    target_link_libraries(ToolboxFiniteElements${dimension}d_${mode_lower} PRIVATE project_warnings)
    target_compile_options(ToolboxFiniteElements${dimension}d_${mode_lower} PUBLIC ${PEANO_CXX_FLAGS_${mode_upper}})

    if(USE_COMPONENT_DEFINITIONS)
      set(USE_COMPONENT_DEFINITIONS ${USE_COMPONENT_DEFINITIONS} -DUseToolboxFiniteElements)
    endif()

    install(TARGETS ToolboxFiniteElements${dimension}d_${mode_lower}
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

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/toolbox/finiteelements"
)

foreach(pattern IN LISTS HEADERS_TO_GLOB)
  install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
    PATTERN ${pattern}
  )
endforeach()
