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
     *
     * ## Multiscale particle storage and adaptive meshes
     *
     * This routine only accepts one particle and computes contributions from
     * all active particles around. We do not distinguish on which level these
     * active particles are stored. Therefore, this routine automatically
     * takes all particles on coarser levels and on the same resolution level
     * into account.
     *
     * It does not know about particles on finer resolution levels.
     *
     * @Mladen We have to discuss this effect.
     */
    template <typename Particle, typename ParticleContainer>
    void findSmoothingLengthWithConstantSearchRadius(Particle& localParticle, const ParticleContainer& activeParticles);

  } // namespace kernels
} // namespace swift2


#include "SmoothingLengthComputation.cpph"
