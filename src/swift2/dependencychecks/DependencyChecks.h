#pragma once

#include <string>
#include <type_traits>


namespace swift2 {
  /**
   * @namespace Dependency checks
   *
   * A set of routines which do dependency checks, i.e. validate per particle
   * if the particle has completed its previous algorithmic steps before we
   * allow it to do the next state transition. Basically, this flag ensures
   * that we do not leave out particles and forget to update them or jump
   * the queue.
   *
   * The macros are automatically inserted into the code whenever you compile
   * with assertions. This happens in swift2.particle.Particle.
   */
  namespace dependencychecks {

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
     * @param particle: the problematic particle
     * @param algorithmStepMarkerEnum: Enum representing algorithm step particle is currently in
     * @param sweepStageMarkerEnum: Enum representing traversal sweep stage particle is currently in
     * @param errors: Number of errors that have been detected. If errors == 0, this function can
     *                be used to just print out the current state, and won't abort when finished.
     * @param call_identifier: Additional information on which dependency check failed to be printed
     *                         to screen before exiting.
     */
    template <typename Particle, typename AlgorithmOrInitStep>
    static std::string outputParticle(
      const Particle*                                             particle,
      AlgorithmOrInitStep                                         algorithmStepMarkerEnum,
      typename std::remove_pointer<Particle>::type::SweepStage    sweepStageMarkerEnum,
      const int                                                   errors,
      const std::string                                           call_identifier
    );


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
    template <typename ParticleContainer, typename Marker>
    static void markAlgorithmStep(
      const ParticleContainer&                                                                  assignedParticles,
      const Marker&                                                                             marker,
      typename std::remove_pointer<typename ParticleContainer::value_type>::type::AlgorithmStep algorithmStepMarkerEnum,
      typename std::remove_pointer<typename ParticleContainer::value_type>::type::SweepStage    sweepStageMarkerEnum,
      bool                                                                                      stepMovedParticle,
      bool (*workOnParticle)(const typename std::remove_pointer<typename ParticleContainer::value_type>::type*, const Marker&)
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
    template <typename ParticleContainer, typename Marker>
    static void markInitStep(
      const ParticleContainer&                                                               localParticles,
      const Marker&                                                                          marker,
      typename std::remove_pointer<typename ParticleContainer::value_type>::type::InitStep   initStepMarkerEnum,
      typename std::remove_pointer<typename ParticleContainer::value_type>::type::SweepStage sweepStageMarkerEnum,
      bool                                                                                   stepMovedParticle,
      bool (*workOnParticle)(const typename std::remove_pointer<typename ParticleContainer::value_type>::type*, const Marker&)
    );


    /**
     * @brief Check that all steps and stages before the current one have been completed.
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
    static int checkPastStages(
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
    static int checkFutureStages(
      const int  currentStep,
      const int  currentStage,
      const int  nsteps,
      const int  nstages,
      const int* sweepStateProgression
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
    template <typename ParticleContainer, typename Marker>
    static void checkAlgorithmStep(
      const ParticleContainer&                                                                  localParticles,
      const Marker&                                                                             marker,
      typename std::remove_pointer<typename ParticleContainer::value_type>::type::AlgorithmStep algorithmStepMarkerEnum,
      typename std::remove_pointer<typename ParticleContainer::value_type>::type::SweepStage    sweepStageMarkerEnum,
      bool (*workOnParticle)(const typename std::remove_pointer<typename ParticleContainer::value_type>::type*, const Marker&)
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
      const ParticleContainer&                                                                  assignedParticles,
      typename std::remove_pointer<typename ParticleContainer::value_type>::type::AlgorithmStep algorithmStepMarkerEnum
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
    template <typename ParticleContainer, typename Marker>
    static void checkInitStep(
      const ParticleContainer&                                                               localParticles,
      const Marker&                                                                          marker,
      typename std::remove_pointer<typename ParticleContainer::value_type>::type::InitStep   initStepMarkerEnum,
      typename std::remove_pointer<typename ParticleContainer::value_type>::type::SweepStage sweepStageMarkerEnum,
      bool (*workOnParticle)(const typename std::remove_pointer<typename ParticleContainer::value_type>::type*, const Marker&)
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
      const ParticleContainer&                                                             activeParticles,
      typename std::remove_pointer<typename ParticleContainer::value_type>::type::InitStep initStepMarkerEnum
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
