// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <functional>
#include <list>
#include <vector>

#include "SwiftKernels.h"
#include "tarch/la/Vector.h"


namespace swift2 {
  namespace kernels {
    /**
     * Generic particle self-interaction kernel accepting a functor
     *
     * @TODO Mladen: check docu here once you implement proper SML computation
     * The generic implementation realises a single loop: We call a self interaction
     * per particle that has no dependence over neighbours. The functor hence modifies the particle.
     *
     * We distinguish two use cases for the particle self-interactions:
     *
     * 1) We want to execute a stand-alone computation over the particle, e.g.
     * the kick step, which is does not require any particle-particle
     * contribution.
     *
     * 2) We want to execute computations after and before the particle-particle interactions kernels, e.g. to
     * 'prepare' or 'end' a given calculation in SPH loops (e.g. density).
     * In order to ensure the self-interaction kernels execute consistently during the mesh
     * traversals for these type of operations, the user should bear in mind the difference between these two cases:
     *
     *  - The 'prepare' case should be mapped into touchVertexFirstTime. Peano internally ensures
     *  that CellHasUpdatedParticle is false in this situation.
     *  - The 'end' case should be mapped into touchVertexLastTime. Peano internally ensures
     *  that CellHasUpdatedParticle is true in this situation.
     *
     *
     * @param ParticleContainer Typicaly either
     *         std::list<Particle *>
     *   or
     *         std::unordered_set<Particle *>
     *
     *
     */
    template <typename ParticleContainer, typename Particle>
    void genericSelfInteraction(
      const ParticleContainer& localParticles, std::function<void(Particle* localParticle)> interaction
    );

    /**
     * @brief Prepares a particle for the density calculation
     *
     * Zeroes all the relevant arrays in perparation for the sums taking place
     * in the loop over neighbours (local-active set)
     *
     * This is equivalent to the hydro_init_part function in SWIFT
     *
     * @param ParticleContainer Typicaly either
     *         std::list<Particle *>
     *   or
     *         std::unordered_set<Particle *>
     *
     *
     */
    template <typename ParticleContainer>
    void prepareDensityCalculation(const ParticleContainer& localParticles);

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
     *
     * @param ParticleContainer Typicaly either
     *         std::list<Particle *>
     *   or
     *         std::unordered_set<Particle *>
     *
     *
     */
    template <typename ParticleContainer>
    void endDensityCalculation(const ParticleContainer& localParticles);

    /**
     * @brief Finishes the calculations from the density loop same as
     * endDensityCalculation(...), but doesn't modify the particles'
     * smoothing lengths nor search radii.
     *
     * WARNING: the only reason we still have this is because it was already implemented,
     * and we might use it in the future. However we currently DO NOT have a SPH
     * formulation which correctly works with constant smoothing lengths. We're solving
     * the wrong equations for that.
     */
    template <typename ParticleContainer>
    void endDensityCalculationWithConstantSmoothingLength(const ParticleContainer& localParticles);

    /**
     * @brief Finishes the calculations from the density loop. Same as
     * endDensityCalculation(...), but doesn't modify the particles'
     * search radii.
     */
    template <typename ParticleContainer>
    void endDensityCalculationWithConstantSearchRadius(const ParticleContainer& localParticles);

    /**
     * Resets the acceleration and other variables
     * Done by "hydro_reset_acceleration" in SWIFT
     *
     * @param ParticleContainer Typicaly either
     *         std::list<Particle *>
     *   or
     *         std::unordered_set<Particle *>
     *
     *
     */
    template <typename ParticleContainer>
    void prepareHydroForceCalculation(const ParticleContainer& localParticles);

    /**
     * Ends the force calculation. Only introduces missing factors for Udot
     * Done by "hydro_end_force" in SWIFT
     *
     * @param ParticleContainer Typicaly either
     *         std::list<Particle *>
     *   or
     *         std::unordered_set<Particle *>
     *
     *
     */
    template <typename ParticleContainer>
    void endHydroForceCalculation(const ParticleContainer& localParticles);

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
