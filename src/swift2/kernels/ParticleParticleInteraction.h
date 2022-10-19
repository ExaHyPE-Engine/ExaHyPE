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
    std::function<void(Particle *localParticle,
                       const Particle *const activeParticle)>
        interaction);

// Density calculation particle-particle kernel
template <typename Particle>
void computeDensityWithConstantSmoothingLength(
    const std::list<Particle *> &localParticles,
    const std::list<Particle *> &activeParticles);

} // namespace kernels
} // namespace swift2

#include "ParticleParticleInteraction.cpph"
