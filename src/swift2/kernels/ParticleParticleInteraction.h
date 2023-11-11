// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <list>
#include <unordered_set>

#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/VertexMarker.h"

namespace swift2 {
  namespace kernels {
    /**
     * Generic particle interaction kernel accepting a functor
     *
     * The generic implementation realises a nested loops: We call interaction
     * per pair of local and active particles. The interaction functor may
     * modify the local particle. Besides the simple nested loop, the routine
     * ensures that no particle is updated twice if it belongs to two cells
     * simultaneously.
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
      const peano4::datamanagement::CellMarker&                                          marker,
      const ParticleContainer&                                                           localParticles,
      const ParticleContainer&                                                           activeParticles,
      std::function<void(Particle* localParticle, const Particle* const activeParticle)> interaction
    );

    /**
     * @brief The actual density kernel, which interacts a local particle and an active
     * particle, and updates the local particle.
     */
    template <typename Particle>
    void densityKernel(Particle* localParticle, const Particle* activeParticle);

    /**
     * @brief The actual force kernel, which interacts a local particle and an active
     * particle, and updates the local particle.
     *
     * ## Vectorisation
     *
     * This operation struggles to vectorise, as we have the statement
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *   if (tarch::la::smaller(r, iactR) and tarch::la::greater(r, 0.0) and not localParticle->getIsBoundaryParticle())
     * {
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * in there. This if statement encapsulates quite a lot of operations. The
     * Intel compiler therefore cannot vectorise this routine anymore.
     *
     *
     * @see forceKernelVectorised()
     */
    template <typename Particle>
    void forceKernel(Particle* localParticle, const Particle* activeParticle);

    /**
     * Vectorised alternative implementation of forceKernel()
     *
     * This routine is a vectorised variant of forceKernel() which you can use
     * if and only if you know that localParticle and activeParticle never ever
     * can point to the same object, i.e. if you know that there is no aliasing.
     * It is not clear if this operation is faster than forceKernel, but it
     * delivers more flops due to the excessive vectorisation.
     *
     * To make the function vectorisable, we had to apply a few tweaks compared
     * to the non-vector counterpart. Most of these tweaks are now
     * @ref swift_particle_new_solver "documented as part of Swift's generic solver documentation".
     *
     *
     * ## Specific differences to other vector kernel
     *
     * As we know a priori that this function is only invoked on distinct junks
     * of particles, we know that we'll compare the same particle against each
     * other. Consequently, we may assume that the radius is never 0. This
     * additional check can be omitted.
     *
     *
     *
     * @todo This routine does not support boundary particles. But we wanted to
     *   remove this flag anyway.
     */
    template <typename Particle>
    void forceKernelVectorised(Particle* __restrict__ localParticle, const Particle* __restrict__ activeParticle);

    /**
     * First part of force kernel routine, where we check if two particles are
     * close enough to actually interact.
     */
    template <typename Particle>
    bool forceKernelDistanceCheck(Particle* __restrict__ localParticle, const Particle* __restrict__ activeParticle);

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
     * @todo Mladen: check docu once this is fixed.
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
    void computeDensity(
      const peano4::datamanagement::CellMarker& marker,
      const ParticleContainer&                  localParticles,
      const ParticleContainer&                  activeParticles
    );

    /**
     * @brief Same as computeDensity(...), but doesn't modify the particles' search radii.
     *
     */
    template <typename ParticleContainer>
    void computeDensityWithConstantSearchRadius(
      const peano4::datamanagement::CellMarker& marker,
      const ParticleContainer&                  localParticles,
      const ParticleContainer&                  activeParticles
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
      const peano4::datamanagement::CellMarker& marker,
      const ParticleContainer&                  localParticles,
      const ParticleContainer&                  activeParticles
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
    void computeHydroForce(
      const peano4::datamanagement::CellMarker& marker,
      const ParticleContainer&                  localParticles,
      const ParticleContainer&                  activeParticles
    );


    /**
     * Optimised alternative to computeHydroForce()
     *
     * Semantically, this operation does exactly the same as
     * computeHydroForce(). However, there are a few differences how the
     * function runs through the particles. The routine can be used if and only
     * if you use a @ref toolbox_particles_memorypool "memory pool" as it is
     * also discussed in the @ref page_swift_performance_optimisation "optimisation guidelines".
     *
     * The routine knows way more about the internal ordering of the particles
     * through the explicit arguments particlesAssociatedWithLocalVertices and
     * numberOfParticlesPerLocalVertex. It implicitly knows how the particles
     * are stored through the naming convention/the fact that you have to use
     * it with a memory pool. The Python class
     * peano4.toolbox.particles.UpdateParticle_MultiLevelInteraction_StackOfLists_ContiguousParticles.UpdateParticle_MultiLevelInteraction_StackOfLists_ContiguousParticles
     * provides a lot of information how we construct the passed lists over
     * particles.
     *
     * Further to the optimised loops, the function calls forceKernelVectorised()
     * instead of forceKernel() whenever it knows a priori that two particle
     * ranges to not overlap. See forceKernelVectorised() for further remarks
     * how we deliver vectorised kernels.
     *
     * See also @ref swift_particle_new_solver Create new solvers "Swift's discussion how to vectorise kernels"
     * for further details.
     *
     */
    template <typename ParticleContainer>
    void computeHydroForce_vectoriseAndMapDistanceChecksOntoMasking(
      const peano4::datamanagement::CellMarker& marker,
      const ParticleContainer&                  localVertices,
      const std::vector<int>&                   numberOfParticlesPerVertex,
      const ParticleContainer&                  activeParticles,
      const std::vector<int>&                   numberOfActiveParticlesPerVertex
    );


    /**
     * Alternative implementation of the hydro force which splits up the
     * underlying algorithm into two steps:
     *
     * 1. Identify all the particles which actually impose an interaction.
     *    This is a purely geometric distance check and its result is stored in
     *    a boolean array.
     * 2. For those interactions which are flagged as interactive, we actually
     *    evaluate the force.
     *
     * In this variant, we "only" vectorise the first step, assuming that this
     * is where the majority of the runtime is spent. This way, we loose out on
     * the vectorisation potential of the force, but we do not do a lot of
     * calculations which eventually are masked out anyway.
     */
    template <typename ParticleContainer>
    void computeHydroForce_vectoriseDistanceChecks(
      const peano4::datamanagement::CellMarker& marker,
      const ParticleContainer&                  localVertices,
      const std::vector<int>&                   numberOfParticlesPerVertex,
      const ParticleContainer&                  activeParticles,
      const std::vector<int>&                   numberOfActiveParticlesPerVertex
    );
  } // namespace kernels
} // namespace swift2

#include "ParticleParticleInteraction.cpph"
