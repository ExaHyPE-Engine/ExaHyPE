# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

file(GLOB_RECURSE HEADERS ${HEADER_EXTENSIONS_TO_GLOB})

set(SOURCES
  Curvi.cpp
  PrecisionControl.cpp
  RefinementControl.cpp
  RefinementControlService.cpp
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
  tests/TimeSteppingTest.cpp

  amr/tests/ChildDataStageTest.cpp
  amr/tests/InterpolationTest.cpp
  amr/tests/MultiresolutionDetailTest.cpp
  amr/tests/RestrictionTest.cpp

  dg/BoundaryConditions.cpp
  dg/CellIntegral.cpp
  dg/DGUtils.cpp
  dg/LimitingSolverState.cpp
  dg/Riemann.cpp
  dg/tests/AMRMatricesTest.cpp
  dg/tests/CellIntegralTest.cpp
  dg/tests/DGTracerTest.cpp
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

  fv/PatchUtils.cpp
  fv/PostprocessingKernels.cpp
  fv/tests/ApplyBoundaryConditionsOnFaceTest.cpp
  fv/tests/ApplySplit1DRiemannToPatchTest.cpp
  fv/tests/CopyPatchTest.cpp
  fv/tests/EigenvalueStatisticsTest.cpp
  fv/tests/FiniteVolumeTest.cpp
  fv/limiter/tests/SlopeLimiterTest.cpp
  fv/particles/ParticleForces.cpp
  fv/particles/ParticleHelper.cpp
  fv/particles/ParticleInterpolation.cpp
)

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
