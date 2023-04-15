file(GLOB_RECURSE SOURCES "*")
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

foreach(mode IN LISTS BUILD_MODES)
  string(TOLOWER ${mode} mode_lower)
  string(TOUPPER ${mode} mode_upper)

  foreach(dimension IN LISTS WITH_DIMENSIONS)
    add_library(ExaHyPE2Core${dimension}d_${mode_lower} STATIC)
    display_header("Creating Makefile of ExaHyPE2Core${dimension}d_${mode_lower}")

    target_sources(ExaHyPE2Core${dimension}d_${mode_lower} PRIVATE ${SOURCES})
    target_include_directories(ExaHyPE2Core${dimension}d_${mode_lower} PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_compile_options(ExaHyPE2Core${dimension}d_${mode_lower} PRIVATE ${ENABLE_NVHPC_CUDA_API})
    target_link_options(ExaHyPE2Core${dimension}d_${mode_lower} PRIVATE ${ENABLE_NVHPC_CUDA_API})
    target_compile_definitions(ExaHyPE2Core${dimension}d_${mode_lower} PUBLIC ${${mode_upper}_DEFINITIONS} Dimensions=${dimension})
    target_link_options(ExaHyPE2Core${dimension}d_${mode_lower} PUBLIC ${${mode_upper}_LDFLAGS})
    target_link_libraries(ExaHyPE2Core${dimension}d_${mode_lower} PUBLIC project_options  Peano4Core${dimension}d_${mode_lower})
    target_link_libraries(ExaHyPE2Core${dimension}d_${mode_lower} PUBLIC ToolboxLoadBalancing${dimension}d_${mode_lower})
    target_link_libraries(ExaHyPE2Core${dimension}d_${mode_lower} PUBLIC ToolboxBlockStructured${dimension}d_${mode_lower})
    target_link_libraries(ExaHyPE2Core${dimension}d_${mode_lower} PRIVATE project_warnings)

    install(TARGETS ExaHyPE2Core${dimension}d_${mode_lower}
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
