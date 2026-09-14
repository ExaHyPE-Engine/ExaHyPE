# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

file(GLOB_RECURSE HEADERS ${HEADER_EXTENSIONS_TO_GLOB})

set(SOURCES
  CellData.cpp
  Curvi.cpp
  Enumeration.cpp
  NonNegativeReduction.cpp
  NonNegativeStatistics.cpp
  PrecisionControl.cpp
  ProjectPatch.cpp
  ReconstructPatch.cpp
  RefinementControl.cpp
  RefinementControlService.cpp
  RefinementCriterionHelpers.cpp
  TimeStepping.cpp
  UnitTests.cpp
  tests/CurviTest.cpp
  tests/EnumerationTest.cpp
  tests/NonNegativeReductionTest.cpp
  tests/NonNegativeStatisticsTest.cpp
  tests/PatchKernelsTest.cpp
  tests/RefinementControlServiceTest.cpp
  tests/RefinementControlTest.cpp
  tests/RefinementCriterionHelpersTest.cpp

  amr/AMRRoutines.cpp
  amr/ChildDataStage.cpp
  amr/MultiresolutionDetail.cpp
  amr/tests/ChildDataStageTest.cpp
  amr/tests/InterpolationTest.cpp
  amr/tests/MultiresolutionDetailTest.cpp
  amr/tests/RestrictionTest.cpp

  dg/BoundaryConditions.cpp
  dg/CellIntegral.cpp
  dg/DGUtils.cpp
  dg/EigenvalueStatistics.cpp
  dg/Limiter.cpp
  dg/LimiterStatus.cpp
  dg/LimitingSolverState.cpp
  dg/Riemann.cpp
  dg/SubcellProjection.cpp
  dg/Tracer.cpp
  dg/tests/CellIntegralTest.cpp
  dg/tests/DGUtilsTest.cpp
  dg/tests/EigenvalueStatisticsTest.cpp
  dg/tests/LimiterStatusTest.cpp
  dg/tests/LimiterTest.cpp
  dg/tests/LimitingSolverStateTest.cpp
  dg/tests/RiemannTest.cpp
  dg/tests/SubcellProjectionTest.cpp
  dg/tests/TestUtils.cpp
  dg/rusanov/Rusanov.cpp
  dg/rusanov/tests/RiemannTest.cpp

  fv/BoundaryConditions.cpp
  fv/EigenvalueStatistics.cpp
  fv/FiniteVolume.cpp
  fv/HandleBoundary.cpp
  fv/PatchUtils.cpp
  fv/PostprocessingKernels.cpp
  fv/Tracer.cpp
  fv/tests/ApplySplit1DRiemannToPatchTest.cpp
  fv/tests/CopyPatchTest.cpp
  fv/tests/DeviceKernelFixtures.cpp
  fv/tests/EigenvalueStatisticsTest.cpp
  fv/tests/FiniteVolumeTest.cpp
  fv/tests/HandleBoundaryTest.cpp
  fv/tests/KernelsOnDeviceTest.cpp
  fv/limiter/SlopeLimiter.cpp
  fv/limiter/tests/SlopeLimiterTest.cpp
  fv/reconstruction/Constant.cpp
  fv/reconstruction/InterfaceFluxes.cpp
  fv/reconstruction/LimitedLinear.cpp
  fv/reconstruction/WENO.cpp
  fv/weno/NonlinearWeights.cpp
  fv/weno/tests/NonlinearWeightsTest.cpp
  fv/timestepping/SingleStep.cpp
  fv/particles/ParticleDomainBoundary.cpp
  fv/particles/ParticleForces.cpp
  fv/particles/ParticleHelper.cpp
  fv/particles/ParticleIntegration.cpp
  fv/particles/ParticleIntegrationHelper.cpp
  fv/particles/ParticleInterpolation.cpp
  fv/particles/ParticleProjection.cpp
  fv/particles/ParticleUpdate.cpp
)

if(_peano_gpu_backend STREQUAL "cuda")
  set_source_files_properties(fv/tests/KernelsOnDeviceTest.cpp PROPERTIES LANGUAGE CUDA)
elseif(_peano_gpu_backend STREQUAL "hip")
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
