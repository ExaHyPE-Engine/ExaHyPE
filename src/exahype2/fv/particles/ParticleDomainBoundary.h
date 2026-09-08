// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/fv/particles/ParticleHelper.h"
#include "tarch/la/Vector.h"

namespace exahype2::fv::particles::boundary {
  /**
   * Mirrors newPosition back into the domain along every non-periodic axis
   * and multiplies the particle's velocity along that axis by
   * -dampingFactor.
   */
  template <typename Particle>
  void reflectIfOutOfDomain(
    Particle&                              particle,
    tarch::la::Vector<DIMENSIONS, double>& newPosition,
    double                                 dampingFactor
  );
} // namespace exahype2::fv::particles::boundary

#include "exahype2/fv/particles/ParticleDomainBoundary.cpph"
