# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
foreach(mode IN LISTS PEANO_BUILD_TYPES)
  string(TOLOWER ${mode} mode_lower)
  string(TOUPPER ${mode} mode_upper)

  foreach(dimension IN LISTS PEANO_DIMENSIONS)
    add_executable(UnitTests${dimension}d_${mode_lower})
    add_test(NAME UnitTests${dimension}d_${mode_lower} COMMAND UnitTests${dimension}d_${mode_lower})

    target_sources(UnitTests${dimension}d_${mode_lower} PRIVATE main.cpp)
    target_link_libraries(UnitTests${dimension}d_${mode_lower} PRIVATE PeanoOptions)
    target_compile_definitions(UnitTests${dimension}d_${mode_lower} PRIVATE Dimensions=${dimension} ${PEANO_${mode_upper}_DEFINITIONS})
    target_compile_options(UnitTests${dimension}d_${mode_lower} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${PEANO_CXX_FLAGS_${mode_upper}}>)

    target_link_libraries(UnitTests${dimension}d_${mode_lower} PRIVATE Peano4Core${dimension}d_${mode_lower})
    if(ENABLE_PARTICLES)
      target_link_libraries(UnitTests${dimension}d_${mode_lower} PRIVATE ToolboxParticles${dimension}d_${mode_lower})
    endif()
    if(ENABLE_FINITEELEMENTS)
      target_link_libraries(UnitTests${dimension}d_${mode_lower} PRIVATE ToolboxFiniteElements${dimension}d_${mode_lower})
    endif()
    if(ENABLE_BLOCKSTRUCTURED)
      target_link_libraries(UnitTests${dimension}d_${mode_lower} PRIVATE ToolboxBlockStructured${dimension}d_${mode_lower})
    endif()
    if(ENABLE_LOADBALANCING)
      target_link_libraries(UnitTests${dimension}d_${mode_lower} PRIVATE ToolboxLoadBalancing${dimension}d_${mode_lower})
    endif()
    if(ENABLE_EXAHYPE)
      target_link_libraries(UnitTests${dimension}d_${mode_lower} PRIVATE ExaHyPE2Core${dimension}d_${mode_lower})
    endif()
    if(ENABLE_MGHYPE)
      target_link_libraries(UnitTests${dimension}d_${mode_lower} PRIVATE Multigrid${dimension}d_${mode_lower})
    endif()
    if(ENABLE_SWIFT)
      target_link_libraries(UnitTests${dimension}d_${mode_lower} PRIVATE SWIFT2Core${dimension}d_${mode_lower})
    endif()

    install(TARGETS UnitTests${dimension}d_${mode_lower}
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
