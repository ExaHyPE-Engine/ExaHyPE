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
      cmake_host_system_information(RESULT UNITTEST_LOGICAL_CORES QUERY NUMBER_OF_LOGICAL_CORES)
      set(UNITTEST_MPI_RANKS ${UNITTEST_LOGICAL_CORES} ${MPIEXEC_MAX_NUMPROCS} 16)
      list(SORT UNITTEST_MPI_RANKS COMPARE NATURAL ORDER ASCENDING)
      list(GET UNITTEST_MPI_RANKS 0 UNITTEST_MPI_RANKS)
      add_test(NAME ${target}Parallel
        COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${UNITTEST_MPI_RANKS} ${CMAKE_BINARY_DIR}/${target}
      )
    endif()
  endforeach()
endforeach()
