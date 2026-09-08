// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/fv/particles/ParticleDomainBoundary.h"
#include "exahype2/fv/particles/ParticleForces.h"
#include "exahype2/fv/particles/ParticleHelper.h"
#include "exahype2/fv/particles/ParticleIntegration.h"
#include "exahype2/fv/particles/ParticleInterpolation.h"
#include "peano4/datamanagement/CellMarker.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/VectorOperations.h"

namespace exahype2::fv::particles {
  template <int... ParticleAttributeIndices, typename Particle>
  INLINE_METHOD void updateParticleState(
    Particle&                                 particle,
    const peano4::datamanagement::CellMarker& cellMarker,
    const double* NOALIAS const               cellValues,
    const double                              timeStepSize,
    const int                                 integrationMethod
  );
} // namespace exahype2::fv::particles

#include "exahype2/fv/particles/ParticleUpdate.cpph"
