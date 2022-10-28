// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <list>

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
    void genericInteraction(
        const std::list<Particle *> &localParticles,
        const std::list<Particle *> &activeParticles,
        std::function<void(Particle *localParticle, const Particle *const activeParticle)>
            interaction);


    // Density calculation particle-particle kernel
    template <typename Particle>
    void computeDensityWithConstantSmoothingLength(
        const std::list<Particle *> &localParticles,
        const std::list<Particle *> &activeParticles,
        const double kernelSupportRadius, 
        const int hydroDimensions);

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
     */
    template <typename Particle>
    void computeHydroForce(
        const std::list<Particle *> &localParticles,
        const std::list<Particle *> &activeParticles,
        const double kernelSupportRadius, 
        const int hydroDimensions,
        const double betaAV
        );




  } 
} 

#include "ParticleParticleInteraction.cpph"
