# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

file(GLOB_RECURSE HEADERS ${HEADER_EXTENSIONS_TO_GLOB})

set(SOURCES
  Curvi.cpp
  ErrorMeasurement.cpp
  RefinementRequests.cpp
  TimeStepping.cpp
  UnitTests.cpp
  tests/CurviTest.cpp
  tests/ErrorMeasurementTest.cpp
  tests/LevelJumpsTest.cpp
  tests/MultiresolutionDetailTest.cpp
  tests/RefinementRequestsTest.cpp
  tests/RefinementVotesTest.cpp
  tests/SolverRepositoryTest.cpp
  tests/TimeSteppingTest.cpp

  fv/tests/FiniteVolumeSchemeTest.cpp
  fv/ader/tests/SolverTest.cpp
  fv/tests/WENOTest.cpp
  fv/tests/HaloProfileTestData.cpp
  fv/tests/KernelsTest.cpp
  fv/tests/SolverTest.cpp
  fv/tests/TracingTest.cpp
  fv/limiter/tests/SlopeLimiterTest.cpp
  fv/weno/tests/NonlinearWeightsTest.cpp

  dg/tests/BasisTest.cpp
  dg/tests/KernelTestData.cpp
  dg/tests/KernelsTest.cpp
  dg/tests/TracingTest.cpp
  dg/tests/TransferTestData.cpp
  dg/ader/tests/CorrectorTest.cpp
  dg/ader/tests/LinearPredictorTest.cpp
  dg/ader/tests/NonlinearPredictorTest.cpp
  dg/ader/tests/PredictorTest.cpp
  dg/ader/tests/SolverTest.cpp
  dg/limiting/tests/DetectorTest.cpp
  dg/limiting/tests/LimiterStatusTest.cpp
  dg/limiting/tests/LimiterTestData.cpp
  dg/limiting/tests/SubcellLimiterTest.cpp
  dg/limiting/tests/SubcellSchemeTest.cpp
  dg/rk/tests/SolverTest.cpp
  dg/rk/tests/StagesTest.cpp

  particles/FileReader.cpp
  particles/TrajectoryDatabase.cpp
  particles/tests/ForcesTest.cpp
  particles/tests/ParticleErrorMeasurementTest.cpp
  particles/tests/ParticleSetTest.cpp
  particles/tests/TrajectoryDatabaseTest.cpp
  particles/tests/TrajectoryRecordingTest.cpp

  plotter/FacePlotter.cpp
  plotter/PatchPlotter.cpp
  runner/Console.cpp
  runner/Runner.cpp
  runner/RuntimeParameters.cpp
  runner/Statistics.cpp
  runner/tests/ConsoleTest.cpp
  runner/tests/RunnerTest.cpp
  runner/tests/RuntimeParametersTest.cpp
  runner/tests/StatisticsTest.cpp
  plotter/tests/FacePlotterTest.cpp
  plotter/tests/ParticlePlotterTest.cpp
  plotter/tests/PatchPlotterTest.cpp

  terms/tests/TermsTest.cpp
)

set(GPU_SOURCES
  ErrorMeasurement.cpp
  fv/ader/tests/SolverTest.cpp
  fv/tests/WENOTest.cpp
  dg/ader/tests/LinearPredictorTest.cpp
  dg/ader/tests/NonlinearPredictorTest.cpp
  dg/ader/tests/SolverTest.cpp
  dg/limiting/tests/DetectorTest.cpp
  dg/limiting/tests/SubcellLimiterTest.cpp
  dg/limiting/tests/SubcellSchemeTest.cpp
  dg/rk/tests/SolverTest.cpp
  dg/rk/tests/StagesTest.cpp
  dg/tests/KernelsTest.cpp
  dg/tests/TracingTest.cpp
  fv/tests/FiniteVolumeSchemeTest.cpp
  fv/tests/SolverTest.cpp
  fv/tests/TracingTest.cpp
  particles/tests/ParticleErrorMeasurementTest.cpp
  particles/tests/ParticleSetTest.cpp
  particles/tests/TrajectoryRecordingTest.cpp
  plotter/tests/ParticlePlotterTest.cpp
  tests/ErrorMeasurementTest.cpp
  tests/LevelJumpsTest.cpp
  tests/MultiresolutionDetailTest.cpp
  tests/RefinementVotesTest.cpp
)
if(_peano_gpu_backend STREQUAL "cuda")
  set_source_files_properties(${GPU_SOURCES} PROPERTIES LANGUAGE CUDA)
elseif(_peano_gpu_backend STREQUAL "hip")
  set_source_files_properties(${GPU_SOURCES} PROPERTIES LANGUAGE HIP)
endif()

install_peano_headers(
  PATTERNS ${HEADER_EXTENSIONS_TO_GLOB}
)

foreach(mode IN LISTS SUPPORTED_BUILD_TYPES)
  foreach(dimensions IN LISTS SUPPORTED_DIMENSIONS)
    peano_add_target(
      TYPE STATIC_LIBRARY
      NAME ExaHyPE${mode}${dimensions}D
      MODE ${mode}
      DIMENSION ${dimensions}
      SOURCES ${SOURCES}
      HEADERS ${HEADERS}
      LINK_LIBRARIES Peano${mode}${dimensions}D
    )
  endforeach()
endforeach()
