# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
file(GLOB_RECURSE SOURCES ${SOURCE_EXTENSIONS_TO_GLOB})
file(GLOB_RECURSE HEADERS ${HEADER_EXTENSIONS_TO_GLOB})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS})

foreach(mode IN LISTS CMAKE_BUILD_TYPE)
  string(TOLOWER ${mode} mode_lower)
  string(TOUPPER ${mode} mode_upper)

  add_library(Tarch_${mode_lower} STATIC)
  #display_header("Creating Makefile of Tarch_${mode_lower}")

  target_sources(Tarch_${mode_lower} PRIVATE ${SOURCES} ${HEADERS})
  target_include_directories(Tarch_${mode_lower} PRIVATE ${CMAKE_SOURCE_DIR}/src)
  target_compile_definitions(Tarch_${mode_lower} PUBLIC ${PEANO_${mode_upper}_DEFINITIONS})
  target_link_options(Tarch_${mode_lower} PUBLIC ${PEANO_${mode_upper}_LDFLAGS})
  target_link_libraries(Tarch_${mode_lower} PUBLIC project_options)
  target_compile_options(Tarch_${mode_lower} PUBLIC $<$<COMPILE_LANGUAGE:CXX>:${PEANO_CXX_FLAGS_${mode_upper}}>)
  target_compile_options(Tarch_${mode_lower} PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:${PEANO_CUDA_FLAGS_${mode_upper}}>)
  target_link_libraries(Tarch_${mode_lower} PRIVATE project_warnings)

  install(TARGETS Tarch_${mode_lower}
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

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/tarch"
)

foreach(pattern IN LISTS HEADER_EXTENSIONS_TO_GLOB)
  install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
    PATTERN ${pattern}
  )
endforeach()
