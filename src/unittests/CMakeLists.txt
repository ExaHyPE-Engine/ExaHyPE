file(GLOB_RECURSE SOURCES "*")
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

foreach(dimension IN LISTS WITH_DIMENSIONS)
  add_executable(UnitTests${dimension}d)
  #display_header("Creating Makefile of UnitTests${dimension}d")
  add_test(NAME UnitTests${dimension}d COMMAND UnitTests${dimension}d)

  target_sources(UnitTests${dimension}d PRIVATE ${SOURCES})
  target_include_directories(UnitTests${dimension}d PRIVATE ${CMAKE_SOURCE_DIR}/src)
  target_compile_definitions(UnitTests${dimension}d PUBLIC ${DEBUG_DEFINITIONS} Dimensions=${dimension})
  target_link_options(UnitTests${dimension}d PUBLIC ${DEBUG_LDFLAGS})
  target_link_libraries(UnitTests${dimension}d PUBLIC project_options Peano4Core${dimension}d_debug)
  if(ENABLE_EXAHYPE)
    target_link_libraries(UnitTests${dimension}d PUBLIC ExaHyPE2Core${dimension}d_debug)
  endif()
  if(ENABLE_SWIFT)
    target_link_libraries(UnitTests${dimension}d PUBLIC SWIFT2Core${dimension}d_debug)
  endif()
  target_link_libraries(UnitTests${dimension}d PRIVATE project_warnings)

  install(TARGETS UnitTests${dimension}d
    RUNTIME DESTINATION         ${CMAKE_INSTALL_BINDIR}
            COMPONENT           ${PROJECT_NAME}_Runtime
    LIBRARY DESTINATION         ${CMAKE_INSTALL_LIBDIR}
            COMPONENT           ${PROJECT_NAME}_Runtime
            NAMELINK_COMPONENT  ${PROJECT_NAME}_Devel
    ARCHIVE DESTINATION         ${CMAKE_INSTALL_LIBDIR}
            COMPONENT           ${PROJECT_NAME}_Devel
  )
endforeach()
