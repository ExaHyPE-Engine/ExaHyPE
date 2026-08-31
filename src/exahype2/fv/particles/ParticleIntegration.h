// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/fv/particles/ParticleForces.h"
#include "exahype2/fv/particles/ParticleHelper.h"
#include "exahype2/fv/particles/ParticleIntegrationHelper.h"
#include "exahype2/fv/particles/ParticleInterpolation.h"
#include "peano4/datamanagement/CellMarker.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"

namespace exahype2::fv::particles::integration {
  template <typename Particle>
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> explicitEulerIntegration(
    Particle&    particle,
    const double timeStepSize
  );

  template <typename Particle>
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> semiImplicitEulerIntegration(
    Particle&    particle,
    const double timeStepSize
  );

  template <int... ParticleAttributeIndices, typename Particle>
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> midpointIntegration(
    Particle&                                 particle,
    const peano4::datamanagement::CellMarker& cellMarker,
    const double* NOALIAS const               cellValues,
    const double                              timeStepSize
  );

  template <int... ParticleAttributeIndices, typename Particle>
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> rk3Integration(
    Particle&                                 particle,
    const peano4::datamanagement::CellMarker& cellMarker,
    const double* NOALIAS const               cellValues,
    const double                              timeStepSize
  );

  template <int... ParticleAttributeIndices, typename Particle>
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> rk4Integration(
    Particle&                                 particle,
    const peano4::datamanagement::CellMarker& cellMarker,
    const double* NOALIAS const               cellValues,
    const double                              timeStepSize
  );

  template <int... ParticleAttributeIndices, typename Particle>
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> velocityVerletIntegration(
    Particle&                                 particle,
    const peano4::datamanagement::CellMarker& cellMarker,
    const double* NOALIAS const               cellValues,
    const double                              timeStepSize
  );

  template <int... ParticleAttributeIndices, typename Particle>
  INLINE_METHOD tarch::la::Vector<DIMENSIONS, double> applyIntegration(
    Particle&                                 particle,
    const peano4::datamanagement::CellMarker& cellMarker,
    const double* NOALIAS const               cellValues,
    const double                              timeStepSize,
    const int                                 integrationMethod
  );
} // namespace exahype2::fv::particles::integration

#include "exahype2/fv/particles/ParticleIntegration.cpph"
