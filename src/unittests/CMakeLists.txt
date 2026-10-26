# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

set(TARCH_TEST_SOURCES
  tarch/AssertionsTest.cpp
  tarch/CPUFeatureDetectionTest.cpp
  tarch/MemoryUsageTest.cpp
  tarch/NonCriticalAssertionsTest.cpp
  tarch/SpanTest.cpp

  tarch/accelerator/BufferTest.cpp
  tarch/accelerator/ChunkedMemoryPoolTest.cpp
  tarch/accelerator/DeviceCopiesTest.cpp
  tarch/accelerator/DeviceCopiesTestData.cpp
  tarch/accelerator/MemoryAllocationTest.cpp
  tarch/accelerator/ParallelConstructsTest.cpp
  tarch/accelerator/ScratchTest.cpp

  tarch/database/GlobalTimeSeriesDatabaseTest.cpp

  tarch/instrumentation/CommandLineSinkTest.cpp
  tarch/instrumentation/WatchTest.cpp

  tarch/la/FixedPointSumTest.cpp
  tarch/la/MatrixTest.cpp
  tarch/la/MatrixVectorTest.cpp
  tarch/la/ScalarTest.cpp
  tarch/la/VectorTest.cpp

  tarch/mpi/CommunicationManagerTest.cpp
  tarch/mpi/DiagnosticsTest.cpp
  tarch/mpi/HandlesTest.cpp
  tarch/mpi/HelperThreadTest.cpp
  tarch/mpi/PingPongTest.cpp
  tarch/mpi/PollPacingTest.cpp
  tarch/mpi/RankTest.cpp
  tarch/mpi/ReductionRecordTest.cpp

  tarch/plotter/FileWriterTest.cpp
  tarch/plotter/FormatTest.cpp
  tarch/plotter/HDF5FileWriterTest.cpp
  tarch/plotter/Meshes.cpp
  tarch/plotter/NetCDFFileWriterTest.cpp
  tarch/plotter/VTKHDFFileWriterTest.cpp
  tarch/plotter/VTUFileWriterTest.cpp

  tarch/reader/CSVReaderTest.cpp
  tarch/reader/JSONReaderTest.cpp
  tarch/reader/NetCDFFieldSamplerTest.cpp
  tarch/reader/NetCDFFileReaderTest.cpp

  tarch/tasking/RuntimeTest.cpp
  tarch/tasking/TaskGraphTest.cpp

  tarch/testing/UnitTestTest.cpp
)

set(PEANO4_TEST_SOURCES
  peano4/CellConnectivityTest.cpp
  peano4/HaloExchangeTest.cpp
  peano4/LoopTest.cpp
  peano4/MigrationExchangeTest.cpp
  peano4/RefinementRequestTest.cpp
  peano4/SpacetreeTest.cpp
  peano4/TestGrids.cpp

  peano4/communication/CommunicationManagerTest.cpp
  peano4/communication/HaloExchangeKernelsTest.cpp
  peano4/communication/MigrationExchangeKernelsTest.cpp

  peano4/grid/AutomatonStateTest.cpp
  peano4/grid/GridVertexTest.cpp
  peano4/grid/LatticeIndexMapTest.cpp

  peano4/sfc/GrammarTest.cpp
  peano4/sfc/SpaceFillingCurveTest.cpp

  peano4/topology/ChunkedTasksTest.cpp
  peano4/topology/TreeTest.cpp
)

set(EXAHYPE2_TEST_SOURCES
  exahype2/CurviTest.cpp
  exahype2/ErrorMeasurementTest.cpp
  exahype2/LevelJumpsTest.cpp
  exahype2/MultiresolutionDetailTest.cpp
  exahype2/RefinementRequestsTest.cpp
  exahype2/RefinementVotesTest.cpp
  exahype2/SolverRepositoryTest.cpp
  exahype2/TimeSteppingTest.cpp

  exahype2/dg/BasisTest.cpp
  exahype2/dg/KernelTestData.cpp
  exahype2/dg/KernelsTest.cpp
  exahype2/dg/TracingTest.cpp
  exahype2/dg/TransferTestData.cpp

  exahype2/dg/ader/CorrectorTest.cpp
  exahype2/dg/ader/LinearPredictorTest.cpp
  exahype2/dg/ader/NonlinearPredictorTest.cpp
  exahype2/dg/ader/PredictorTest.cpp
  exahype2/dg/ader/SolverTest.cpp

  exahype2/dg/limiting/DetectorTest.cpp
  exahype2/dg/limiting/LimiterStatusTest.cpp
  exahype2/dg/limiting/LimiterTestData.cpp
  exahype2/dg/limiting/SubcellLimiterTest.cpp
  exahype2/dg/limiting/SubcellSchemeTest.cpp

  exahype2/dg/rk/SolverTest.cpp
  exahype2/dg/rk/StagesTest.cpp

  exahype2/fv/FiniteVolumeSchemeTest.cpp
  exahype2/fv/HaloProfileTestData.cpp
  exahype2/fv/KernelsTest.cpp
  exahype2/fv/SolverTest.cpp
  exahype2/fv/TracingTest.cpp

  exahype2/fv/limiter/SlopeLimiterTest.cpp

  exahype2/particles/ForcesTest.cpp
  exahype2/particles/ParticleErrorMeasurementTest.cpp
  exahype2/particles/ParticleSetTest.cpp
  exahype2/particles/TrajectoryDatabaseTest.cpp
  exahype2/particles/TrajectoryRecordingTest.cpp

  exahype2/plotter/FacePlotterTest.cpp
  exahype2/plotter/ParticlePlotterTest.cpp
  exahype2/plotter/PatchPlotterTest.cpp

  exahype2/runner/ConsoleTest.cpp
  exahype2/runner/RunnerTest.cpp
  exahype2/runner/RuntimeParametersTest.cpp
  exahype2/runner/StatisticsTest.cpp

  exahype2/terms/TermsTest.cpp
)

set(GPU_TEST_SOURCES
  tarch/accelerator/BufferTest.cpp
  tarch/accelerator/ChunkedMemoryPoolTest.cpp
  tarch/accelerator/DeviceCopiesTest.cpp
  tarch/accelerator/DeviceCopiesTestData.cpp
  tarch/accelerator/ParallelConstructsTest.cpp
  tarch/accelerator/ScratchTest.cpp
  tarch/la/FixedPointSumTest.cpp

  peano4/CellConnectivityTest.cpp
  peano4/HaloExchangeTest.cpp
  peano4/MigrationExchangeTest.cpp
  peano4/communication/HaloExchangeKernelsTest.cpp
  peano4/communication/MigrationExchangeKernelsTest.cpp

  exahype2/ErrorMeasurementTest.cpp
  exahype2/LevelJumpsTest.cpp
  exahype2/MultiresolutionDetailTest.cpp
  exahype2/RefinementVotesTest.cpp
  exahype2/dg/KernelsTest.cpp
  exahype2/dg/TracingTest.cpp
  exahype2/dg/ader/LinearPredictorTest.cpp
  exahype2/dg/ader/NonlinearPredictorTest.cpp
  exahype2/dg/ader/SolverTest.cpp
  exahype2/dg/limiting/DetectorTest.cpp
  exahype2/dg/limiting/SubcellLimiterTest.cpp
  exahype2/dg/limiting/SubcellSchemeTest.cpp
  exahype2/dg/rk/SolverTest.cpp
  exahype2/dg/rk/StagesTest.cpp
  exahype2/fv/FiniteVolumeSchemeTest.cpp
  exahype2/fv/SolverTest.cpp
  exahype2/fv/TracingTest.cpp
  exahype2/particles/ParticleErrorMeasurementTest.cpp
  exahype2/particles/ParticleSetTest.cpp
  exahype2/particles/TrajectoryRecordingTest.cpp
  exahype2/plotter/ParticlePlotterTest.cpp
)
if(_peano_gpu_backend STREQUAL "cuda")
  list(APPEND TARCH_TEST_SOURCES tarch/accelerator/cuda/ReductionTest.cpp)
  set_source_files_properties(tarch/accelerator/cuda/ReductionTest.cpp ${GPU_TEST_SOURCES} PROPERTIES LANGUAGE CUDA)
elseif(_peano_gpu_backend STREQUAL "hip")
  list(APPEND TARCH_TEST_SOURCES tarch/accelerator/hip/ReductionTest.cpp)
  set_source_files_properties(tarch/accelerator/hip/ReductionTest.cpp ${GPU_TEST_SOURCES} PROPERTIES LANGUAGE HIP)
elseif(_peano_gpu_backend STREQUAL "sycl")
  list(APPEND TARCH_TEST_SOURCES tarch/accelerator/sycl/ReductionTest.cpp)
elseif(_peano_gpu_backend STREQUAL "kokkos")
  list(APPEND TARCH_TEST_SOURCES tarch/accelerator/kokkos/ReductionTest.cpp)
elseif(_peano_gpu_backend STREQUAL "ompt")
  list(APPEND TARCH_TEST_SOURCES tarch/accelerator/ompt/ReductionTest.cpp)
endif()

cmake_host_system_information(RESULT logical_cores QUERY NUMBER_OF_LOGICAL_CORES)
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
  peano_add_target(
    TYPE            OBJECT_LIBRARY
    NAME            TarchUnitTests${mode}
    MODE            ${mode}
    SOURCES         ${TARCH_TEST_SOURCES}
    LINK_LIBRARIES  Tarch${mode}
    NO_INSTALL
  )

  foreach(dimensions IN LISTS SUPPORTED_DIMENSIONS)
    set(target "UnitTests${mode}${dimensions}D")

    peano_add_target(
      TYPE            EXECUTABLE
      NAME            ${target}
      MODE            ${mode}
      DIMENSION       ${dimensions}
      SOURCES         Main.cpp ${PEANO4_TEST_SOURCES} ${EXAHYPE2_TEST_SOURCES}
      LINK_LIBRARIES  TarchUnitTests${mode} ExaHyPE${mode}${dimensions}D
      NO_INSTALL
    )

    foreach(library IN ITEMS tarch peano4 exahype2)
      set(test "${target}-${library}")
      set(test_working_directory "${CMAKE_CURRENT_BINARY_DIR}/${test}")
      file(MAKE_DIRECTORY "${test_working_directory}")

      add_test(NAME ${test} COMMAND $<TARGET_FILE:${target}> --filter "${library}/*" --junit junit.xml)
      set_tests_properties(${test} PROPERTIES
        WORKING_DIRECTORY "${test_working_directory}"
        PROCESSORS ${logical_cores}
        ENVIRONMENT "OMP_NUM_THREADS=${logical_cores}"
      )

      foreach(mpi_ranks IN LISTS mpi_rank_counts)
        math(EXPR threads_per_rank "${logical_cores} / ${mpi_ranks}")
        math(EXPR mpi_test_processors "${mpi_ranks} * ${threads_per_rank}")
        set(mpi_test "${test}-${mpi_ranks}Ranks")
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
            --filter "${library}/*"
            --junit junit.xml
        )
        set_tests_properties(${mpi_test} PROPERTIES
          WORKING_DIRECTORY "${mpi_test_working_directory}"
          PROCESSORS ${mpi_test_processors}
          ENVIRONMENT "OMP_NUM_THREADS=${threads_per_rank}"
          LABELS mpi
        )
      endforeach()
    endforeach()
  endforeach()
endforeach()
