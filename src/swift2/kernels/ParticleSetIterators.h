// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "ParticleUpdatePredicates.h"
#include "swift2/CellMetaData.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/Lock.h"


namespace swift2 {
  namespace kernels {

    // template for convenience and code legibility.
    template <typename ParticleContainer>
    using PCParticle = typename std::remove_pointer<typename ParticleContainer::value_type>::type;

    /**
     * Definition of particle update (unary operation)
     *
     * The interface uses the marker, as it is the @ref
     * swift_particle_new_solver "responsibility of any update to check if the
     * update is actually valid". That is, an operator should always use a
     * UpdateParticleAssignedToVertexPredicate internally to check if it
     * actually should update. How this knowledge is taken into account is
     * however up to the realisation. In the simplest case, it is just a mere
     * enclosing if statement.
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
     *   touch_vertex_first_time_kernel = """::swift2::kernels::forAllParticles(
     * marker, assignedParticles,
     * ::swift2::timestepping::resetMovedParticleMarker<globaldata::{}>
     * );""".format( my_particle_type ), touch_vertex_last_time_kernel  =
     * """::swift2::kernels::forAllParticles( marker, assignedParticles,
     * ::swift2::timestepping::computeExplicitEulerWithGlobalTimeStepSize<globaldata::{}>
     * );""".format( my_particle_type ),
     *   [...]
     * )
     * ~~~~~~~~~~~~~~~~~~~~~~~~
     *
     */
    template <typename F, typename Particle>
    concept ParticleUnaryOperatorOnVertex = requires(
      F                                           f,
      const peano4::datamanagement::VertexMarker& marker,
      Particle&                                   localParticle
    ) {
      { f(marker, localParticle) } -> std::same_as<void>;
    };

    template <typename F, typename Particle>
    concept ParticleUnaryOperatorOnCell = requires(
      F                                         f,
      const peano4::datamanagement::CellMarker& marker,
      Particle&                                 localParticle
    ) {
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
     * The interface uses the marker, as it is the @ref
     * swift_particle_new_solver "responsibility of any update to check if the
     * update is actually valid". That is, a binary operator should always use a
     * UpdateParticlePairWithinCellPredicate internally to check if it actually
     * should update. How this knowledge is taken into account is however up to
     * the realisation. In the simplest case, it is just a mere enclosing if
     * statement.
     *
     * So one we assume that we employ
     * localParticleCanBeUpdatedInCellKernelFromAnyOtherParticle as update rule,
     * a function implementing the concept of ParticleBinaryOperator resembles
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * void myParticleBinaryOperator(
     *   const peano4::datamanagement::CellMarker& marker,
     *   globaldata::MyParticle&                   localParticle,
     *   globaldata::MyParticle&                   activeParticle
     * ) {
     *   if (
     * ::swift2::kernels::localParticleCanBeUpdatedInCellKernelFromAnyOtherParticle(
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
     *   if (
     * ::swift2::kernels::localParticleCanBeUpdatedInCellKernelFromAnyOtherParticle(
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
    template <typename F, typename LocalParticle, typename ActiveParticle>
    concept ParticleBinaryOperator = requires(
      F                                         f,
      const peano4::datamanagement::CellMarker& marker,
      LocalParticle&                            localParticle,
      ActiveParticle&                           activeParticle
    ) {
      { f(marker, localParticle, activeParticle) } -> std::same_as<void>;
    };

    /**
     * Different (generic) kernel realisations
     *
     * The iterator templates accept a type that specifies which realisation to
     * pick. There are a few default realisations which are defined in this
     * namespace. To make use of them, we define dummy instances of these
     * objects. Their sole purpose is that you can trigger the C++ type
     * deduction to find the realisation variant of choice.
     */
    namespace kernelrealisation {
      class GenericRealisation {};
      class CoalescedMemoryRealisation {};
      class CoalescedMemoryLocalActiveRealisation {};
      class CoalescedMemoryActiveLocalRealisation {};
      class EvaluatePredicateInPrologueRealisation {};
      class EvaluatePredicateInPrologueLocalActiveRealisation {};
      class EvaluatePredicateInPrologueActiveLocalRealisation {};

      class GenericOffloadRealisation {};
      class CoalescedMemoryOffloadRealisation {};
      class CoalescedMemoryLocalActiveOffloadRealisation {};
      class CoalescedMemoryActiveLocalOffloadRealisation {};

      extern GenericRealisation                     Generic;
      extern CoalescedMemoryRealisation             CoalescedMemory;
      extern CoalescedMemoryLocalActiveRealisation  CoalescedMemoryLocalActive;
      extern CoalescedMemoryActiveLocalRealisation  CoalescedMemoryActiveLocal;
      extern EvaluatePredicateInPrologueRealisation EvaluatePredicateInPrologue;
      extern EvaluatePredicateInPrologueLocalActiveRealisation
        EvaluatePredicateInPrologueLocalActive;
      extern EvaluatePredicateInPrologueActiveLocalRealisation
                                       EvaluatePredicateInPrologueActiveLocal;

      extern GenericOffloadRealisation GenericOffload;
      extern CoalescedMemoryOffloadRealisation CoalescedMemoryOffload;
      extern CoalescedMemoryLocalActiveOffloadRealisation CoalescedMemoryLocalActiveOffload;
      extern CoalescedMemoryActiveLocalOffloadRealisation CoalescedMemoryActiveLocalOffload;
    } // namespace kernelrealisation

    /**
     * Run over all particles and update them independent of each other
     *
     * The routine accepts a container over particle pointers, but the
     * functor f actually accepts references. It is the responsibility of
     * this routine to map pointers onto references.
     *
     * ## Predicate
     *
     * The predicate can be used to mask out certain updates.
     *
     * We distinguish two use cases for the particle self-interactions:
     *
     * 1. We want to execute a stand-alone computation over the particle, e.g.
     *    the kick step, which is does not require any particle-particle
     *    contribution.
     * 2. We want to execute computations after and before the particle-particle
     *    interactions kernels, e.g. to 'prepare' or 'end' a given calculation
     * in SPH loops (e.g. density).
     *
     * The most popular predicates therefore are:
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * ::swift2::kernels::localParticleCanBeUpdatedInVertexKernel<typename
     * ParticleContainer::DoFType>
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * and
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * ::swift2::kernels::alwaysUpdateInVertexKernel<typename
     * ParticleContainer::DoFType>
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *
     * ## Consistency remarks
     *
     * In order to ensure the self-interaction kernels execute consistently
     * during the mesh traversals for these type of operations, the user should
     * bear in mind the difference between these two cases:
     *
     *  - The 'prepare' case should be mapped into touchVertexFirstTime(). Peano
     *    internally ensures that CellHasUpdatedParticle is false in this
     * situation.
     *  - The 'end' case should be mapped into touchVertexLastTime. Peano
     *    internally ensures that CellHasUpdatedParticle is true in this
     * situation.
     *
     * This routine does not vectorise over the particles. If any vectorisation
     * is used, you'll see the vector instructions arise from the actual compute
     * kernel.
     *
     * @param ParticleContainer A subtype of std::list<Particle *>
     */
    template <auto ParticleOp, auto PredicateOp>
    void forAllParticles(
      const peano4::datamanagement::VertexMarker& marker,
      auto&                                       assignedParticles,
      int  numberOfCoalescedAssignedParticles,
      ::swift2::kernels::kernelrealisation::GenericRealisation realisation
    );


    /**
     * Exactly the same version as the generic variant, but we use the
     * offload attribute.
     */
    template <auto ParticleOp, auto PredicateOp>
    void forAllParticles(
      const peano4::datamanagement::VertexMarker& marker,
      auto&                                       assignedParticles,
      int numberOfCoalescedAssignedParticles,
      ::swift2::kernels::kernelrealisation::GenericOffloadRealisation
    );

    /**
     * Run over all local particle-active particle combinations
     *
     * The generic implementation realises a nested loop: We call interaction
     * per pair of local and active particles. The interaction functor may
     * modify the local particle. It is the responsibility of the update
     * predicate to ensure that no particle is updated twice: For example,
     * most users will look whether a particle is contained within the
     * local cell, so we do not update it when actually the neighbour should
     * do that job, but if it is sitting right at the face in-between two
     * cells, then we also have to check a boolean that tells us where we
     * are regarding updates. The predicate also
     * should check if there's a self-interaction and mask it out if
     * appropriate.
     *
     * In Swift's terminology, we rely on a non-symmetric XXX calculation where
     * XXX is the compute kernel (such as density): Our implementations loops
     * over all local particles (outer loop) and, per local particle then over
     * all active particles (inner loop). Inside this second loop, we compute a
     * force acting from the active particle onto the local particle, but if and
     * only if the PairPredicate holds. But this knowledge is not used to update
     * the active particle in an way.
     *
     * This might seem to be a missed tuning opportunity: If a local and an
     * active particle reside on the same level, we know that the symmetric
     * force component will be computed later on, i.e. either in a subsequent
     * step of the present loop or later on when the other cell hosting the
     * other particle invokes the update kernel.
     *
     * Let L be the local one and A the active one, we have
     *
     * @f$ f(L,A) = -f(A,L). @f$
     *
     * Following the description above, this antisymmetry is not exploited in
     * the kernel. We know that
     * there will be another loop iteration or cell interaction kernel call
     * which takes care of the negative complementary force.
     *
     * It is subject (and possible) to inject the antisymmetry optimisation
     * later in the user kernel, but that's beyond scope here.
     *
     *
     * ## Multiscale interactions
     *
     * If A resides on a coarser level than L, then we have to exploit
     * the antisymmetry.
     * We need to add the force explicitly as discussed in @ref
     * page_toolbox_particles_mesh_traversal.
     *
     * This routine follows Peano's @ref page_toolbox_particles_mesh_traversal
     * "generic multiscale discussion", where we point out that we have
     * correctly take particles into account which reside on coarser levels.
     * This is necessary if some particles have larger cut-off radii than the
     * others or if we work with adaptive meshes.
     *
     *
     * ## Signatures and internal types
     *
     * The routine accepts a container over particle pointers.
     * The functor f however accepts references. It is the responsibility of
     * this routine to map pointers onto references. We use
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *   for (auto& localParticle : localParticles) {
     *      f(*localParticle);
     *   }
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *
     * ## Predicates
     *
     * This would be the generic predicates as most kernels require it:
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~
     *       UpdateParticleAssignedToCellPredicate<PCParticle<LocalParticleContainer>>
     *                 localParticlePredicate =
     * ::swift2::kernels::alwaysUpdateInCellKernel<PCParticle<LocalParticleContainer>>,
     *       UpdateParticlePairWithinCellPredicate<PCParticle<LocalParticleContainer>>
     *                 particlePairPredicate =
     * ::swift2::kernels::localParticleCanBeUpdatedInCellKernelFromAnyOtherParticle<PCParticle<LocalParticleContainer>>
     * ~~~~~~~~~~~~~~~~~~~~~~~
     *
     * However, you can use a more bespoke version as follows:
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~
     *       UpdateParticleAssignedToCellPredicate<PCParticle<LocalParticleContainer>>
     *                 localParticlePredicate =
     * ::swift2::kernels::localParticleCanBeUpdatedInCellKernel<PCParticle<LocalParticleContainer>>,
     *       UpdateParticlePairWithinCellPredicate<PCParticle<LocalParticleContainer>>
     *                 particlePairPredicate =
     * ::swift2::kernels::localParticleCanBeUpdatedInCellKernelFromAnyOtherParticle<PCParticle<LocalParticleContainer>>
     * ~~~~~~~~~~~~~~~~~~~~~~~
     *
     *
     * @param LocalParticleContainer  A subtype of std::list<Particle *>. You
     *   have no guarantee of the pointers in the list, i.e. that they are in
     *   any way consecutive.
     * @param ActiveParticleContainer A subtype of std::list<Particle *>. You
     *   have no guarantee of the pointers in the list, i.e. that they are in
     *   any way consecutive.
     * @see Variant accepting
     * ::swift2::kernels::kernelrealisation::CoalescedMemoryRealisation for a
     * discussion of memory ordering optimisation.
     */
    template <auto PairOperator, auto LocalPredicate, auto PairPredicate>
    void forAllParticlePairs(
      const swift2::CellMetaData& cellMetaData,
      auto&                       localParticles,
      auto&                       activeParticles,
      ::swift2::kernels::kernelrealisation::GenericRealisation                        realisation
    );

    /**
     * Exactly the same version as the generic variant, but with an additional
     * offload annotation.
     */
    template <auto PairOperator, auto LocalPredicate, auto PairPredicate>
    void forAllParticlePairs(
      const swift2::CellMetaData& cellMetaData,
      auto&                       localParticles,
      auto&                       activeParticles,
      ::swift2::kernels::kernelrealisation::GenericOffloadRealisation
    );

    template <auto ParticleOp, auto PredicateOp>
    void forAllParticles(
      const peano4::datamanagement::VertexMarker& marker,
      auto&                                       assignedParticles,
      int                                         numberOfAssignedParticles,
      ::swift2::kernels::kernelrealisation::CoalescedMemoryRealisation
        realisation
    );

    template <auto ParticleOp, auto PredicateOp>
    void forAllParticles(
      const peano4::datamanagement::CellMarker& marker,
      auto&                                     localParticles,
      const std::vector<int>&                   numberOfLocalParticles,
      ::swift2::kernels::kernelrealisation::CoalescedMemoryRealisation
        realisation
    );

    template <auto ParticleOp, auto PredicateOp>
    void forAllParticles(
      const peano4::datamanagement::VertexMarker& marker,
      auto&                                       assignedParticles,
      int                                         numberOfAssignedParticles,
      ::swift2::kernels::kernelrealisation::CoalescedMemoryOffloadRealisation
        realisation
    );

    template <auto ParticleOp, auto PredicateOp>
    void forAllParticles(
      const peano4::datamanagement::CellMarker& marker,
      auto&                                     localParticles,
      const std::vector<int>&                   numberOfLocalParticles,
      ::swift2::kernels::kernelrealisation::CoalescedMemoryOffloadRealisation
        realisation
    );

    template <auto PairOperator, auto LocalPredicate, auto PairPredicate>
    void forAllParticlePairs(
      const swift2::CellMetaData& cellMetaData,
      auto&                       localParticles,
      auto&                       activeParticles,
      ::swift2::kernels::kernelrealisation::
        CoalescedMemoryActiveLocalRealisation realisation
    );

    template <auto PairOperator, auto LocalPredicate, auto PairPredicate>
    void forAllParticlePairs(
      const swift2::CellMetaData& cellMetaData,
      auto&                       localParticles,
      auto&                       activeParticles,
      ::swift2::kernels::kernelrealisation::
        CoalescedMemoryActiveLocalOffloadRealisation realisation
    );

    template <auto PairOperator, auto LocalPredicate, auto PairPredicate>
    void forAllParticlePairs(
      const swift2::CellMetaData& cellMetaData,
      auto&                       localParticles,
      auto&                       activeParticles,
      ::swift2::kernels::kernelrealisation::
        EvaluatePredicateInPrologueActiveLocalRealisation realisation
    );

    template <auto PairOperator, auto LocalPredicate, auto PairPredicate>
    void forAllParticlePairs(
      const swift2::CellMetaData& cellMetaData,
      auto&                       localParticles,
      auto&                       activeParticles,
      ::swift2::kernels::kernelrealisation::
        EvaluatePredicateInPrologueLocalActiveRealisation realisation
    );

    /**
     * Simple version of pair-wise comparison exploiting coalesced memory
     *
     * Please consult the generic cousin (with a generic template argument)
     * for an explanation of the semantics.
     *
     * @image html coalesced-memory.png
     *
     * Before you continue to read, please read also the documentation
     * of forAllParticles() that accepts
 ::swift2::kernels::kernelrealisation::CoalescedMemoryRealisation
     * as argument. This routine mirrors all ideas described there. The
     * only "difference" is that both the local and active particle sets
     * are clustered, i.e. we get sequences of pointers, but we know that
     * these sequences are split into chunks with consecutive memory
     * regions.
     *
     *
     * ## Concept
     *
     * We know in this case that activeParticles is clustered into chunks
     * described by numberOfCoalescedActiveParticlesPerVertex. The same
     * reasoning holds for localParticles. Per local
     * particles it has to loop over all active particles. We
     * employ exactly the techniques described for forAllParticles(),
     * but we run over the active/local particle set chunk-wisely. Per chunk,
     * we exploit the fact that particles are stored continuously.
     *
     * ## Ordering
     *
     * In principle, the counters over active and local particles run
     * independent of each other, while the active set always contains the
     * local set. If we assume that the update function can be vectorised, we
     * see that the active set has to be the outer loop: We fix the particle
     * that works on a chunk of local particles. If we made the active set
     * the inner loop, i.e. evaluated all the impacts of a set of active
     * particles onto one local particle, we would have a write conflict
     * if vectorised.
     *
     * ## Loop fusion vs separation
     *
     * We have to check if a particle is to be updated. If so, we also have to
     * check if the symmetric evaluation has to be performed: By default, we
     * do not exploit any force symmetry on one level, as it makes bookkeeping
     * more complicated. However, coarser active particles act on finer
     * particles and immediately receive their update back, i.e. between scales
     * we exploit the symmetry.
     *
     * The generic implementation does these checks within one loop, i.e. once
     * we have found an active particle excerting a force onto a local one, we
     * immediately check if they reside on different levels and, if so, do the
     * symmetric calculation. We may assume that such multiscale relations are
     * rare. Furthermore, they have the potential to stop vectorisation as we
     * have concurrent writes to an active particle. Therefore, we could
     * split the check into two separate loops, where the second will rarely
     * ever trigger any calculation.
     *
     * While a valid rationale, we found the split version to perform
     * significantly slower than a fused one.
     *
     *
     *
     * ## Implementation
     *
     * The outer loop is kind of trivial:
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 int firstActiveParticleOfCurrentChunk  = 0;
 for (auto& activeParticleChunk: numberOfActiveParticlesPerVertex) {
   typename ActiveParticleContainer::value_type
 pFirstActiveParticleOfCurrentChunk = *activeParticles.begin() +
 firstActiveParticleOfCurrentChunk; firstActiveParticleOfCurrentChunk +=
 activeParticleChunk; for (int iActiveParticle=0;
 iActiveParticle<activeParticleChunk; iActiveParticle++) {
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * iterates over the chunks within the active sets. Per chunk, it creates
     * a pointer to the first active particles within this chunk. From hereon,
     * we know that the particles are stored continuously (in this chunk) and
     * hence can use
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * PCParticle<ActiveParticleContainer>&  activeParticle =
 *(pFirstActiveParticleOfCurrentChunk+iActiveParticle);
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * later on the reconstruct the correct active particle.
     *
     *
     */


    /**
     * Alternative implementation of CoalescedMemoryActiveLocalRealisation
     *
     * In the variant with CoalescedMemoryActiveLocalRealisation, the outer
     * loop runs over the active particles, while the inner loop runs over
     * the local ones. Here, we invert these two loops.
     */
    template <
      auto ParticlePairOp,
      auto LocalPredicateOp,
      auto ParticlePairPredicateOp>
    void forAllParticlePairs(
      const swift2::CellMetaData& cellMetaData,
      auto&                       localParticles,
      auto&                       activeParticles,
      ::swift2::kernels::kernelrealisation::
        CoalescedMemoryActiveLocalRealisation realisation
    );

    template <
      auto ParticlePairOp,
      auto LocalPredicateOp,
      auto ParticlePairPredicateOp>
    void forAllParticlePairs(
      const swift2::CellMetaData& cellMetaData,
      auto&                       localParticles,
      auto&                       activeParticles,
      ::swift2::kernels::kernelrealisation::
        CoalescedMemoryLocalActiveRealisation realisation
    );

    template <
      auto ParticlePairOp,
      auto LocalPredicateOp,
      auto ParticlePairPredicateOp>
    void forAllParticlePairs(
      const swift2::CellMetaData& cellMetaData,
      auto&                       localParticles,
      auto&                       activeParticles,
      ::swift2::kernels::kernelrealisation::
        CoalescedMemoryLocalActiveOffloadRealisation realisation
    );

    /**
     * We split up the loop into two loops: The first one runs over the
     * predicates and collects all the results in one large bit field. The
     * idea/hope is that this part vectorises. After that, we run over the
     * particle set again, but only evaluate those guys where the bitfield
     * is actually set.
     */
    template <auto ParticleOp, auto PredicateOp>
    void forAllParticles(
      const peano4::datamanagement::VertexMarker& marker,
      auto&                                       assignedParticles,
      int numberOfCoalescedAssignedParticles,
      ::swift2::kernels::kernelrealisation::
        EvaluatePredicateInPrologueRealisation
    );

    template <auto ParticleOp, auto PredicateOp>
    void forAllParticles(
      const peano4::datamanagement::CellMarker& marker,
      auto&                                     localParticles,
      const std::vector<int>&                   numberOfCoalescedLocalParticles,
      ::swift2::kernels::kernelrealisation::
        EvaluatePredicateInPrologueRealisation
    );

    /**
     * Move all the predicate evaluations into dedicated prologue
     *
     * Alternative implementation to the one with
     * ::swift2::kernels::kernelrealisation::CoalescedMemoryRealisation. We
     * run over the particles and first
     */
    template <typename LocalParticleContainer, typename ActiveParticleContainer>
    void forAllParticlePairs(
      const swift2::CellMetaData& cellMetaData,
      auto&                       localParticles,
      auto&                       activeParticles,
      ::swift2::kernels::kernelrealisation::
        EvaluatePredicateInPrologueActiveLocalRealisation realisation
    );

    template <typename LocalParticleContainer, typename ActiveParticleContainer>
    void forAllParticlePairs(
      const swift2::CellMetaData& cellMetaData,
      auto&                       localParticles,
      auto&                       activeParticles,
      ::swift2::kernels::kernelrealisation::
        EvaluatePredicateInPrologueLocalActiveRealisation realisation
    );

    namespace internal {
      /**
       * If particles on multiple levels interact, we have to be very
       * careful and ensure that multiple subcells (if mapped onto tasks)
       * do not write to the same coarse grid particles concurrently.
       */
      extern tarch::multicore::BooleanSemaphore _multiscaleInteractionSemaphore;


      /**
       * Identify interacting particles
       *
       * In several situations, we have a continuous chunk of particles and
       * want to identify which of these interact with each other due to the
       * particlePairPredicate. Besides a boolset that identifies those that
       * do interact, we also return a bitfield which identifies where we would
       * have to identify multiscale interactions.
       */
      template <
        int SubChunkSize,
        typename LocalParticle,
        typename ActiveParticle>
      std::pair<std::bitset<SubChunkSize>, std::bitset<SubChunkSize>>
        identifyInteractingParticlesFixedLocalParticle(
          const peano4::datamanagement::CellMarker& marker,
          LocalParticle*                            localParticle,
          ActiveParticle* firstActiveParticleInSubchunk,
          int             thisSubchunkSize,
          UpdateParticlePairWithinCellPredicate<LocalParticle>
            particlePairPredicate
        );

      /**
       * Counterpart to other operations, but this time we loop over the local
       * particle and keep the active one fixed.
       */
      template <
        int SubChunkSize,
        typename LocalParticle,
        typename ActiveParticle>
      std::pair<std::bitset<SubChunkSize>, std::bitset<SubChunkSize>>
        identifyInteractingParticlesFixedActiveParticle(
          const peano4::datamanagement::CellMarker& marker,
          LocalParticle*  firstLocalParticleInSubchunk,
          int             thisSubchunkSize,
          ActiveParticle* activeParticle,
          UpdateParticlePairWithinCellPredicate<LocalParticle>
            particlePairPredicate
        );
    } // namespace internal
  }   // namespace kernels
} // namespace swift2


#include "ParticleSetIterators.cpph"
