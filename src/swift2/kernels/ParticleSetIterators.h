// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "ParticleUpdatePredicates.h"


#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/Lock.h"


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
     *
     * A typical operator looks like
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~
     * [=](
     *   const peano4::datamanagement::VertexMarker& marker,
     *   globaldata::MyParticle&                     particle
     * ) -> void {
     *   if ( ::swift2::kernels::localParticleCanBeMoved(marker, particle) ) {
     *     [...]
     *     particle.setMoveState(Particle::MoveState::Moved);
     *   }
     * }
     * ~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * This realisation is stereotypical and also highlight one tiny detail:
     * If you alter the particle position, you have to reset the MoveState.
     * Other kernels do not have any additional bookkeeping.
     *
     * Swift offers a set of pre-defined timestepping and reduction kernels.
     * These are templates and hence have to be specialised:
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~
     * AlgorithmStep(
     *   [...]
     *   touch_vertex_first_time_kernel = """::swift2::kernels::forAllParticles( marker, assignedParticles, ::swift2::timestepping::resetMovedParticleMarker<globaldata::{}> );""".format( my_particle_type ),
     *   touch_vertex_last_time_kernel  = """::swift2::kernels::forAllParticles( marker, assignedParticles, ::swift2::timestepping::computeExplicitEulerWithGlobalTimeStepSize<globaldata::{}> );""".format( my_particle_type ),
     *   [...]
     * )
     * ~~~~~~~~~~~~~~~~~~~~~~~~
     *
     */
    template<typename F, typename Particle>
    concept ParticleUnaryOperatorOnVertex = requires (F f, const peano4::datamanagement::VertexMarker &marker, Particle &localParticle) {
        { f(marker, localParticle) } -> std::same_as<void>;
    };

    template<typename F, typename Particle>
    concept ParticleUnaryOperatorOnCell = requires (F f, const peano4::datamanagement::CellMarker &marker, Particle &localParticle) {
        { f(marker, localParticle) } -> std::same_as<void>;
    };

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
     *
     * So one we assume that we employ localParticleCanBeUpdatedInCellKernelFromAnyOtherParticle
     * as update rule, a function implementing the concept of
     * ParticleBinaryOperator resembles
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * void myParticleBinaryOperator(
     *   const peano4::datamanagement::CellMarker& marker,
     *   globaldata::MyParticle&                   localParticle,
     *   globaldata::MyParticle&                   activeParticle
     * ) {
     *   if ( ::swift2::kernels::localParticleCanBeUpdatedInCellKernelFromAnyOtherParticle(
     *     marker,
     *     localParticle,
     *     activeParticle
     *   )) {
     *     [...] // your code
     *   }
     * }
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * This function can now be used anywhere where a template expects a
     * ParticleBinaryOperator.
     *
     *
     * ## Using the function
     *
     * Once you have introduced your function (in a cpp file of your choice,
     * e.g.), you can hand it in via a plain myParticleBinaryOperator argument.
     *
     *
     * For very simple interactions that you define only once, don't want to
     * unit test, ... you might skip the explicit declaration and add the thing
     * in directly as a functor:
     *
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * [=](
     *   const peano4::datamanagement::CellMarker& marker,
     *   globaldata::MyParticle&                   localParticle,
     *   globaldata::MyParticle&                   activeParticle
     * ) -> void {
     *   if ( ::swift2::kernels::localParticleCanBeUpdatedInCellKernelFromAnyOtherParticle(
     *     marker,
     *     localParticle,
     *     activeParticle
     *   )) {
     *     [...] // your code
     *   }
     * }
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * ## Type-generic interaction operators
     *
     * Obviously, you might want to make this function
     * a template, so it can be used for different particles. It might also be
     * tempting to distinguish the type of active and local particles, so you
     * can have interactions between different species:
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * template <typename LocalParticle, typename ActiveParticle>
     * void myParticleBinaryOperator(
     *   const peano4::datamanagement::CellMarker& marker,
     *   LocalParticle&                            localParticle,
     *   ActiveParticle&                           activeParticle
     * ) {
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * The only thing to take into account is that the compiler now might
     * struggle to deduce the type. If this happens, simply hand in
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * myParticleBinaryOperator<globaldata::MyType,globaldata::MyType>
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * as argument where myParticleBinaryOperator had been sufficient before.
     */
    template<typename F, typename LocalParticle, typename ActiveParticle>
    concept ParticleBinaryOperator = requires (F f, const peano4::datamanagement::CellMarker& marker, LocalParticle& localParticle, ActiveParticle& activeParticle) {
        { f(marker, localParticle, activeParticle) } -> std::same_as<void>;
    };

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
      ParticleUnaryOperatorOnVertex<typename ParticleContainer::DoFType > auto f,
      UpdateParticleAssignedToVertexPredicate<typename ParticleContainer::DoFType> predicate =
          ::swift2::kernels::localParticleCanBeUpdatedInVertexKernel<typename ParticleContainer::DoFType>
    );


    /**
     * Loop over all particles within a cell
     */
    template <typename ParticleContainer>
    void forAllLocalParticles(
      const peano4::datamanagement::CellMarker&                    marker,
      ParticleContainer&                                           localParticles,
      ParticleUnaryOperatorOnCell<typename std::remove_pointer<typename ParticleContainer::value_type>::type > auto f,
      UpdateParticleAssignedToCellPredicate<typename std::remove_pointer<typename ParticleContainer::value_type>::type>
          predicate = ::swift2::kernels::localParticleCanBeUpdatedInCellKernel<typename std::remove_pointer<typename ParticleContainer::value_type>::type>
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
     * This routine is not be used/does not work if f alters the global state,
     * i.e. if f performs a global reduction.
     */
    template <typename ParticleContainer>
    void forAllParticlesVectorised(
      const peano4::datamanagement::VertexMarker&                  marker,
      ParticleContainer&                                           assignedParticles,
      int                                                          numberOfAssignedParticles,
      ParticleUnaryOperatorOnVertex<typename ParticleContainer::DoFType > auto  f
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
     *
     * This routine is not be used/does not work if f alters the global state,
     * i.e. if f performs a global reduction.
     */
    template <typename ParticleContainer>
    void forAllParticlesVectoriseWithCheckPreamble(
      const peano4::datamanagement::VertexMarker&                  marker,
      ParticleContainer&                                           assignedParticles,
      int                                                          numberOfAssignedParticles,
      ParticleUnaryOperatorOnVertex<typename ParticleContainer::DoFType > auto f,
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
          auto f,
      UpdateParticleAssignedToCellPredicate<typename std::remove_pointer<typename LocalParticleContainer::value_type>::type>
          localParticlePredicate = ::swift2::kernels::alwaysUpdateInCellKernel<typename std::remove_pointer<typename LocalParticleContainer::value_type>::type>,
      UpdateParticlePairWithinCellPredicate<typename std::remove_pointer<typename LocalParticleContainer::value_type>::type>
          particlePairPredicate = ::swift2::kernels::localParticleCanBeUpdatedInCellKernelFromAnyOtherParticle<typename std::remove_pointer<typename LocalParticleContainer::value_type>::type>
    );


    /**
     * Brute-force vectorised update
     *
     * The idea behind this routine is that we run over the particle pairs in a
     * brute force manner. It works if and only if the user works with
     * coalesced memory access and therefore knows the number of local
     * particles. We assume/hope that f is vectorisable. To avoid simultaneous
     * writes to one particle, we loop over the active particles first and then
     * over the local ones. That is, we compute the impact of one active
     * particle onto all local particles.
     *
     * The interaction between fine and coarse particles requires special
     * attention: If a particle pair arises from different resolution levels,
     * we have to add the arising force bi-directionally. However, the
     * underlying check ("are we on the same level") is not vector-friendly.
     * What we do therefore is to extract this check out into a separate
     * nested loop which we add as a epilogue.
     *
     * @todo Pawel I think this is a second place where we can, in the second
     *       variant, use a bitset.
     */
    template <typename LocalParticleContainer, typename ActiveParticleContainer>
    void forAllParticlePairsVectorised(
      const peano4::datamanagement::CellMarker&  marker,
      LocalParticleContainer&                    localParticles,
      ActiveParticleContainer&                   activeParticles,
      const std::vector<int>&                    numberOfLocalParticlesPerVertex,
      const std::vector<int>&                    numberOfActiveParticlesPerVertex,
      ParticleBinaryOperator<typename std::remove_pointer<typename LocalParticleContainer::value_type>::type, typename std::remove_pointer<typename ActiveParticleContainer::value_type>::type>
         auto f
    );

    /**
     * @todo dummy function at the moment. Only wraps around forAllParticlePairsVectorised()
     */
    template <typename LocalParticleContainer, typename ActiveParticleContainer>
    void forAllParticlePairsVectoriseWithCheckPreamble(
      const peano4::datamanagement::CellMarker&  marker,
      LocalParticleContainer&                    localParticles,
      ActiveParticleContainer&                   activeParticles,
      const std::vector<int>&                    numberOfLocalParticlesPerVertex,
      const std::vector<int>&                    numberOfActiveParticlesPerVertex,
      ParticleBinaryOperator<typename std::remove_pointer<typename LocalParticleContainer::value_type>::type, typename std::remove_pointer<typename ActiveParticleContainer::value_type>::type>
         auto f
    );

    namespace internal {
      /**
       * If particles on multiple levels interact, we have to be very
       * careful and ensure that multiple subcells (if mapped onto tasks)
       * do not write to the same coarse grid particles concurrently.
       */
      extern tarch::multicore::BooleanSemaphore  _multiscaleInteractionSemaphore;
    }
  } // namespace kernels
} // namespace swift2


#include "ParticleSetIterators.cpph"
