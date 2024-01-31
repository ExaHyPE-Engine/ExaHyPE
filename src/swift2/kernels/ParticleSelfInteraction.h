// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <functional>
#include <list>
#include <vector>

#include "tarch/la/Vector.h"


namespace swift2 {
  namespace kernels {
    /**
     * Flag boundary particles.
     * These particles we will not be updated by any algorithmic step.
     *
     * @param ParticleContainer Typicaly either
     *         std::list<Particle *>
     *   or
     *         std::unordered_set<Particle *>
     *
     *
     */
    template <typename ParticleContainer>
    void flagBoundaryParticles(const ParticleContainer& localParticles, const double nparts);

    template <typename ParticleContainer>
    void flagBoundaryParticles(
      const ParticleContainer&                     localParticles,
      const double                                 nparts,
      const tarch::la::Vector<Dimensions, double>& domainSize,
      const tarch::la::Vector<Dimensions, double>& domainOffset
    );

  } // namespace kernels
} // namespace swift2

#include "ParticleSelfInteraction.cpph"
