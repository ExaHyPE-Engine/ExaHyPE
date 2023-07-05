// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <list>
#include <unordered_set>

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
     *
     * @param ParticleContainer Typicaly either
     *         std::list<Particle *>
     *   or
     *         std::unordered_set<Particle *>
     *
     *
     */
    template <typename Particle, typename ParticleContainer>
    void genericInteraction(
      const ParticleContainer&                                                           localParticles,
      const ParticleContainer&                                                           activeParticles,
      std::function<void(Particle* localParticle, const Particle* const activeParticle)> interaction
    );

    /**
     * @brief The actual density kernel, which interacts a local particle and an active
     * particle, and updated the local particle.
     *
     */
    template <typename Particle>
    void densityKernel(Particle* localParticle, const Particle* activeParticle);

    /**
     * @brief Non-symmetric density calculation (as per Swift terminology).
     * In this, only the local particle is updated (in the symmetric case, both
     * are updated at the same time). This compute kernel is inserted by Peano's
     * Python API into the nested loop over active and local particles. So we can
     * assume that those two terms do exist.
     *
     * The present kernel is a sole particle-particle interaction kernel.
     *
     * Here we calculate the density and drho/dt sums.
     * The latter can be used in the Newton-Raphson iteration.
     *
     * Subscripts a and b represent indices of Local and Active particles, respectively:
     *
     * h_a: Smoothing length of Local particle
     * h_b: Smoothign length of Active particle
     * dx : Vector separating the two particles (x_loc - x_act)
     * r  : Distance between the two particles  (|| dx ||)
     *
     *
     * @param ParticleContainer Typicaly either
     *         std::list<Particle *>
     *   or
     *         std::unordered_set<Particle *>
     *   where Particle is your particular Particle type.
     *
     */
    template <typename ParticleContainer>
    void computeDensity(const ParticleContainer& localParticles, const ParticleContainer& activeParticles);

    /**
     * @brief Same as computeDensity(...), but doesn't modify the particles' search radii.
     *
     */
    template <typename ParticleContainer>
    void computeDensityWithConstantSearchRadius(
      const ParticleContainer& localParticles, const ParticleContainer& activeParticles
    );

    /**
     * @brief Same as computeDensity(...), but doesn't modify the particles' search radii
     * nor their smoothing length.
     *
     * WARNING: the only reason we still have this is because it was already implemented,
     * and we might use it in the future. However we currently DO NOT have a SPH
     * formulation which correctly works with constant smoothing lengths. We're solving
     * the wrong equations for that.
     *
     */
    template <typename ParticleContainer>
    void computeDensityWithConstantSmoothingLengths(
      const ParticleContainer& localParticles, const ParticleContainer& activeParticles
    );

    /** Hydro Force calculation particle-particle kernel
     *
     * @brief Non-symmetric force interaction between two particles (as per Swift terminology).
     * In this, only the Local particle is updated (in the symmetric case, both are updated at the same time).
     * Time derivative of the internal energy, du/dt, is also updated here.
     * Subscripts a and b represent indices of Local and Active particles, respectively:
     *
     * h_a: Smoothing length of Local particle
     * h_b: Smoothign length of Active particle
     * dx : Vector separating the two particles (x_loc - x_act)
     * r  : Distance between the two particles  (|| dx ||)
     *
     *
     * @param ParticleContainer Typicaly either
     *         std::list<Particle *>
     *   or
     *         std::unordered_set<Particle *>
     *   where Particle is your particular Particle type.
     *
     */
    template <typename ParticleContainer>
    void computeHydroForce(const ParticleContainer& localParticles, const ParticleContainer& activeParticles);

  } // namespace kernels
} // namespace swift2

#include "ParticleParticleInteraction.cpph"
