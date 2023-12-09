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
     *
     * ## Multiscale interactions
     *
     * This routine follows Peano's @ref page_toolbox_particles_mesh_traversal "generic multiscale discussion",
     * where we point out that we have correctly take particles into account
     * which reside on coarser levels. This is necessary if some particles have
     * larger cut-off radii than the others or if we work with adaptive meshes.
     *
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
     * Check distance between two particles
     *
     * First part of force kernel routine, where we check if two particles are
     * close enough to actually interact. This routine also returns true if
     * localParticle and activeParticle are the same. That is, it does not
     * check for equality. Therefore, the routine has to be used with
     * forceKernelVectorised() which in turn checks the distance interally but
     * masks out zero distances.
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

    /**
     * Hydro Force calculation particle-particle kernel
     *
     * @brief Non-symmetric force interaction between two particles (as per
     * Swift terminology). In this, only the Local particle is updated (in the
     * symmetric case, both are updated at the same time). Time derivative of
     * the internal energy, du/dt, is also updated here. Subscripts a and b
     * represent indices of Local and Active particles, respectively:
     *
     * Our implementations loops over all local particles (outer loop) and, per
     * local particle then over all active particles (inner loop). Inside this
     * second loop, we compute the hydro force acting from the active particle
     * onto the local particle. We know that the local particles are a subset
     * of the active particle and that the hydroforce @f$ f(p,p) :=0 @f$: If a
     * particle acts on itself, the arising force equals zero. So there's no
     * need for a case distinction although we could use one.
     *
     * Important is a second case distinction: If a local and an active
     * particle reside on the same level, we know that the symmetric force
     * component will be computed later on. Let L be the local one and A the
     * active one, we have
     *
     * @f$ f(L,A) = -f(A,L) @f$
     *
     * but this antisymmetry is not exploited in this kernel. We know that
     * there will be another loop iteration or cell interaction kernel call
     * which takes care of the negative complementary force.
     *
     * However, if A resides on a coarser level than L, this assumption is wrong.
     * We need to add the force explicitly as discussed in @ref page_toolbox_particles_mesh_traversal.
     *
     *
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
     * ## Multiscale relationships
     *
     * Consult computeHydroForce() for a discussion of the handling of
     * multiscale relations. We can exploit the fact here that a set of
     * continuous active particles all should have the same cut-off
     * radius.
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
     * In this variant, we primarily vectorise the first step, assuming that
     * this is where the majority of the runtime is spent. This way, we might
     * loose out on the vectorisation potential of the force, but we do not do
     * a lot of calculations which eventually are masked out anyway.
     *
     * After step 1 has completed we have a list of markers. Now we stil try
     * to vectorise over the markers with a 1: We run trough the particle set
     * once again with a while loop. Whenever we find a particle which acts on
     * the current particle, we look @f$ K \in \{2^1-1,2^2-1,2^3-1\} @f$ steps
     * ahead. If several particle in a row are to be handled, i.e. are flagged,
     * we us a vectorised for loop.
     *
     * To make this lookahead simpler, we extend the boolean field by default
     * by MaxVectorise-1, i.e. @f$ 2^k-1 @f$, elements. Unfortunately, I had to
     * manually unroll the lookahead function to make the compiler inline and
     * vectorise properly. On the long term, this should be done automatically
     * via a for loop.
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
