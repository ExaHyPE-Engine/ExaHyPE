# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

cmake_host_system_information(RESULT logical_cores QUERY NUMBER_OF_LOGICAL_CORES)
# A host that reports no core count would let the rank arithmetic below divide by zero.
if(NOT logical_cores OR logical_cores LESS 1)
  set(logical_cores 1)
endif()

set(mpi_ranks 1)
set(best_utilisation 0)
foreach(candidate_ranks 4 3 2)
  if(candidate_ranks GREATER logical_cores)
    continue()
  endif()
  if(MPIEXEC_MAX_NUMPROCS AND candidate_ranks GREATER MPIEXEC_MAX_NUMPROCS)
    continue()
  endif()
  math(EXPR candidate_utilisation "${logical_cores} / ${candidate_ranks} * ${candidate_ranks}")
  if(candidate_utilisation GREATER best_utilisation)
    set(best_utilisation ${candidate_utilisation})
    set(mpi_ranks ${candidate_ranks})
  endif()
endforeach()

math(EXPR threads_per_rank "${logical_cores} / ${mpi_ranks}")
math(EXPR parallel_test_processors "${mpi_ranks} * ${threads_per_rank}")

foreach(mode IN LISTS SUPPORTED_BUILD_TYPES)
  foreach(dimensions IN LISTS SUPPORTED_DIMENSIONS)
    set(target "UnitTests${mode}${dimensions}D")

    set(test_dependencies
      Peano${mode}${dimensions}D
      ExaHyPE${mode}${dimensions}D
      Runner${mode}${dimensions}D
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

    # The serial variant is a single process and may use the whole machine, so it
    # claims every processor. Without that claim ctest counts it as one processor
    # and places further tests beside it, each with a full OpenMP team.
    add_test(NAME ${target} COMMAND $<TARGET_FILE:${target}>)
    set_tests_properties(${target} PROPERTIES
      WORKING_DIRECTORY "${test_working_directory}"
      PROCESSORS ${logical_cores}
      ENVIRONMENT "OMP_NUM_THREADS=${logical_cores}"
    )

    set(parallel_test_working_directory "${CMAKE_CURRENT_BINARY_DIR}/${target}Parallel")
    file(MAKE_DIRECTORY "${parallel_test_working_directory}")

    add_test(NAME ${target}Parallel
      COMMAND
        ${MPIEXEC_EXECUTABLE}
        ${MPIEXEC_NUMPROC_FLAG} ${mpi_ranks}
        --bind-to none
        ${MPIEXEC_PREFLAGS}
        $<TARGET_FILE:${target}>
        ${MPIEXEC_POSTFLAGS}
    )
    set_tests_properties(${target}Parallel PROPERTIES
      WORKING_DIRECTORY "${parallel_test_working_directory}"
      PROCESSORS ${parallel_test_processors}
      ENVIRONMENT "OMP_NUM_THREADS=${threads_per_rank}"
    )
  endforeach()
endforeach()
