// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <list>
#include <vector>


namespace swift2 {
  namespace kernels {
    /**
     * Generic particle interaction kernel accepting a functor
     *
     * The generic implementation realises a nested loops: We call interaction
     * per pair of local and active particles. The interaction functor may
     * modify the local particle. Besides the simple nested loop, the routine
     * ensures that no particle is updated twice if it belongs to two cells
     * simulatenously.
     *
     * What the routine not does is to check if activeParticle!=localParticle,
     * i.e. please assume that the routine is also called for self-interaction
     * potentials.
     */
    template <typename Particle>
    void genericSelfInteraction(
      const std::vector< Particle* >& localParticles,
      std::function< void(Particle* localParticle) > interaction
    );

    /* This is equivalent to the hydro_init_part function in SWIFT */
    // Prepares the particle for the density calculation. Calculates self
    // contribution, etc
    template <typename Particle>
    void prepareDensityCalculation(
      const std::vector< Particle* >& localParticles
    );

    /* This is equivalent to the hydro_end_density function in SWIFT */
    // Ends the density calculation. Also does a single Newton-Raphson
    // iteration.
    template <typename Particle>
    void endDensityCalculation(
      const std::vector< Particle* >& localParticles,
      const double kernelSupportRadius,
      const int hydroDimensions
    );




  }
}


#include "ParticleSelfInteraction.cpph"

