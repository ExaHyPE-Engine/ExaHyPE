# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

file(GLOB_RECURSE HEADERS ${HEADER_EXTENSIONS_TO_GLOB})

set(SOURCES
  CellData.cpp
  NonNegativeReduction.cpp
  NonNegativeStatistics.cpp
  PrecisionControl.cpp
  RefinementControl.cpp
  RefinementControlService.cpp
  RefinementCriterionHelpers.cpp
  TimeStepping.cpp
  UnitTests.cpp
  tests/NonNegativeReductionTest.cpp
  tests/NonNegativeStatisticsTest.cpp
  tests/RefinementControlServiceTest.cpp
  tests/RefinementControlTest.cpp
  tests/RefinementCriterionHelpersTest.cpp

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
  fv/tests/EigenvalueStatisticsTest.cpp
  fv/tests/FiniteVolumeTest.cpp
  fv/tests/HandleBoundaryTest.cpp
  fv/limiter/SlopeLimiter.cpp
  fv/limiter/tests/SlopeLimiterTest.cpp
  fv/reconstruction/Constant.cpp
  fv/reconstruction/LimitedLinear.cpp
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

install_peano_headers(
  PATTERNS ${HEADER_EXTENSIONS_TO_GLOB}
)

foreach(mode IN LISTS SUPPORTED_BUILD_TYPES)
  foreach(dimensions IN LISTS SUPPORTED_DIMENSIONS)
    peano_add_target(
      TYPE STATIC_LIBRARY
      NAME ExaHyPE2${mode}${dimensions}d
      MODE ${mode}
      DIMENSION ${dimensions}
      SOURCES ${SOURCES}
      HEADERS ${HEADERS}
      LINK_LIBRARIES ToolboxBlockStructured${mode}${dimensions}d ToolboxParticles${mode}${dimensions}d
    )
  endforeach()
endforeach()
