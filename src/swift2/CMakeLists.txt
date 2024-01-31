# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
file(GLOB_RECURSE SOURCES ${FILES_TO_GLOB})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

foreach(mode IN LISTS CMAKE_BUILD_TYPE)
  string(TOLOWER ${mode} mode_lower)
  string(TOUPPER ${mode} mode_upper)

  foreach(dimension IN LISTS WITH_DIMENSIONS)
    add_library(SWIFT2Core${dimension}d_${mode_lower} STATIC)
    #display_header("Creating Makefile of SWIFT2Core${dimension}d_${mode_lower}")

    target_sources(SWIFT2Core${dimension}d_${mode_lower} PRIVATE ${SOURCES})
    target_include_directories(SWIFT2Core${dimension}d_${mode_lower} PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_compile_definitions(SWIFT2Core${dimension}d_${mode_lower} PUBLIC ${PEANO_${mode_upper}_DEFINITIONS} Dimensions=${dimension})
    target_link_options(SWIFT2Core${dimension}d_${mode_lower} PUBLIC ${PEANO_${mode_upper}_LDFLAGS})
    target_link_libraries(SWIFT2Core${dimension}d_${mode_lower} PUBLIC project_options Peano4Core${dimension}d_${mode_lower})
    target_link_libraries(SWIFT2Core${dimension}d_${mode_lower} PRIVATE project_warnings)
    target_compile_options(SWIFT2Core${dimension}d_${mode_lower} PUBLIC ${PEANO_CXX_FLAGS_${mode_upper}})

    install(TARGETS SWIFT2Core${dimension}d_${mode_lower}
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
