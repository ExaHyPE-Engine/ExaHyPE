#pragma once

#include <string>
#include <type_traits>

#include "swift2/kernels/ParticleUpdatePredicates.h"


namespace swift2 {
  /**
   * @namespace Dependency checks
   *
   * A set of routines which do dependency checks, i.e. validate per particle
   * if the particle has completed its previous algorithmic steps before we
   * allow it to do the next state transition. Basically, the checks ensure
   * that we do not leave out particles and forget to update them or jump
   * the queue.
   *
   * The macros are automatically inserted into the code whenever you compile
   * with assertions. This happens in swift2.particle.Particle.
   *
   * Different algorithmic steps might have different access policies. There
   * are three different flags in swift2.particle.AlgorithmStep. By default,
   * they are set to None. swift2.particle.Particle._dependency_checks_modify_steps()
   * will then insert default policies/invariants. But you might want to alter
   * these.
   */
  namespace dependencychecks {
    /**
     * Different invariants that have to hold for the code. They are checked by
     * isValid() which is called by the mark... routines and checkPastStages.
     */
    enum class Invariant {
      TouchFirstUsage_MaskOutAfterwards_AllPreviousStepsUpdateAtLeastOnce,
      TouchFirstUsage_MaskOutAfterwards_AllPreviousStepsUpdateAtLeastOnce_SweepMayRerun,
      TouchAtMostOnce_MaskOutOtherwise_AllPreviousStepsUpdateAtLeastOnce,
      TouchAtMostOnce_MaskOutOtherwise_AllPreviousStepsUpdateAtLeastOnce_SweepMayRerun,
      TouchAtLeastOnce_AllPreviousStepsUpdateAtLeastOnce,
      TouchAtLeastOnce_AllPreviousStepsUpdateAtLeastOnce_MayOverwritePreviousCellUpdatesFromSameSweep
    };

    std::string toString(Invariant policy);

    namespace internal {
      /**
       * Is current state that's to be updated valid
       *
       * This routine is called by the mark operations after(!) it has altered
       * the two counters numberOfUpdates and numberOfMaskOuts. The actual
       * validation usually is enforced by assertions, but this has to happen
       * outside. The routine here only determines if a state is valid or not.
       */
      bool isValid(
        Invariant policy,
        bool      workOnParticle,
        bool      particleIsLocal,
        int       numberOfUpdates,
        int       numberOfMaskOuts
      );

      /**
       * Do all previous steps have to alter state at least once.
       */
      bool previousStepsAllHaveToAccessAtLeastOnce(Invariant policy);

      /**
       * Which future steps should be checked
       *
       * This routine returns the relative step in the future which has
       * to be cleared at this point (so no updated or masked out marker set).
       * Invoked in the Nth step and return n, we check all steps starting from
       * N+n if they are cleared.
       *
       * If you return 0, then this implies that all future steps have to be
       * cleared. If you return numberOfSteps, we don't check any
       * future step.
       */
      int firstFutureStepThatHasToBeCleared(Invariant policy, int numberOfSteps);

      /**
       * @brief This function is meant to be called on a particle on which dependency issues
       * in the main algorithm steps have been detected.
       *
       *
       * If the consistency checks fail, this routine should be used to write the
       * full current state of a problematic particle to screen. After that, the
       * routine aborts.
       *
       * It is important that we use the log functionality of tarch::logging, as
       * it allows us to distinguish from which rank messages come from. It also
       * enables us to integrate the messages into mainstream performance
       * analysis workflows.
       *
       * Even though it might seem to be appealing, the output routine may not
       * contain any assertion: We use it within assertions, so if there's
       * another assertion in there, we won't get the full output.
       *
       *
       * ## Realisation
       *
       * I originally inserted some assertions of the type
       *
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~
       * assertion3(particle->getDependencyChecksAlgorithmStepUpdates(i) >= 0, particle->getDependencyChecksAlgorithmStepUpdates(i), i, particle->toString() );
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~
       *
       * into the source code. While this seems to be convenient (check that
       * there's no garbage here), it is the wrong place to add these
       * assertions: If an assertion fails due to garbage somewhere and wants
       * to plot the particle status, then the plotting itself should not raise
       * yet another assertion. Therefore, I now dump the value as "invalid"
       * and outsource the actual consistency check ensuring that a value is
       * not invalid to checkFutureStages() and checkPastStages().
       *
       *
       *
       *
       * @param particle: the problematic particle
       * @param algorithmStepMarkerEnum: Enum representing algorithm step particle is currently in
       * @param sweepStageMarkerEnum: Enum representing traversal sweep stage particle is currently in
       * @param errors: Number of errors that have been detected. If errors == 0, this function can
       *                be used to just print out the current state, and won't abort when finished.
       * @param call_identifier: Additional information on which dependency check failed to be printed
       *                         to screen before exiting.
       */
      template <typename Particle>
      static std::string outputParticle(
        const Particle*                                                                        particle,
        typename std::remove_pointer<Particle>::type::DependencyChecksAlgorithmStepLastUpdated algorithmStep,
        typename std::remove_pointer<Particle>::type::DependencyChecksPeanoEventUsedBySwift    sweepStageMarkerEnum,
        const int                                                                              errors
      );

      template <typename Particle>
      static std::string outputParticle(
        const Particle*                                                                     particle,
        typename std::remove_pointer<Particle>::type::DependencyChecksInitStepLastUpdated   initStep,
        typename std::remove_pointer<Particle>::type::DependencyChecksPeanoEventUsedBySwift sweepStageMarkerEnum,
        const int                                                                           errors
      );

      /**
       * @brief Check that all previous steps and stages
       *
       * If we are given a non-local particle, we can only check the current
       * stage and see if it has not been touched at all. Particles may change
       * their state between two steps, so we cannot really say anything about
       *
       * If the policy says previousStepsAllHaveToAccessAtLeastOnce(),
       * Ibefore the current one have been completed.
       *
       * @param currentStep: integer value of the current algorithm step
       * @param currentStage: integer value of the current sweep stage
       * @param nsteps: Total number of algorithm steps
       * @param nstages: Total number of sweep stages
       * @param sweepStateProgression: integer array carrying flags/counters of completed sweep
       *                               stages and algorithm steps of a particle
       *
       * @return errors: Number of dependency errors detected
       */
      int checkPastStages(
        Invariant  policy,
        bool       isLocal,
        const int  currentStep,
        const int  currentStage,
        const int  nsteps,
        const int  nstages,
        const int* sweepStateProgression
      );


      /**
       * @brief Check that all steps and stages after the current one have not been done yet.
       *
       * @param currentStep: integer value of the current algorithm step
       * @param currentStage: integer value of the current sweep stage
       * @param nsteps: Total number of algorithm steps
       * @param nstages: Total number of sweep stages
       * @param sweepStateProgression: integer array carrying flags/counters of completed sweep
       *                               stages and algorithm steps of a particle
       *
       * @return errors: Number of dependency errors detected
       */
      int checkFutureStages(
        Invariant  policy,
        const int  currentStep,
        const int  currentStage,
        const int  nsteps,
        const int  nstages,
        const int* sweepStateProgression
      );
    } // namespace internal

    /**
     * @brief Mark a localParticle having entered a sweep stage in a main algorithm step.
     *
     * This routine is used both by touchVertexFirstTime and
     * touchVertexLastTime to keep track of state transitions. We run over the
     * particles and check for each one if the routine would update it. If so,
     * they increment the counter getAlgorithmStepSweepStateProgression(). Any
     * positive value here counts how often particles are updated. If a
     * particle is not to be updated, we decrement the counter. Obviously, a
     * particle should either be ignored all the time, or it should be updated.
     * There's one exception to this rule: If a routine updates a particle's
     * position, we might update the particle at one point, alter its position,
     * then re-assign it to a new particle and there mask it out.
     *
     * @param assignedParticles: the particles around a vertex that will be subject to the validation
     * @param marker: a peano4::datamanagement::VertexMarker or peano4::datamanagement::CellMarker
     * @param algorithmStepMarkerEnum: Enum representing algorithm step particle is currently in
     * @param sweepStageMarkerEnum: Enum representing traversal sweep stage particle is currently in
     * @param stepMovedParticle whether the particle has been moved in this (or in the previous)
     *                          algorithm step. This may corrupt the set of localParticles. See
     *                          @page_toolbox_particles_mesh_traversal.
     * @param workOnParticle A function used to determine whether the particle is being worked on in
     *                       the specific sweep stage of the algorithm step. As arguments, it takes
     *                       the particle itself and the marker. This should be the same function used
     *                       in the actual stage of the algorithm step as your code, and should be one
     *                       of the functions defined in src/swift2/kernels/ParticleState.h
     */
    template <typename ParticleContainer>
    static void markAlgorithmStep(
      const ParticleContainer&                    particles,
      const peano4::datamanagement::VertexMarker& marker,
      typename std::remove_pointer<
        typename ParticleContainer::value_type>::type::DependencyChecksAlgorithmStepLastUpdated algorithmStepMarkerEnum,
      typename std::remove_pointer<typename ParticleContainer::value_type>::type::DependencyChecksPeanoEventUsedBySwift
                sweepStageMarkerEnum,
      Invariant                                   policy,
      ::swift2::kernels::UpdateParticleAssignedToVertexPredicate<typename std::remove_pointer<typename ParticleContainer::value_type>::type>
                workOnParticle,
      int spacetreeId
    );


    /**
     * A particle is marked if there is at least one active interaction in this cell.
     *
     *
     */
    template <typename ParticleContainer>
    static void markAlgorithmStep(
      const ParticleContainer&                    localParticles,
      const ParticleContainer&                    activeParticles,
      const peano4::datamanagement::CellMarker&   marker,
      typename std::remove_pointer<
        typename ParticleContainer::value_type>::type::DependencyChecksAlgorithmStepLastUpdated algorithmStepMarkerEnum,
      typename std::remove_pointer<typename ParticleContainer::value_type>::type::DependencyChecksPeanoEventUsedBySwift
                sweepStageMarkerEnum,
      Invariant                                   policy,
      ::swift2::kernels::UpdateParticlePairWithinCellPredicate<typename std::remove_pointer<typename ParticleContainer::value_type>::type>     workOnParticle,
      int spacetreeId
    );


    /**
     * @brief Mark a localParticle having entered a sweep stage in a initialisation algorithm step.
     *
     * @param localParticle: the problematic particle
     * @param marker: a peano4::datamanagement::VertexMarker or peano4::datamanagement::CellMarker
     * @param algorithmStepMarkerEnum: Enum representing algorithm step particle is currently in
     * @param sweepStageMarkerEnum: Enum representing traversal sweep stage particle is currently in
     * @param stepMovedParticle whether the particle has been moved in this (or in the previous)
     *                          algorithm step. This may corrupt the set of localParticles. See
     *                          @page_toolbox_particles_mesh_traversal.
     * @param workOnParticle A function used to determine whether the particle is being worked on in
     *                       the specific sweep stage of the algorithm step. As arguments, it takes
     *                       the particle itself and the marker. This should be the same function used
     *                       in the actual stage of the algorithm step as your code, and should be one
     *                       of the functions defined in src/swift2/kernels/ParticleState.h
     */
    template <typename ParticleContainer>
    static void markInitStep(
      const ParticleContainer& localParticles,
      const peano4::datamanagement::VertexMarker&            marker,
      typename std::remove_pointer<typename ParticleContainer::value_type>::type::DependencyChecksInitStepLastUpdated
        initStepMarkerEnum,
      typename std::remove_pointer<typename ParticleContainer::value_type>::type::DependencyChecksPeanoEventUsedBySwift
                sweepStageMarkerEnum,
      Invariant policy,
      ::swift2::kernels::UpdateParticleAssignedToVertexPredicate<typename std::remove_pointer<typename ParticleContainer::value_type>::type>     workOnParticle,
      int spacetreeId
    );


    /**
     * A particle is marked if there is at least one active interaction in this cell.
     *
     *
     */
    template <typename ParticleContainer>
    static void markInitStep(
      const ParticleContainer& localParticles,
      const ParticleContainer& activeParticles,
      const peano4::datamanagement::CellMarker&            marker,
      typename std::remove_pointer<typename ParticleContainer::value_type>::type::DependencyChecksInitStepLastUpdated
        initStepMarkerEnum,
      typename std::remove_pointer<typename ParticleContainer::value_type>::type::DependencyChecksPeanoEventUsedBySwift
                sweepStageMarkerEnum,
      Invariant policy,
      ::swift2::kernels::UpdateParticlePairWithinCellPredicate<typename std::remove_pointer<typename ParticleContainer::value_type>::type>     workOnParticle,
      int spacetreeId
    );


    /**
     * @brief Dependency Check: Verify that particles have completed all the algorithm steps and stages of the
     * main algorithm steps before the current one. Also ensure none of the steps nor stages after the current
     * one have been performed yet. If an error is found, debug information is printed to stdout
     * and the program is aborted.
     *
     * This routine is typically inserted after each sweep stage (touchVertexFirstTime, CellKernel,
     * touchVertesLastTime) of any AlgorithmStep. It ensures that all data piped into an algorithm step are valid
     * and consistent.
     *
     * @param localParticles: set of local Particles to perform checks on
     * @param marker: a peano4::datamanagement::VertexMarker or peano4::datamanagement::CellMarker
     * @param algorithmStepMarkerEnum: Enum representing current main algorithm step
     * @param sweepStageMarkerEnum: Enum representing traversal sweep stage particle is currently in
     * @param workOnParticle A function used to determine whether the particle is being worked on in
     *                       the specific sweep stage of the algorithm step. As arguments, it takes
     *                       the particle itself and the marker. This should be the same function used
     *                       in the actual stage of the algorithm step as your code, and should be one
     *                       of the functions defined in src/swift2/kernels/ParticleState.h
     */
    template <typename ParticleContainer>
    static void checkAlgorithmStep(
      Invariant                policy,
      const ParticleContainer& localParticles,
      const ParticleContainer& activeParticles,
      const peano4::datamanagement::CellMarker&            marker,
      typename std::remove_pointer<
        typename ParticleContainer::value_type>::type::DependencyChecksAlgorithmStepLastUpdated algorithmStepMarkerEnum,
      typename std::remove_pointer<typename ParticleContainer::value_type>::type::DependencyChecksPeanoEventUsedBySwift
        sweepStageMarkerEnum,
        ::swift2::kernels::UpdateParticlePairWithinCellPredicate<typename std::remove_pointer<typename ParticleContainer::value_type>::type>     workOnParticle,
      int spacetreeId
    );


    template <typename ParticleContainer>
    static void checkAlgorithmStep(
      Invariant                policy,
      const ParticleContainer& localParticles,
      const peano4::datamanagement::VertexMarker&            marker,
      typename std::remove_pointer<
        typename ParticleContainer::value_type>::type::DependencyChecksAlgorithmStepLastUpdated algorithmStepMarkerEnum,
      typename std::remove_pointer<typename ParticleContainer::value_type>::type::DependencyChecksPeanoEventUsedBySwift
        sweepStageMarkerEnum,
      ::swift2::kernels::UpdateParticleAssignedToVertexPredicate<typename std::remove_pointer<typename ParticleContainer::value_type>::type>     workOnParticle,
      int spacetreeId
    );


    /**
     * @brief Dependency Check: At the final stage of a sweep, i.e. touchVertexLastTime(), check that all
     * particles hosted by this vertex have completed all their dependencies, i.e. have been localParticles at least
     * once.
     *
     * Within the vertex events touchVertexFirstTime() and
     * touchVertexLastTime(), we have no active and local particles anymore.
     * There are solely the ones around the vertex on this mesh level. They are
     * called assignedParticles.
     *
     * Throughout the traversal, there are no global active and local
     * particles: Both sets are views onto the global particle set and they adopt
     * per cell. These views are not even exclusive, i.e. local particles are
     * always a subset of active ones. We know that cells update only local
     * particles. Within a subsequent touchVertexLastTime(), every particles has
     * have to be updated, i.e. every particle has been local at least once.
     * Therefore, we write that all particles have to have been local at least
     * once.
     *
     * @param assignedParticles: set of particles hosted by a vertex to perform checks on
     * @param algorithmStepMarkerEnum: Enum representing current main algorithm step
     */
    template <typename ParticleContainer>
    static void checkParticlesAssignedToVertexInTouchLastTimeAlgorithmStep(
      Invariant                policy,
      const ParticleContainer& assignedParticles,
      typename std::remove_pointer<
        typename ParticleContainer::value_type>::type::DependencyChecksAlgorithmStepLastUpdated algorithmStepMarkerEnum,
        int spacetreeId
    );


    /**
     * @brief Dependency Check: Verify that particles have completed all the algorithm steps and stages of the
     * initialisation algorithm steps before the current one. Also ensure none of the steps nor stages after the current
     * one have been performed yet. If an error is found, debug information is printed to stdout
     * and the program is aborted.
     *
     * This routine is typically inserted after each sweep stage (touchVertexFirstTime, CellKernel,
     * touchVertesLastTime) of any AlgorithmStep during the initialisation. It ensures that all data
     * piped into an algorithm step are valid and consistent.
     *
     * @param localParticles: set of local Particles to perform checks on
     * @param marker: a peano4::datamanagement::VertexMarker or peano4::datamanagement::CellMarker
     * @param initStepMarkerEnum: Enum representing current initialisation algorithm step
     * @param sweepStageMarkerEnum: Enum representing traversal sweep stage particle is currently in
     * @param workOnParticle A function used to determine whether the particle is being worked on in
     *                       the specific sweep stage of the algorithm step. As arguments, it takes
     *                       the particle itself and the marker. This should be the same function used
     *                       in the actual stage of the algorithm step as your code, and should be one
     *                       of the functions defined in src/swift2/kernels/ParticleState.h
     */
    template <typename ParticleContainer>
    static void checkInitStep(
      Invariant                policy,
      const ParticleContainer& localParticles,
      const peano4::datamanagement::VertexMarker&            marker,
      typename std::remove_pointer<typename ParticleContainer::value_type>::type::DependencyChecksInitStepLastUpdated
        initStepMarkerEnum,
      typename std::remove_pointer<typename ParticleContainer::value_type>::type::DependencyChecksPeanoEventUsedBySwift
        sweepStageMarkerEnum,
      ::swift2::kernels::UpdateParticleAssignedToVertexPredicate<typename std::remove_pointer<typename ParticleContainer::value_type>::type>     workOnParticle,
      int spacetreeId
    );


    template <typename ParticleContainer>
    static void checkInitStep(
      Invariant                policy,
      const ParticleContainer& localParticles,
      const ParticleContainer& activeParticles,
      const peano4::datamanagement::CellMarker&            marker,
      typename std::remove_pointer<typename ParticleContainer::value_type>::type::DependencyChecksInitStepLastUpdated
        initStepMarkerEnum,
      typename std::remove_pointer<typename ParticleContainer::value_type>::type::DependencyChecksPeanoEventUsedBySwift
        sweepStageMarkerEnum,
      ::swift2::kernels::UpdateParticlePairWithinCellPredicate<typename std::remove_pointer<typename ParticleContainer::value_type>::type>     workOnParticle,
      int spacetreeId
    );

    /**
     * @brief Dependency Check: At the final stage of a sweep (TouchVertexLastTime), check that all
     * ActiveParticles have completed all their dependencies, i.e. have been localParticles at least
     * once. This
     *
     * @param activeParticles: set of active Particles to perform checks on
     * @param initStepMarkerEnum: Enum representing current initialisation algorithm step
     */
    template <typename ParticleContainer>
    static void checkParticlesAssignedToVertexInTouchLastTimeInitStep(
      Invariant                policy,
      const ParticleContainer& activeParticles,
      typename std::remove_pointer<typename ParticleContainer::value_type>::type::DependencyChecksInitStepLastUpdated
        initStepMarkerEnum,
      int spacetreeId
    );


    /**
     * @brief Clear the dependency check counters of the main algorithm step dependency checks. Intended to be called
     * first thing every new simulation step.
     *
     * @param localParticles: Set of particles to work on.
     */
    template <typename ParticleContainer>
    static void clearDependencyChecksAlgorithmStep(const ParticleContainer& localParticles);


    /**
     * @brief Clear the dependency check counters of the initialisation step dependency checks. Intended to be called
     * first thing during initialisation.
     *
     * @param localParticles: Set of particles to work on.
     */
    template <typename ParticleContainer>
    static void clearDependencyChecksInitStep(const ParticleContainer& localParticles);


  } // namespace dependencychecks
} // namespace swift2


#include "../dependencychecks/DependencyChecks.cpph"
