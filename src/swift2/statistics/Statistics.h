// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <list>
#include <vector>

#include "swift2/kernels/ParticleUpdatePredicates.h"

namespace swift2 {
  namespace statistics {
    /**
     * Reduce velocity and search radius stats into species information
     *
     * Inform the species about a local particle's search radius and velocity.
     * This
     */
    template <typename Particle>
    bool reduceVelocityAndSearchRadiusUpdateParticlePredicate(
      const peano4::datamanagement::VertexMarker& marker,
      const Particle& localParticle
    ) {
      return ::swift2::kernels::localParticleCanBeUpdatedInVertexKernel(marker,localParticle);
    }

    template <typename Particle>
    void reduceVelocityAndSearchRadius(const peano4::datamanagement::VertexMarker& marker, Particle& localParticle);
  } // namespace statistics
} // namespace swift2

#include "Statistics.cpph"
