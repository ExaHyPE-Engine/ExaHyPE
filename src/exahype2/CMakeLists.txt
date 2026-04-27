# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

file(GLOB_RECURSE HEADERS ${HEADER_EXTENSIONS_TO_GLOB})

set(SOURCES
  exahype2.cpp
  CellAccess.cpp
  CellData.cpp
  EnclaveBookkeeping.cpp
  EnclaveTask.cpp
  LoadBalancingConfiguration.cpp
  PrecisionControl.cpp
  RefinementControl.cpp
  RefinementControlService.cpp
  TimeStepping.cpp
  UnitTests.cpp
  tests/EnclaveBookkeepingTest.cpp

  dg/BoundaryConditions.cpp
  dg/CellIntegral.cpp
  dg/DGUtils.cpp
  dg/Riemann.cpp
  dg/Tracer.cpp
  dg/tests/CellIntegralTest.cpp
  dg/tests/DGUtilsTest.cpp
  dg/tests/RiemannTest.cpp
  dg/tests/TestUtils.cpp
  dg/rusanov/Rusanov.cpp
  dg/rusanov/tests/RiemannTest.cpp

  fv/BoundaryConditions.cpp
  fv/InterpolationRestriction.cpp
  fv/PatchUtils.cpp
  fv/PostprocessingKernels.cpp
  fv/Tracer.cpp
  fv/tests/ApplySplit1DRiemannToPatchTest.cpp
  fv/tests/CopyPatchTest.cpp
  fv/tests/InterpolationRestrictionTest.cpp
  fv/particles/MultiParticleIntegration.cpp
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
  foreach(dim IN LISTS SUPPORTED_DIMENSIONS)
    add_peano_library(
      NAME ExaHyPE2${mode}${dim}d
      MODE ${mode}
      DIMENSION ${dim}
      SOURCES ${SOURCES}
      HEADERS ${HEADERS}
      LINK_LIBRARIES ToolboxBlockStructured${mode}${dim}d ToolboxLoadBalancing${mode}${dim}d ToolboxParticles${mode}${dim}d
    )
  endforeach()
endforeach()
