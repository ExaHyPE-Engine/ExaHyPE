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

    /**
     * @brief Prepares a particle for the density calculation  
     *
     * Zeroes all the relevant arrays in perparation for the sums taking place
     * in the loop over neighbours (local-active set)
     *
     * This is equivalent to the hydro_init_part function in SWIFT
     */
    template <typename Particle>
    void prepareDensityCalculation(
      const std::vector< Particle* >& localParticles
    );

    /** 
     * @brief Finishes the calculations from the density loop
     *
     * Multiplies the denstiy and number of neighbours by the appropiate
     * constants and add the self-contribution term. Additional quantities
     * such as the velocity gradients will also get the final terms added to
     * them here.
     * This is equivalent to the hydro_end_density function in SWIFT
     *
     * Ends the density calculation. Also does a single Newton-Raphson
     * iteration.
     *
     */
    template <typename Particle>
    void endDensityCalculation(
      const std::vector< Particle* >& localParticles,
      const int hydroDimensions,
      const double etaFactor,
      const double alphaAV,
      const double hTolerance,
      const double hHydroMin,
      const double hHydroMax
    );

    /**
     * Same as above, but with constant h, i.e. no Newton-Raphson 
     */
    template <typename Particle>
    void endDensityCalculationWithConstantSmoothingLength(
      const std::vector< Particle* >& localParticles,
      const int hydroDimensions,
      const double etaFactor,
      const double alphaAV
    );

    /**
     * Resets the acceleration and other variables
     * Done by "hydro_reset_acceleration" in SWIFT
     */
    template <typename Particle>
    void prepareHydroForceCalculation(
      const std::vector< Particle* >& localParticles
    );

    /**
     * Ends the force calculation. Only introduces missing factors for Udot
     * Done by "hydro_end_force" in SWIFT
     */
    template <typename Particle>
    void endHydroForceCalculation(
      const std::vector< Particle* >& localParticles,
      const int hydroDimensions
    );

    /**
     * Flag boundary particles.
     * These particles we will not be updated by any algorithmic step.
     */
    template <typename Particle>
    void flagBoundaryParticles(
      const std::vector< Particle* >&  localParticles
    );


  }
}


#include "ParticleSelfInteraction.cpph"

