# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
foreach(mode IN LISTS PEANO_BUILD_TYPES)
  string(TOLOWER ${mode} mode_lower)
  string(TOUPPER ${mode} mode_upper)

  foreach(dimension IN LISTS PEANO_DIMENSIONS)
    add_executable(UnitTests${mode}${dimension}d)
    add_test(NAME UnitTests${mode}${dimension}d COMMAND UnitTests${mode}${dimension}d)
    if(WITH_MPI)
      add_test(NAME UnitTests${mode}${dimension}dParallel
        COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 2 ${CMAKE_BINARY_DIR}/UnitTests${mode}${dimension}d
      )
    endif()

    target_sources(UnitTests${mode}${dimension}d PRIVATE main.cpp)
    target_link_libraries(UnitTests${mode}${dimension}d PRIVATE PeanoOptions Peano4${mode}${dimension}d)
    target_compile_definitions(UnitTests${mode}${dimension}d PRIVATE Dimensions=${dimension} ${PEANO_${mode_upper}_DEFINITIONS})
    target_compile_options(UnitTests${mode}${dimension}d PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${PEANO_CXX_FLAGS_${mode_upper}}>)

    if(ENABLE_TOOLBOX_PARTICLES)
      target_link_libraries(UnitTests${mode}${dimension}d PRIVATE ToolboxParticles${mode}${dimension}d)
    endif()
    if(ENABLE_TOOLBOX_BLOCKSTRUCTURED)
      target_link_libraries(UnitTests${mode}${dimension}d PRIVATE ToolboxBlockStructured${mode}${dimension}d)
    endif()
    if(ENABLE_TOOLBOX_LOADBALANCING)
      target_link_libraries(UnitTests${mode}${dimension}d PRIVATE ToolboxLoadBalancing${mode}${dimension}d)
    endif()
    if(ENABLE_EXAHYPE)
      target_link_libraries(UnitTests${mode}${dimension}d PRIVATE ExaHyPE2${mode}${dimension}d)
    endif()

    install(TARGETS UnitTests${mode}${dimension}d
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
