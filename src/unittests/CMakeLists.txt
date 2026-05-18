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

    set(test_working_directory "${CMAKE_CURRENT_BINARY_DIR}/${target}")
    file(MAKE_DIRECTORY "${test_working_directory}")

    add_test(NAME ${target} COMMAND ${target})
    set_tests_properties(${target}
      PROPERTIES
        WORKING_DIRECTORY "${test_working_directory}"
    )

    if(WITH_MPI)
      cmake_host_system_information(RESULT logical_cores QUERY NUMBER_OF_LOGICAL_CORES)
      set(mpi_ranks ${logical_cores} ${MPIEXEC_MAX_NUMPROCS} 16)
      list(SORT mpi_ranks COMPARE NATURAL ORDER ASCENDING)
      list(GET mpi_ranks 0 mpi_ranks)

      set(parallel_test_working_directory "${CMAKE_CURRENT_BINARY_DIR}/${target}Parallel")
      file(MAKE_DIRECTORY "${parallel_test_working_directory}")

      add_test(NAME ${target}Parallel
        COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${mpi_ranks} ${CMAKE_BINARY_DIR}/${target}
      )
      set_tests_properties(${target}Parallel
        PROPERTIES
          WORKING_DIRECTORY "${parallel_test_working_directory}"
      )
    endif()
  endforeach()
endforeach()
