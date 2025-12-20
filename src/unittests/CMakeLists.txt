# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

foreach(mode IN LISTS SUPPORTED_BUILD_TYPES)
  string(TOLOWER ${mode} mode_lower)

  foreach(dimensions IN LISTS SUPPORTED_DIMENSIONS)
    set(target "UnitTests${mode}${dimensions}d")

    set(test_dependencies Peano4${mode}${dimensions}d)

    if(ENABLE_TOOLBOX_PARTICLES)
      list(APPEND test_dependencies ToolboxParticles${mode}${dimensions}d)
    endif()

    if(ENABLE_TOOLBOX_BLOCKSTRUCTURED)
      list(APPEND test_dependencies ToolboxBlockStructured${mode}${dimensions}d)
    endif()

    if(ENABLE_TOOLBOX_LOADBALANCING)
      list(APPEND test_dependencies ToolboxLoadBalancing${mode}${dimensions}d)
    endif()

    if(ENABLE_EXAHYPE)
      list(APPEND test_dependencies ExaHyPE2${mode}${dimensions}d)
    endif()

    add_peano_executable(
      NAME            ${target}
      MODE            ${mode}
      DIMENSION       ${dimensions}
      SOURCES         main.cpp
      LINK_LIBRARIES  ${test_dependencies}
    )

    add_test(NAME ${target} COMMAND ${target})

    if(WITH_MPI)
      add_test(NAME ${target}Parallel
        COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS} ${CMAKE_BINARY_DIR}/${target}
      )
    endif()
  endforeach()
endforeach()
