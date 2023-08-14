// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

/* #include <list> */
/* #include <unordered_set> */

namespace swift2 {
  namespace kernels {


    /**
     *@todo add documentation
     */
    template <typename Particle>
    void hydro_end_density(Particle& localParticle);


    /**
     *@todo add documentation
     */
    template <typename Particle>
    void hydro_prepare_density(Particle& localParticle);


    /**
     *@brief find the smoothing length of a localParticle
     * iteratively while looping over the neighboring activeParticles.
     *
     * This implementation assumes that the localParticle's search radius
     * is constant and correct.
     */
    template <typename Particle, typename ParticleContainer>
    void findSmoothingLengthWithConstantSearchRadius(Particle& localParticle, const ParticleContainer& activeParticles);

  } // namespace kernels
} // namespace swift2


#include "SmoothingLengthComputation.cpph"
