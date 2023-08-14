// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace swift2 {
  namespace kernels {

    /**
     *@brief find the smoothing length of a localParticle
     * iteratively while looping over the neighboring activeParticles.
     *
     * This implementation assumes that the localParticle's search radius
     * is constant and correct, i.e. that the search radius is larger than
     * or equal to the compact support radius of the particle.
     */
    template <typename Particle, typename ParticleContainer>
    void findSmoothingLengthWithConstantSearchRadius(Particle& localParticle, const ParticleContainer& activeParticles);

  } // namespace kernels
} // namespace swift2


#include "SmoothingLengthComputation.cpph"
