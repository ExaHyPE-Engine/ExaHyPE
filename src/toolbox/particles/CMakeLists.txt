file(GLOB_RECURSE SOURCES "*")
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

foreach(mode IN LISTS BUILD_MODES)
  string(TOLOWER ${mode} mode_lower)
  string(TOUPPER ${mode} mode_upper)

  foreach(dimension IN LISTS WITH_DIMENSIONS)
    add_library(ToolboxParticles${dimension}d_${mode_lower} STATIC)
    display_header("Creating Makefile of ToolboxParticles${dimension}d_${mode_lower}")

    target_sources(ToolboxParticles${dimension}d_${mode_lower} PRIVATE ${SOURCES})
    target_include_directories(ToolboxParticles${dimension}d_${mode_lower} PRIVATE ${CMAKE_SOURCE_DIR}/src)
    string(REPLACE " " ";" COMPILE_OPTIONS ${CMAKE_CXX_FLAGS_${mode_upper}})
    target_compile_options(ToolboxParticles${dimension}d_${mode_lower} PUBLIC ${COMPILE_OPTIONS})
    target_compile_definitions(ToolboxParticles${dimension}d_${mode_lower} PUBLIC ${${mode_upper}_DEFINITIONS} Dimensions=${dimension})
    target_link_options(ToolboxParticles${dimension}d_${mode_lower} PUBLIC ${${mode_upper}_LDFLAGS})
    target_link_libraries(ToolboxParticles${dimension}d_${mode_lower} PRIVATE project_options project_warnings Peano4Core${dimension}d_${mode_lower})

    install(TARGETS ToolboxParticles${dimension}d_${mode_lower}
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
