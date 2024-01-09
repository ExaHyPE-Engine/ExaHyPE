# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
file(GLOB_RECURSE SOURCES ${FILES_TO_GLOB})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

foreach(mode IN LISTS CMAKE_BUILD_TYPE)
  string(TOLOWER ${mode} mode_lower)
  string(TOUPPER ${mode} mode_upper)

  add_library(PETSc_${mode_lower} STATIC)
  #display_header("Creating Makefile of PETSc_${mode_lower}")

  target_sources(PETSc_${mode_lower} PRIVATE ${SOURCES})
  target_include_directories(PETSc_${mode_lower} PRIVATE ${CMAKE_SOURCE_DIR}/src)
  target_compile_definitions(PETSc_${mode_lower} PUBLIC ${PEANO_${mode_upper}_DEFINITIONS})
  target_link_options(PETSc_${mode_lower} PUBLIC ${PEANO_${mode_upper}_LDFLAGS})
  target_link_libraries(PETSc_${mode_lower} PRIVATE project_warnings)
  target_compile_options(PETSc_${mode_lower} PUBLIC ${PEANO_CXX_FLAGS_${mode_upper}})

  install(TARGETS PETSc_${mode_lower}
    RUNTIME DESTINATION         ${CMAKE_INSTALL_BINDIR}
            COMPONENT           ${PROJECT_NAME}_Runtime
    LIBRARY DESTINATION         ${CMAKE_INSTALL_LIBDIR}
            COMPONENT           ${PROJECT_NAME}_Runtime
            NAMELINK_COMPONENT  ${PROJECT_NAME}_Devel
    ARCHIVE DESTINATION         ${CMAKE_INSTALL_LIBDIR}
            COMPONENT           ${PROJECT_NAME}_Devel
    )
endforeach()
