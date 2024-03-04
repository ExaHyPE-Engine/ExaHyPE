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
    void reduceVelocityAndSearchRadius_without_check(
      Particle*        particle
    );

    template <typename Particle>
    void reduceVelocityAndSearchRadius(const peano4::datamanagement::VertexMarker& marker, Particle& localParticle) {
      if (::swift2::kernels::localParticleCanBeUpdatedInVertexKernel(marker,localParticle)) {
        reduceVelocityAndSearchRadius_without_check(&localParticle);
      }
    }

    /**
     * @todo This version is not yet available and likely will never vectorise anyway
     */
    template <typename Particle>
    void reduceVelocityAndSearchRadiusWithMasking(const peano4::datamanagement::VertexMarker& marker, Particle& localParticle) {
      reduceVelocityAndSearchRadius(marker, localParticle);
    }
  } // namespace statistics
} // namespace swift2

#include "Statistics.cpph"
