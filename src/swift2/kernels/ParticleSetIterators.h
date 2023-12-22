// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "ParticleUpdatePredicates.h"


namespace swift2 {
  namespace kernels {
    /**
     * Definition of particle update (unary operation)
     *
     * The interface uses the marker, as it is the @ref swift_particle_new_solver "responsibility of any
     * update to check if the update is actually valid". That is, an
     * operator should always use a UpdateParticleAssignedToVertexPredicate
     * internally to check if it actually should update. How this knowledge
     * is taken into account is however up to the realisation. In the simplest
     * case, it is just a mere enclosing if statement.
     */
    template <typename Particle>
    using ParticleUnaryOperator = std::function<void(const peano4::datamanagement::VertexMarker& marker, Particle& localParticle)>;

    /**
     * Definition of particle-particle interaction
     *
     * This definition works if LocalParticle is a real type, but it also
     * works if it is a pointer, as we then get references to pointers in the
     * signature which degenerates to the pointers in the source code.
     * However, I usually use it without any pointer semantics.
     *
     * The interface uses the marker, as it is the @ref swift_particle_new_solver "responsibility of any
     * update to check if the update is actually valid". That is, a binary
     * operator should always use a UpdateParticlePairWithinCellPredicate
     * internally to check if it actually should update. How this knowledge
     * is taken into account is however up to the realisation. In the simplest
     * case, it is just a mere enclosing if statement.
     */
    template <typename LocalParticle, typename ActiveParticle>
    using ParticleBinaryOperator = std::function<void(const peano4::datamanagement::CellMarker& marker, LocalParticle& localParticle, ActiveParticle& activeParticle)>;

    /**
     * Run over all particles and update them independent of each other
     *
     * The routine accepts a container over particle pointers, but the
     * functor f actually accepts references. It is the responsibility of
     * this routine to map pointers onto references.
     *
     * The predicate can be used to mask out certain updates.
     *
     * We distinguish two use cases for the particle self-interactions:
     *
     * 1. We want to execute a stand-alone computation over the particle, e.g.
     *    the kick step, which is does not require any particle-particle
     *    contribution.
     * 2. We want to execute computations after and before the particle-particle
     *    interactions kernels, e.g. to 'prepare' or 'end' a given calculation in
     *    SPH loops (e.g. density).
     *
     * In order to ensure the self-interaction kernels execute consistently
     * during the mesh traversals for these type of operations, the user should
     * bear in mind the difference between these two cases:
     *
     *  - The 'prepare' case should be mapped into touchVertexFirstTime(). Peano
     *    internally ensures that CellHasUpdatedParticle is false in this situation.
     *  - The 'end' case should be mapped into touchVertexLastTime. Peano
     *    internally ensures that CellHasUpdatedParticle is true in this situation.
     *
     * This routine does not vectorise over the particles. If any vectorisation
     * is used, you'll see the vector instructions arise from the actual compute
     * kernel.
     *
     * @param ParticleContainer A subtype of std::list<Particle *>
     */
    template <typename ParticleContainer>
    void forAllParticles(
      const peano4::datamanagement::VertexMarker&                  marker,
      ParticleContainer&                                           assignedParticles,
      ParticleUnaryOperator<typename ParticleContainer::DoFType >  f,
      UpdateParticleAssignedToVertexPredicate<typename ParticleContainer::DoFType> predicate =
          ::swift2::kernels::localParticleCanBeUpdatedInVertexKernel<typename ParticleContainer::DoFType>
    );


    /**
     * Alternative to forAllParticles() which vectorises
     *
     * This routine attempts a brute force vectorisation of the function calls.
     * Therefore, it does not accept a predicate. We simply rely on the fact
     * that all compute kernels have to realise their validity checks
     * internally anyway and make the outer loop a for loop which should
     * vectorise. For this outer loop, we employ @ref peano4_utils_Loop_parallel "Peano's loop macros" from
     * peano4/utils/Loop.h, which ensure that we use bespoke loop constructs
     * depending on the multithreading backend chosen.
     *
     * @see peano4/utils/Loop.h
     */
    template <typename ParticleContainer>
    void forAllParticles_vectorise(
      const peano4::datamanagement::VertexMarker&                  marker,
      ParticleContainer&                                           assignedParticles,
      int                                                          numberOfAssignedParticles,
      ParticleUnaryOperator<typename ParticleContainer::DoFType >  f
    );


    /**
     * Alternative implementation of forAllParticles_vectorise()
     *
     * This routine is a variant of the vectorised kernels which works with a
     * dedicated check preamble: First, it evaluates the predicate for each
     * particle in the list and memorises the outcome. After that, we split
     * the sequence of particles into chunks of 8. If more than 5 particles
     * have to be updated, we do all 8 of the chunk in one block. If not, we
     * subdivide the block into two chunks of size 4 and continue recursively.
     *
     * Logically, it would be nice to work with a bitset here. However, bitsets
     * are large, and it is not clear if operations on them would map onto
     * AVX instructions.
     */
    template <typename ParticleContainer>
    void forAllParticles_vectoriseWithCheckPreamble(
      const peano4::datamanagement::VertexMarker&                  marker,
      ParticleContainer&                                           assignedParticles,
      int                                                          numberOfAssignedParticles,
      ParticleUnaryOperator<typename ParticleContainer::DoFType >  f,
      UpdateParticleAssignedToVertexPredicate<typename ParticleContainer::DoFType> predicate =
          ::swift2::kernels::localParticleCanBeUpdatedInVertexKernel<typename ParticleContainer::DoFType>
    );


    /**
     * Run over all local particle-active particle combinations
     *
     * The generic implementation realises a nested loops: We call interaction
     * per pair of local and active particles. The interaction functor may
     * modify the local particle. It is the responsibility of the update
     * predicate to ensure that no particle is updated twice, even if it is
     * sitting right at the face in-between two cell. The predicate also
     * should check if there's a self-interaction and mask it out if
     * appropriate.
     *
     * The routine accepts a container over particle pointers, but the
     * functor f actually accepts references. It is the responsibility of
     * this routine to map pointers onto references. We use
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *   for (auto& localParticle : localParticles) {
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * here, but it is important to note that this reference still becomes a
     * reference to a pointer as localParticle is a container over pointers.
     *
     * In Swift's terminology, this is a non-symmetric XXX calculation where XXX
     * is the compute kernel (such as density):
     * Our implementations loops over all local particles (outer loop) and, per
     * local particle then over all active particles (inner loop). Inside this
     * second loop, we compute a force acting from the active particle
     * onto the local particle.
     *
     * To avoid self-interaction, users have to ensure that @f$ f(p,p) :=0 @f$: If a
     * particle acts on itself, the arising force equals zero. This should
     * always be the case, as the predicate passed in here is an optimisation,
     * i.e. users should not rely on it to mask out self-interaction a priori.
     *
     * Equally important is a second case distinction: If a local and an active
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
     * ## Multiscale interactions
     *
     * This routine follows Peano's @ref page_toolbox_particles_mesh_traversal "generic multiscale discussion",
     * where we point out that we have correctly take particles into account
     * which reside on coarser levels. This is necessary if some particles have
     * larger cut-off radii than the others or if we work with adaptive meshes.
     *
     * ## Examples of usage
     *
     * This would be a generic predicate as most kernels require it:
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~
     * UpdateParticlePairWithinCellPredicate<typename std::remove_pointer<typename LocalParticleContainer::value_type>::type> predicate = ::swift2::kernels::localParticleCanBeUpdatedInCellKernelFromAnyOtherParticle<typename std::remove_pointer<typename LocalParticleContainer::value_type>::type>
     * ~~~~~~~~~~~~~~~~~~~~~~~
     *
     * However, as this function is usually invoked by code generated through
     * Python, I personally recommend to use a syntax that is easier to parse:
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~
     * "UpdateParticlePairWithinCellPredicate<globaldata::{}>".format( particle._name )
     * ~~~~~~~~~~~~~~~~~~~~~~~
     *
     *
     * ### Hydro Force calculation particle-particle kernel
     *
     * Non-symmetric force interaction between two particles (as per
     * Swift terminology). In this, only the Local particle is updated (in the
     * symmetric case, both are updated at the same time). Time derivative of
     * the internal energy, du/dt, is also updated here. Subscripts a and b
     * represent indices of Local and Active particles, respectively:
     *
     * h_a: Smoothing length of Local particle
     * h_b: Smoothign length of Active particle
     * dx : Vector separating the two particles (x_loc - x_act)
     * r  : Distance between the two particles  (|| dx ||)
     *
     * To inject the hydro force, we typically use the code snippet similar to
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * cell_kernel_for_force_calculation = "::swift2::forAllParticlePairs(marker, localParticles, activeParticles,::swift2::kernels::forceKernel<globaldata::HydroPart>);"
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * As we have to know the particle type for the injected functor, the SPH
     * particle for example replaces this similar to
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * cell_kernel_for_force_calculation = "::swift2::forAllParticlePairs(marker, localParticles, activeParticles,::swift2::kernels::forceKernel<globaldata::{}>);".format(self.name)
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * @param LocalParticleContainer  A subtype of std::list<Particle *>
     * @param ActiveParticleContainer A subtype of std::list<Particle *>
     */
    template <typename LocalParticleContainer, typename ActiveParticleContainer>
    void forAllParticlePairs(
      const peano4::datamanagement::CellMarker&  marker,
      LocalParticleContainer&                    localParticles,
      ActiveParticleContainer&                   activeParticles,
      ParticleBinaryOperator<typename std::remove_pointer<typename LocalParticleContainer::value_type>::type, typename std::remove_pointer<typename ActiveParticleContainer::value_type>::type>
          f,
      UpdateParticleAssignedToCellPredicate<typename std::remove_pointer<typename LocalParticleContainer::value_type>::type>
          localParticlePredicate = ::swift2::kernels::alwaysUpdateInCellKernel<typename std::remove_pointer<typename LocalParticleContainer::value_type>::type>,
      UpdateParticlePairWithinCellPredicate<typename std::remove_pointer<typename LocalParticleContainer::value_type>::type>
          particlePairPredicate = ::swift2::kernels::localParticleCanBeUpdatedInCellKernelFromAnyOtherParticle<typename std::remove_pointer<typename LocalParticleContainer::value_type>::type>
    );
  }
}


#include "ParticleSetIterators.cpph"
