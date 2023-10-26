#pragma once

#include <string>
#include <type_traits>

namespace swift2 {
namespace dependencyChecks {

  /**
   * @brief This function is meant to be called on a particle on which dependency issues
   * in the main algorithm steps have been detected. It writes the full current state of
   * a problematic particle to screen and aborts the mission.
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
  static void dependencyCheckAlgorithmStepDebugOutput(
    const Particle*                                             particle,
    typename std::remove_pointer<Particle>::type::AlgorithmStep algorithmStepMarkerEnum,
    typename std::remove_pointer<Particle>::type::SweepStage    sweepStageMarkerEnum,
    const int                                                   errors,
    const std::string                                           call_identifier
  );


  /**
   * @brief This function is meant to be called on a particle on which dependency issues
   * in the initialisation algorithm steps have been detected. It writes the full current state of
   * a problematic particle to screen and aborts the mission.
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
  static void dependencyCheckInitStepDebugOutput(
    const Particle*                                          particle,
    typename std::remove_pointer<Particle>::type::InitStep   initStepMarkerEnum,
    typename std::remove_pointer<Particle>::type::SweepStage sweepStageMarkerEnum,
    const int                                                errors,
    const std::string                                        call_identifier
  );


  /**
   * @brief Mark a localParticle having entered a sweep stage in a main algorithm step.
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
  static void dependencyMarkAlgorithmStep(
    const ParticleContainer&                                                                  localParticles,
    const Marker&                                                                             marker,
    typename std::remove_pointer<typename ParticleContainer::value_type>::type::AlgorithmStep algorithmStepMarkerEnum,
    typename std::remove_pointer<typename ParticleContainer::value_type>::type::SweepStage    sweepStageMarkerEnum,
    bool                                                                                      stepMovedParticle,
    bool (*workOnParticle)(const benchmarks::swift2::sod::globaldata::hydroPart*, const Marker&)
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
  static void dependencyMarkInitStep(
    const ParticleContainer&                                                               localParticles,
    const Marker&                                                                          marker,
    typename std::remove_pointer<typename ParticleContainer::value_type>::type::InitStep   initStepMarkerEnum,
    typename std::remove_pointer<typename ParticleContainer::value_type>::type::SweepStage sweepStageMarkerEnum,
    bool                                                                                   stepMovedParticle,
    bool (*workOnParticle)(const benchmarks::swift2::sod::globaldata::hydroPart*, const Marker&)
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
    const int currentStep, const int currentStage, const int nsteps, const int nstages, const int* sweepStateProgression
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
    const int currentStep, const int currentStage, const int nsteps, const int nstages, const int* sweepStateProgression
  );


  /**
   * @brief Dependency Check: Verify that particles have completed all the algorithm steps and stages of the
   * main algorithm steps before the current one. If an error is found, debug information is printed to stdout
   * and the program is aborted.
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
  static void dependencyCheckAlgorithmStep(
    const ParticleContainer&                                                                  localParticles,
    const Marker&                                                                             marker,
    typename std::remove_pointer<typename ParticleContainer::value_type>::type::AlgorithmStep algorithmStepMarkerEnum,
    typename std::remove_pointer<typename ParticleContainer::value_type>::type::SweepStage    sweepStageMarkerEnum,
    bool (*workOnParticle)(const benchmarks::swift2::sod::globaldata::hydroPart*, const Marker&)
  );


  /**
   * @brief Dependency Check: At the final stage of a sweep (TouchVertexLastTime), check that all
   * ActiveParticles have completed all their dependencies, i.e. have been localParticles at least
   * once.
   *
   * @param activeParticles: set of active Particles to perform checks on
   * @param algorithmStepMarkerEnum: Enum representing current main algorithm step
   */
  template <typename ParticleContainer>
  static void dependencyCheckActiveParticlesAlgorithmStep(
    const ParticleContainer&                                                                  activeParticles,
    typename std::remove_pointer<typename ParticleContainer::value_type>::type::AlgorithmStep algorithmStepMarkerEnum
  );


  /**
   * @brief Dependency Check: Verify that particles have completed all the algorithm steps and stages of the
   * initialisation algorithm steps before the current one. If an error is found, debug information is printed to stdout
   * and the program is aborted.
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
  static void dependencyCheckInitStep(
    const ParticleContainer&                                                               localParticles,
    const Marker&                                                                          marker,
    typename std::remove_pointer<typename ParticleContainer::value_type>::type::InitStep   initStepMarkerEnum,
    typename std::remove_pointer<typename ParticleContainer::value_type>::type::SweepStage sweepStageMarkerEnum,
    bool (*workOnParticle)(const benchmarks::swift2::sod::globaldata::hydroPart*, const Marker&)
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
  static void dependencyCheckActiveParticlesInitStep(
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


} // namespace dependencyChecks
} // namespace swift2

#include "dependencyChecks.cpph"
