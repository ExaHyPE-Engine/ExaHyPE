file(GLOB_RECURSE SOURCES "*")
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

foreach(mode IN LISTS BUILD_MODES)
  string(TOLOWER ${mode} mode_lower)
  string(TOUPPER ${mode} mode_upper)

  foreach(dimension IN LISTS WITH_DIMENSIONS)
    add_library(ToolboxFiniteElements${dimension}d_${mode_lower} STATIC)
    #display_header("Creating Makefile of ToolboxFiniteElements${dimension}d_${mode_lower}")

    target_sources(ToolboxFiniteElements${dimension}d_${mode_lower} PRIVATE ${SOURCES})
    target_include_directories(ToolboxFiniteElements${dimension}d_${mode_lower} PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_compile_definitions(ToolboxFiniteElements${dimension}d_${mode_lower} PUBLIC ${${mode_upper}_DEFINITIONS} Dimensions=${dimension})
    target_link_options(ToolboxFiniteElements${dimension}d_${mode_lower} PUBLIC ${${mode_upper}_LDFLAGS})
    target_link_libraries(ToolboxFiniteElements${dimension}d_${mode_lower} PUBLIC project_options Peano4Core${dimension}d_${mode_lower})
    target_link_libraries(ToolboxFiniteElements${dimension}d_${mode_lower} PRIVATE project_warnings)

    install(TARGETS ToolboxFiniteElements${dimension}d_${mode_lower}
      RUNTIME DESTINATION         ${CMAKE_INSTALL_BINDIR}
              COMPONENT           ${PROJECT_NAME}_Runtime
      LIBRARY DESTINATION         ${CMAKE_INSTALL_LIBDIR}
              COMPONENT           ${PROJECT_NAME}_Runtime
              NAMELINK_COMPONENT  ${PROJECT_NAME}_Devel
      ARCHIVE DESTINATION         ${CMAKE_INSTALL_LIBDIR}
              COMPONENT           ${PROJECT_NAME}_Devel
    )
  endforeach()
endforeach()
