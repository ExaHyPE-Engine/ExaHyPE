# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

file(GLOB_RECURSE HEADERS ${HEADER_EXTENSIONS_TO_GLOB})

set(SOURCES
  AdaptiveTimeStepping.cpp
  Curvi.cpp
  ErrorMeasurement.cpp
  FixedTimeStepping.cpp
  RefinementControl.cpp
  RefinementControlService.cpp
  Solver.cpp
  UnitTests.cpp
  tests/AdaptiveTimeSteppingTest.cpp
  tests/CurviTest.cpp
  tests/EnumerationTest.cpp
  tests/ErrorMeasurementTest.cpp
  tests/FaceMergesTest.cpp
  tests/FixedTimeSteppingTest.cpp
  tests/NonNegativeReductionTest.cpp
  tests/NonNegativeStatisticsTest.cpp
  tests/ParticleErrorMeasurementTest.cpp
  tests/ProjectPatchTest.cpp
  tests/RefinementControlServiceTest.cpp
  tests/RefinementControlTest.cpp
  tests/RefinementCriterionHelpersTest.cpp
  tests/SolverConceptsTest.cpp
  tests/SolverRepositoryTest.cpp
  tests/SolverTest.cpp
  tests/TracingTest.cpp

  amr/LevelJumpEvents.cpp
  amr/tests/ChildDataStageTest.cpp
  amr/tests/InterpolationTest.cpp
  amr/tests/LevelJumpEventsTest.cpp
  amr/tests/MultiresolutionDetailTest.cpp
  amr/tests/RestrictionTest.cpp

  dg/LimitingSolverState.cpp
  dg/tests/AMRMatricesTest.cpp
  dg/tests/BoundaryConditionsTest.cpp
  dg/tests/BoundaryExtrapolationTest.cpp
  dg/tests/DGTracerTest.cpp
  dg/tests/DGUtilsTest.cpp
  dg/tests/DeviceKernelFixtures.cpp
  dg/tests/EigenvalueStatisticsTest.cpp
  dg/tests/FaceIntegralTest.cpp
  dg/tests/ForEachNodeTest.cpp
  dg/tests/GradientTest.cpp
  dg/tests/KernelsOnDeviceTest.cpp
  dg/tests/LimiterStatusTest.cpp
  dg/tests/LimiterTest.cpp
  dg/tests/LimitingSolverStateTest.cpp
  dg/tests/OperatorAlongAxisTest.cpp
  dg/tests/RiemannTest.cpp
  dg/tests/SubcellProjectionTest.cpp
  dg/tests/TransferKernelsTest.cpp
  dg/limiting/tests/AdmissibilityTest.cpp
  dg/limiting/tests/CorrectAndFlagTest.cpp
  dg/limiting/tests/DynamicAMRTest.cpp
  dg/limiting/tests/FaceStatusTest.cpp
  dg/limiting/tests/ForEachSubcellTest.cpp
  dg/limiting/tests/InitialiseSubcellsTest.cpp
  dg/limiting/tests/PredictSubcellsTest.cpp
  dg/limiting/tests/RecomputeSubcellsTest.cpp
  dg/limiting/tests/SpreadDomainTest.cpp
  dg/limiting/tests/SubcellLayoutTest.cpp
  dg/limiting/tests/VertexExtremaTest.cpp
  dg/rk/tests/DynamicAMRTest.cpp
  dg/rk/tests/InterpolateRiemannSolutionTest.cpp
  dg/rk/tests/KernelsOnDeviceTest.cpp
  dg/rk/tests/LinearCombinationTest.cpp
  dg/rk/tests/StageScratchTest.cpp
  dg/rk/tests/SolverTest.cpp
  dg/rk/tests/StageFixtures.cpp
  dg/rk/tests/UpdateStageTest.cpp
  dg/rk/tests/VolumeIntegralTest.cpp
  dg/ader/tests/CauchyKovalewskiTest.cpp
  dg/ader/tests/CorrectTest.cpp
  dg/ader/tests/DeviceKernelFixtures.cpp
  dg/ader/tests/DynamicAMRTest.cpp
  dg/ader/tests/KernelsOnDeviceTest.cpp
  dg/ader/tests/LinearPredictorTermsTest.cpp
  dg/ader/tests/PicardIterationTest.cpp
  dg/ader/tests/PointSourceContributionTest.cpp
  dg/ader/tests/PointSourcesTest.cpp
  dg/ader/tests/PredictTest.cpp
  dg/ader/tests/PredictorScratchTest.cpp
  dg/ader/tests/SolverTest.cpp
  dg/ader/tests/SpaceTimePredictorTest.cpp
  dg/ader/tests/SpaceTimeTermsTest.cpp
  dg/ader/tests/TimeAveragingTest.cpp
  dg/ader/tests/VolumeIntegralTest.cpp

  fv/PatchUtils.cpp
  fv/tests/ApplyBoundaryConditionsOnFaceTest.cpp
  fv/tests/ApplyBoundaryConditionsTest.cpp
  fv/tests/ApplySplit1DRiemannToPatchTest.cpp
  fv/tests/CopyPatchTest.cpp
  fv/tests/DeviceKernelFixtures.cpp
  fv/tests/DynamicAMRTest.cpp
  fv/tests/EigenvalueStatisticsTest.cpp
  fv/tests/FiniteVolumeTest.cpp
  fv/tests/ForEachVolumeTest.cpp
  fv/tests/KernelsOnDeviceTest.cpp
  fv/tests/ProjectPatchOntoFacesTest.cpp
  fv/tests/RollOverUpdatesTest.cpp
  fv/tests/SolverTest.cpp
  fv/tests/UpdatePatchTest.cpp
  fv/limiter/tests/SlopeLimiterTest.cpp
  fv/weno/tests/NonlinearWeightsTest.cpp
  fv/particles/ParticleForces.cpp
  fv/particles/ParticleHelper.cpp
  fv/particles/ParticleInterpolation.cpp

  particles/tests/CoordinateOwnershipTest.cpp
  particles/tests/CurviTracerTest.cpp
  particles/tests/GridAssociationTest.cpp
  particles/tests/InsertParticlesByCoordinatesAndDataTest.cpp
  particles/tests/InsertParticlesByCoordinatesTest.cpp
  particles/tests/InsertParticlesFromFileTest.cpp
  particles/tests/InteractionSetsTest.cpp
  particles/tests/ParallelStateTest.cpp
  particles/tests/TestParticleSet.cpp
  particles/tests/TrajectoryRecordingTest.cpp

  plotter/FacePlotter.cpp
  plotter/ParticlePlotter.cpp
  plotter/PatchPlotter.cpp
  plotter/Piece.cpp
  plotter/Snapshot.cpp
  plotter/tests/FacePlotterTest.cpp
  plotter/tests/ParticlePlotterTest.cpp
  plotter/tests/PatchPlotterTest.cpp
  plotter/tests/PieceTest.cpp
  plotter/tests/RecordingFileWriter.cpp
  plotter/tests/SnapshotTest.cpp
)

if(_peano_gpu_backend STREQUAL "cuda")
  set_source_files_properties(dg/tests/KernelsOnDeviceTest.cpp PROPERTIES LANGUAGE CUDA)
  set_source_files_properties(dg/rk/tests/KernelsOnDeviceTest.cpp PROPERTIES LANGUAGE CUDA)
  set_source_files_properties(dg/ader/tests/KernelsOnDeviceTest.cpp PROPERTIES LANGUAGE CUDA)
  set_source_files_properties(fv/tests/KernelsOnDeviceTest.cpp PROPERTIES LANGUAGE CUDA)
elseif(_peano_gpu_backend STREQUAL "hip")
  set_source_files_properties(dg/tests/KernelsOnDeviceTest.cpp PROPERTIES LANGUAGE HIP)
  set_source_files_properties(dg/rk/tests/KernelsOnDeviceTest.cpp PROPERTIES LANGUAGE HIP)
  set_source_files_properties(dg/ader/tests/KernelsOnDeviceTest.cpp PROPERTIES LANGUAGE HIP)
  set_source_files_properties(fv/tests/KernelsOnDeviceTest.cpp PROPERTIES LANGUAGE HIP)
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
