file(GLOB_RECURSE TARCH_SOURCES "*")
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

foreach(mode IN LISTS BUILD_MODES)
  string(TOLOWER ${mode} mode_lower)
  add_library(Tarch_${mode_lower} STATIC)
  display_header("Creating Makefile of Tarch_${mode_lower}")
  target_sources(Tarch_${mode_lower} PRIVATE ${TARCH_SOURCES})
  target_include_directories(Tarch_${mode_lower} PRIVATE ${CMAKE_SOURCE_DIR}/src)
  #target_link_directories(Tarch${SUFFIX_${mode}} PRIVATE ${CMAKE_SOURCE_DIR}/src)
  #target_compile_options(Tarch${SUFFIX_${mode}} PUBLIC ${${mode_upper}_FLAGS})
  #target_compile_definitions(Tarch${SUFFIX_${mode}} PUBLIC ${${mode_upper}_DEFINITIONS})
  #target_link_options(Tarch${SUFFIX_${mode}} PUBLIC ${${mode_upper}_LDFLAGS})
  target_link_libraries(Tarch_${mode_lower}
  PUBLIC
      project_options
      project_warnings
  )

  # Some Toolchains of Peano do not work (even for autoconf) therefore the current toolchain options are disabled for CMake
  # if(WITH_TOOLCHAIN STREQUAL "scorep")
  # # TODO: Fix logging libraries
  # find_library(PAPI libpapi.a papi)
  # find_library(CUBE4W libcube4w.a cube4w)
  # target_link_libraries(Tarch${SUFFIX_${mode}} PRIVATE ${PAPI})
  # target_link_libraries(Tarch${SUFFIX_${mode}} PRIVATE ${CUBE4W})
  # target_link_directories(Tarch${SUFFIX_${mode}} PRIVATE ${LD_LIBRARY_PATH})
  # target_link_libraries(Tarch${SUFFIX_${mode}} PRIVATE ${SCOREP_LIBRARIES})
  # target_compile_options(Tarch${SUFFIX_${mode}} PRIVATE ${SCOREP_COMPILE_FLAGS})
  # target_link_options(Tarch${SUFFIX_${mode}} PRIVATE ${SCOREP_LINK_OPTION})
  # elseif(WITH_TOOLCHAIN STREQUAL "intel")
  # target_link_libraries(Tarch${SUFFIX_${mode}} INTERFACE VT::VT)
  # target_include_directories(Tarch${SUFFIX_${mode}} SYSTEM PRIVATE ${VT_INCLUDE_DIRS})
  # target_link_directories(Tarch${SUFFIX_${mode}} INTERFACE ${VT_LIBRARIES_DIR})
  # endif()

  #if(WITH_MULTITHREADING STREQUAL "omp")
  #  if(WITH_GPU STREQUAL "omp")
  #    target_link_libraries(Tarch${SUFFIX_${mode}} PUBLIC OpenMP::OpenMP_CXX_with_offloading)
  #  else()
  #    target_link_libraries(Tarch${SUFFIX_${mode}} PUBLIC OpenMP::OpenMP_CXX)
  #  endif()
  #elseif(WITH_MULTITHREADING STREQUAL "cpp")
  #  target_link_libraries(Tarch${SUFFIX_${mode}} PUBLIC Threads::Threads)
  #elseif(WITH_MULTITHREADING STREQUAL "tbb")
  #  target_link_libraries(Tarch${SUFFIX_${mode}} PUBLIC TBB::tbb)
  #endif()

  #if(WITH_MPI)
  #  target_link_libraries(Tarch${SUFFIX_${mode}} PUBLIC MPI::MPI_CXX)
  #endif()

  #if(WITH_OTTER)
  #  target_link_libraries(Tarch${SUFFIX_${mode}} PUBLIC Otter::Otter)
  #endif()
endforeach()
