# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

cmake_host_system_information(RESULT logical_cores QUERY NUMBER_OF_LOGICAL_CORES)

set(mpi_ranks 4)
if(logical_cores LESS mpi_ranks)
  set(mpi_ranks ${logical_cores})
endif()
if(MPIEXEC_MAX_NUMPROCS AND MPIEXEC_MAX_NUMPROCS LESS mpi_ranks)
  set(mpi_ranks ${MPIEXEC_MAX_NUMPROCS})
endif()

math(EXPR threads_per_rank "${logical_cores} / ${mpi_ranks}")
math(EXPR parallel_test_processors "${mpi_ranks} * ${threads_per_rank}")

foreach(mode IN LISTS SUPPORTED_BUILD_TYPES)
  foreach(dimensions IN LISTS SUPPORTED_DIMENSIONS)
    set(target "UnitTests${mode}${dimensions}d")

    set(test_dependencies
      Peano4${mode}${dimensions}d
      ToolboxParticles${mode}${dimensions}d
      ToolboxBlockStructured${mode}${dimensions}d
      ToolboxLoadBalancing${mode}${dimensions}d
      ExaHyPE2${mode}${dimensions}d
      Runner${mode}${dimensions}d
    )

    peano_add_target(
      TYPE            EXECUTABLE
      NAME            ${target}
      MODE            ${mode}
      DIMENSION       ${dimensions}
      SOURCES         Main.cpp
      LINK_LIBRARIES  ${test_dependencies}
    )

    set(test_working_directory "${CMAKE_CURRENT_BINARY_DIR}/${target}")
    file(MAKE_DIRECTORY "${test_working_directory}")

    add_test(NAME ${target} COMMAND ${target})
    set_tests_properties(${target} PROPERTIES WORKING_DIRECTORY "${test_working_directory}")

    set(parallel_test_working_directory "${CMAKE_CURRENT_BINARY_DIR}/${target}Parallel")
    file(MAKE_DIRECTORY "${parallel_test_working_directory}")

    # Keep the MPI variants hybrid without oversubscribing the test
    # node with one full OpenMP team per rank. Passive waiting also leaves a
    # core available to the MPI progress engine during blocking collectives.
    add_test(NAME ${target}Parallel
      COMMAND
        ${MPIEXEC_EXECUTABLE}
        ${MPIEXEC_NUMPROC_FLAG} ${mpi_ranks}
        ${MPIEXEC_PREFLAGS}
        ${CMAKE_BINARY_DIR}/${target}
        ${MPIEXEC_POSTFLAGS}
    )
    set_tests_properties(${target}Parallel PROPERTIES
      WORKING_DIRECTORY "${parallel_test_working_directory}"
      PROCESSORS ${parallel_test_processors}
      ENVIRONMENT
        "OMP_NUM_THREADS=${threads_per_rank};OMP_WAIT_POLICY=passive;OMPI_MCA_hwloc_base_binding_policy=none;OMPI_MCA_mca_base_component_show_load_errors=none"
    )
  endforeach()

endforeach()
