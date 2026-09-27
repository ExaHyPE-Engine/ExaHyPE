# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

cmake_host_system_information(RESULT logical_cores QUERY NUMBER_OF_LOGICAL_CORES)
# A host that reports no core count would let the rank arithmetic below divide by zero.
if(NOT logical_cores OR logical_cores LESS 1)
  set(logical_cores 1)
endif()

set(mpi_rank_counts)
set(parities_taken)
foreach(candidate_ranks 4 3 2)
  math(EXPR parity "${candidate_ranks} % 2")
  if(candidate_ranks GREATER logical_cores OR parity IN_LIST parities_taken)
    continue()
  endif()
  if(MPIEXEC_MAX_NUMPROCS AND candidate_ranks GREATER MPIEXEC_MAX_NUMPROCS)
    continue()
  endif()
  list(APPEND mpi_rank_counts ${candidate_ranks})
  list(APPEND parities_taken ${parity})
endforeach()

foreach(mode IN LISTS SUPPORTED_BUILD_TYPES)
  foreach(dimensions IN LISTS SUPPORTED_DIMENSIONS)
    set(target "UnitTests${mode}${dimensions}D")

    set(test_dependencies
      Peano${mode}${dimensions}D
      ExaHyPE${mode}${dimensions}D
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

    add_test(NAME ${target} COMMAND $<TARGET_FILE:${target}>)
    set_tests_properties(${target} PROPERTIES
      WORKING_DIRECTORY "${test_working_directory}"
      PROCESSORS ${logical_cores}
      ENVIRONMENT "OMP_NUM_THREADS=${logical_cores}"
    )

    foreach(mpi_ranks IN LISTS mpi_rank_counts)
      math(EXPR threads_per_rank "${logical_cores} / ${mpi_ranks}")
      math(EXPR mpi_test_processors "${mpi_ranks} * ${threads_per_rank}")
      set(mpi_test "${target}-${mpi_ranks}Ranks")
      set(mpi_test_working_directory "${CMAKE_CURRENT_BINARY_DIR}/${mpi_test}")
      file(MAKE_DIRECTORY "${mpi_test_working_directory}")

      add_test(NAME ${mpi_test}
        COMMAND
          ${MPIEXEC_EXECUTABLE}
          ${MPIEXEC_NUMPROC_FLAG} ${mpi_ranks}
          --bind-to none
          ${MPIEXEC_PREFLAGS}
          $<TARGET_FILE:${target}>
          ${MPIEXEC_POSTFLAGS}
      )
      set_tests_properties(${mpi_test} PROPERTIES
        WORKING_DIRECTORY "${mpi_test_working_directory}"
        PROCESSORS ${mpi_test_processors}
        ENVIRONMENT "OMP_NUM_THREADS=${threads_per_rank}"
      )
    endforeach()
  endforeach()
endforeach()
