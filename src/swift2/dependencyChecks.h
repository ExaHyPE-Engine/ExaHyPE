#pragma once
#include <iomanip>
#include <iostream>
#include <string>
#include <type_traits>

#include "kernels/ParticleState.h"

namespace dependencyChecks {

/* #if PeanoDebug > 0 */

  /**
   * @brief This function is meant to be called on a particle on which dependency issues
   * in the main algorithm steps have been detected. It writes the full current state of
   * a problematic particle to screen and aborts the mission.
   *
   * @param localParticle: the problematic particle
   * @param algorithmStepMarkerEnum: Enum representing algorithm step particle is currently in
   * @param sweepStageMarkerEnum: Enum representing traversal sweep stage particle is currently in
   * @param errors: Number of errors that have been detected
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
  ) {

    using PartType = std::remove_pointer<Particle>::type;

    const int nsteps  = static_cast<int>(PartType::AlgorithmStep::count);
    const int nstages = static_cast<int>(PartType::SweepStage::count);


    std::cout << "-----------------------------------------\n";
    std::cout << "Main Algorithm Step Dependency check [" << call_identifier << "]\n";
    std::cout << "Particle " << particle->getPartid() << " - found " << errors << " dependency errors\n";
    std::cout
      << "Failed at Algorithm Step/Sweep Stage: " << PartType::castAlgorithmStepToString(algorithmStepMarkerEnum)
      << " / " << PartType::castSweepStageToString(sweepStageMarkerEnum) << "\n";

    // Set up "header" of the table
    std::cout << "Particle Dependency Checks This Step:\n";
    std::cout << std::left << std::setw(40) << "Algorithm Step ";
    for (int s = 0; s < nstages; s++) {
      // all stage names that we account for
      std::cout << PartType::castSweepStageToString(static_cast<PartType::SweepStage>(s));
      if (s < nstages - 1) {
        std::cout << std::setw(3) << " / ";
      }
    }
    std::cout << "\n";

    // write out what particle has actually done
    int i = 0;
    for (int a = 0; a < nsteps; a++) {

      // write out algorithm step name
      std::cout
        << std::left << std::setw(40) << PartType::castAlgorithmStepToString(static_cast<PartType::AlgorithmStep>(a));

      // write out counters for sweep stages of that algorithm step
      for (int s = 0; s < nstages; s++) {
        std::cout << std::setw(5) << particle->getAlgorithmStepSweepStateProgression(i);
        if (s < nstages - 1) {
          std::cout << std::setw(3) << " / ";
        }
        i++;
      }
      std::cout << "\n";
    }

    std::cout << "Particle data:" << particle->toString() << "\n";

    // flush before you quit
    std::cout << std::endl;

    // check for errors > 0 allows you to use this function to print the particle
    // state without exiting
    if (errors > 0) std::abort();
  }


  /**
   * @brief This function is meant to be called on a particle on which dependency issues
   * in the initialisation algorithm steps have been detected. It writes the full current state of
   * a problematic particle to screen and aborts the mission.
   *
   * @param localParticle: the problematic particle
   * @param initStepMarkerEnum: Enum representing initialisation algorithm step particle is currently in
   * @param sweepStageMarkerEnum: Enum representing traversal sweep stage particle is currently in
   * @param errors: Number of errors that have been detected
   * @param call_identifier: Additional information on which dependency check failed to be printed
   *                         to screen before exiting.
   */
  template <typename Particle>
  static void dependencyCheckInitStepDebugOutput(
    const Particle*                                          localParticle,
    typename std::remove_pointer<Particle>::type::InitStep   initStepMarkerEnum,
    typename std::remove_pointer<Particle>::type::SweepStage sweepStageMarkerEnum,
    const int                                                errors,
    const std::string                                        call_identifier
  ) {

    using PartType = std::remove_pointer<Particle>::type;

    const int nsteps  = static_cast<int>(PartType::InitStep::count);
    const int nstages = static_cast<int>(PartType::SweepStage::count);


    std::cout << "-----------------------------------------\n";
    std::cout << "Initialisation Step Dependency check [" << call_identifier << "]\n";
    std::cout << "Particle " << localParticle->getPartid() << " - found " << errors << " dependency errors\n";
    std::cout
      << "Failed at Init Step/Sweep Stage: " << PartType::castInitStepToString(initStepMarkerEnum) << " / "
      << PartType::castSweepStageToString(sweepStageMarkerEnum) << "\n";

    // Set up "header" of the table
    std::cout << "Particle Dependency Checks This Step:\n";
    std::cout << std::left << std::setw(40) << "Init Step ";
    for (int s = 0; s < nstages; s++) {
      // all stage names that we account for
      std::cout << PartType::castSweepStageToString(static_cast<PartType::SweepStage>(s));
      if (s < nstages - 1) {
        std::cout << std::setw(3) << " / ";
      }
    }
    std::cout << "\n";

    // write out what particle has actually done
    int i = 0;
    for (int a = 0; a < nsteps; a++) {

      // write out init step name
      std::cout << std::left << std::setw(40) << PartType::castInitStepToString(static_cast<PartType::InitStep>(a));

      // write out counters for sweep stages of that init step
      for (int s = 0; s < nstages; s++) {
        std::cout << std::setw(5) << localParticle->getInitStepSweepStateProgression(i);
        if (s < nstages - 1) {
          std::cout << std::setw(3) << " / ";
        }
        i++;
      }
      std::cout << "\n";
    }

    std::cout << "Particle data:" << localParticle->toString() << "\n";

    // flush before you quit
    std::cout << std::endl;

    // check for errors > 0 allows you to use this function to print the particle
    // state without exiting
    if (errors > 0) std::abort();
  }


  /**
   * @ToDo
   */
  template <typename ParticleContainer, typename Marker>
  static void dependencyMarkAlgorithmStep(
    const ParticleContainer&                                                                  localParticles,
    const Marker&                                                                             marker,
    typename std::remove_pointer<typename ParticleContainer::value_type>::type::AlgorithmStep algorithmStepMarkerEnum,
    typename std::remove_pointer<typename ParticleContainer::value_type>::type::SweepStage    sweepStageMarkerEnum,
    bool (*workOnParticle)(const benchmarks::swift2::sod::globaldata::hydroPart*, const Marker&)
  ) {

    using PartType = std::remove_pointer<typename ParticleContainer::value_type>::type;

    const int algorithmStepMarker = static_cast<int>(algorithmStepMarkerEnum);
    const int sweepStageMarker    = static_cast<int>(sweepStageMarkerEnum);

    const int nstages = static_cast<int>(PartType::SweepStage::count);
    const int nsteps  = static_cast<int>(PartType::AlgorithmStep::count);

    const int index_this_stage = algorithmStepMarker * nstages + sweepStageMarker;

    for (auto* particle : localParticles) {

if (particle->getPartid() == 800)
  std::cout << " ==== Marking particle 800 stage=" <<
  PartType::castAlgorithmStepToString(algorithmStepMarkerEnum) << "/"
    << PartType::castSweepStageToString(sweepStageMarkerEnum) <<
    " workOnParticle=" << workOnParticle(particle,marker) <<
    " parallelState " << PartType::castMoveStateToString(particle->getMoveState()) <<
    " localPartcleIsLocal " << swift2::kernels::localParticleIsLocal(particle)
    <<std::endl;


      // Mark what we have just completed, but only if the particle was actually touched!
      if (workOnParticle(particle, marker)){

        particle->setAlgorithmStepSweepStateProgression(
          index_this_stage, particle->getAlgorithmStepSweepStateProgression()[index_this_stage] + 1
        );
      } else if (particle->getMoveState() == PartType::MoveState::Moved){
        // Freshly moved particles may not be sieved correctly into being local particles yet.
        // So if `workOnParticle()` has a check whether the particle is local, it may be skipped.
        // Anticipate this, and mark it by using negative counters.
        particle->setAlgorithmStepSweepStateProgression(
          index_this_stage, particle->getAlgorithmStepSweepStateProgression()[index_this_stage] - 1
        );

      }


    }
  }


  /**
   * @ToDo
   */
  template <typename ParticleContainer, typename Marker>
  static void dependencyMarkInitStep(
    const ParticleContainer&                                                               localParticles,
    const Marker&                                                                          marker,
    typename std::remove_pointer<typename ParticleContainer::value_type>::type::InitStep   initStepMarkerEnum,
    typename std::remove_pointer<typename ParticleContainer::value_type>::type::SweepStage sweepStageMarkerEnum,
    bool (*workOnParticle)(const benchmarks::swift2::sod::globaldata::hydroPart*, const Marker&)
  ) {

    using PartType = std::remove_pointer<typename ParticleContainer::value_type>::type;

    const int initStepMarker = static_cast<int>(initStepMarkerEnum);
    const int sweepStageMarker    = static_cast<int>(sweepStageMarkerEnum);

    const int nstages = static_cast<int>(PartType::SweepStage::count);
    const int nsteps  = static_cast<int>(PartType::InitStep::count);

    const int index_this_stage = initStepMarker * nstages + sweepStageMarker;

    for (auto* particle : localParticles) {

      // Mark what we have just completed, but only if the particle was actually touched!
      if (workOnParticle(particle, marker)){

        particle->setInitStepSweepStateProgression(
          index_this_stage, particle->getInitStepSweepStateProgression()[index_this_stage] + 1
        );
      }
    }
  }




  /**
   * @brief Dependency Check: Verify that particles have completed all the algorithm steps and stages of the
   * main algorithm steps before the current one. If an error is found, debug information is printed to stdout
   * and the program is aborted.
   *
   * @param localParticles: set of local Particles to perform checks on
   * @param marker @TODO
   * @param algorithmStepMarkerEnum: Enum representing current main algorithm step
   * @param sweepStageMarkerEnum: Enum representing traversal sweep stage particle is currently in
   * @param workOnParticle @ToDo
   */
  template <typename ParticleContainer, typename Marker>
  static void dependencyCheckAlgorithmStep(
    const ParticleContainer&                                                                  localParticles,
    const Marker&                                                                             marker,
    typename std::remove_pointer<typename ParticleContainer::value_type>::type::AlgorithmStep algorithmStepMarkerEnum,
    typename std::remove_pointer<typename ParticleContainer::value_type>::type::SweepStage    sweepStageMarkerEnum,
    bool (*workOnParticle)(const benchmarks::swift2::sod::globaldata::hydroPart*, const Marker&)
  ) {

    using PartType = std::remove_pointer<typename ParticleContainer::value_type>::type;

    const int algorithmStepMarker = static_cast<int>(algorithmStepMarkerEnum);
    const int sweepStageMarker    = static_cast<int>(sweepStageMarkerEnum);

    const int nstages = static_cast<int>(PartType::SweepStage::count);
    const int nsteps  = static_cast<int>(PartType::AlgorithmStep::count);

    const int index_this_stage = algorithmStepMarker * nstages + sweepStageMarker;

    for (auto* particle : localParticles) {

if (particle->getPartid() == 800)
  std::cout << " ==== Checking particle 800 stage=" <<
  PartType::castAlgorithmStepToString(algorithmStepMarkerEnum) << "/"
    << PartType::castSweepStageToString(sweepStageMarkerEnum) << std::endl;

      // Check everything else before this stage, and check that everything
      // that should've run until now actually has run.
      int errors = 0;
      for (int step = 0; step < algorithmStepMarker; step++) {

        for (int stage = 0; stage < nstages; stage++) {

          const int index     = step * nstages + stage;
          int       partStage = particle->getAlgorithmStepSweepStateProgression()[index];

          if (partStage == 0){
std::cout << "CHECK1 " <<
  PartType::castAlgorithmStepToString(algorithmStepMarkerEnum) << "/" <<
  PartType::castSweepStageToString(sweepStageMarkerEnum) << " " << step << " / " << stage <<
  std::endl;
            errors++;
          }
        }
      }

      // for the current sweep, only check up to the current stage.
      for (int stage = 0; stage < sweepStageMarker; stage++) {

        const int index     = algorithmStepMarker * nstages + stage;
        int       partStage = particle->getAlgorithmStepSweepStateProgression()[index];

        if (partStage == 0){
std::cout << "CHECK2" << std::endl;
          errors++;
        }
      }

      // If there are any errors, print out a detailed report and exit.
      if (errors > 0)
        dependencyCheckAlgorithmStepDebugOutput(
          particle,
          algorithmStepMarkerEnum,
          sweepStageMarkerEnum,
          errors,
          "localParticle check each step and sweep stage - Something in the past has not been completed"
        );


      // Now check everything *after* this stage, and check that everything
      // that shouldn't have run until now actually hasn't run.
      for (int step = algorithmStepMarker + 1; step < nsteps; step++) {

        for (int stage = 0; stage < nstages; stage++) {

          const int index     = step * nstages + stage;
          int       partStage = particle->getAlgorithmStepSweepStateProgression()[index];

          if (partStage != 0)
            errors++;
        }
      }

      // for the current sweep, only check beginning with the next stage.
      for (int stage = sweepStageMarker + 1; stage < nstages; stage++) {

        const int index     = algorithmStepMarker * nstages + stage;
        int       partStage = particle->getAlgorithmStepSweepStateProgression()[index];

        if (partStage != 0)
          errors++;
      }

      // If there are any errors, print out a detailed report and exit.
      if (errors > 0)
        dependencyCheckAlgorithmStepDebugOutput(
          particle,
          algorithmStepMarkerEnum,
          sweepStageMarkerEnum,
          errors,
          "localParticle check each step and sweep stage - Something in the future has been done too early (or markers haven't been reset correctly)"
        );

    }
  }


  /**
   * @TODO documentation
   * @param activeParticles: set of local Particles to perform checks on
   * @param algorithmStepMarkerEnum: Enum representing current main algorithm step
   */
  template <typename ParticleContainer>
  static void dependencyCheckActiveParticlesAlgorithmStep(
    const ParticleContainer&                                                                  activeParticles,
    typename std::remove_pointer<typename ParticleContainer::value_type>::type::AlgorithmStep algorithmStepMarkerEnum
  ) {

    using PartType = std::remove_pointer<typename ParticleContainer::value_type>::type;

    const int algorithmStepMarker = static_cast<int>(algorithmStepMarkerEnum);
    const int touchVertexLastTime = static_cast<int>(PartType::SweepStage::touchVertexLastTime);

    const int nstages = static_cast<int>(PartType::SweepStage::count);
    const int nsteps  = static_cast<int>(PartType::AlgorithmStep::count);

    const int index_this_stage = algorithmStepMarker * nstages + touchVertexLastTime;

    for (auto* particle : activeParticles) {

if (particle->getPartid() == 800)
  std::cout << " ==== Checking ACTIVE PARTICLE/particle 800 stage=" <<
  PartType::castAlgorithmStepToString(algorithmStepMarkerEnum) << "/"
    << PartType::castSweepStageToString(PartType::SweepStage::touchVertexLastTime) << std::endl;

      // For local particles, we first mark down what step the particle completed.
      // Don't do that for active particles.

      // Now check everything else before this stage, and check that everything
      // that should've run until now actually has run.
      int errors = 0;
      for (int step = 0; step < algorithmStepMarker; step++) {

        for (int stage = 0; stage < nstages; stage++) {

          const int index     = step * nstages + stage;
          int       partStage = particle->getAlgorithmStepSweepStateProgression()[index];

          if (partStage == 0)
            errors++;
        }
      }

      // for the current sweep, only check up to the current stage.
      for (int stage = 0; stage < touchVertexLastTime; stage++) {

        const int index     = algorithmStepMarker * nstages + stage;
        int       partStage = particle->getAlgorithmStepSweepStateProgression()[index];

        if (partStage == 0)
          errors++;
      }

      // If there are any errors, print out a detailed report and exit.
      if (errors > 0)
        dependencyCheckAlgorithmStepDebugOutput(
          particle,
          algorithmStepMarkerEnum,
          PartType::SweepStage::touchVertexLastTime,
          errors,
          "activeParticles dependency check in TouchVertexLastTime"
        );


      // Now check everything *after* this stage, and check that everything
      // that shouldn't have run until now actually hasn't run.
      for (int step = algorithmStepMarker + 1; step < nsteps; step++) {

        for (int stage = 0; stage < nstages; stage++) {

          const int index     = step * nstages + stage;
          int       partStage = particle->getAlgorithmStepSweepStateProgression()[index];

          if (partStage != 0)
            errors++;
        }
      }

      // There shouldn't be any sweep stages after touchVertexLastTime, so no need to
      // check what would come after in this algorithm step.

      // If there are any errors, print out a detailed report and exit.
      if (errors > 0)
        dependencyCheckAlgorithmStepDebugOutput(
          particle,
          algorithmStepMarkerEnum,
          PartType::SweepStage::touchVertexLastTime,
          errors,
          "activeParticles dependency check in TouchVertexLastTime - Something in the future has been done too early (or markers haven't been reset correctly)"
        );

    }
  }


  /**
   * @brief Dependency Check: Verify that particles have completed all the algorithm steps and stages of the
   * initialisation steps before the current one. If an error is found, debug information is printed to stdout
   * and the program is aborted.
   *
   * @param localParticles: set of local Particles to perform checks on
   * @param marker @TODO
   * @param initStepMarkerEnum: Enum representing current initialisation algorithm step
   * @param sweepStageMarkerEnum: Enum representing traversal sweep stage particle is currently in
   * @param workOnParticle @ToDo
   */
  template <typename ParticleContainer, typename Marker>
  static void dependencyCheckInitStep(
    const ParticleContainer&                                                               localParticles,
    const Marker&                                                                             marker,
    typename std::remove_pointer<typename ParticleContainer::value_type>::type::InitStep   initStepMarkerEnum,
    typename std::remove_pointer<typename ParticleContainer::value_type>::type::SweepStage sweepStageMarkerEnum,
    bool (*workOnParticle)(const benchmarks::swift2::sod::globaldata::hydroPart*, const Marker&)
  ) {

    using PartType = std::remove_pointer<typename ParticleContainer::value_type>::type;

    const int initStepMarker   = static_cast<int>(initStepMarkerEnum);
    const int sweepStageMarker = static_cast<int>(sweepStageMarkerEnum);

    const int nstages = static_cast<int>(PartType::SweepStage::count);
    const int nsteps  = static_cast<int>(PartType::InitStep::count);


    const int index_this_stage = initStepMarker * nstages + sweepStageMarker;

    for (auto* particle : localParticles) {

      // Check everything else before this stage, and check that everything
      // that should've run until now actually has run.
      int errors = 0;
      for (int step = 0; step < initStepMarker; step++) {

        for (int stage = 0; stage < nstages; stage++) {

          const int index     = step * nstages + stage;
          int       partStage = particle->getInitStepSweepStateProgression()[index];

          if (partStage == 0)
            errors++;
        }
      }

      // for the current sweep, only check up to the current stage.
      for (int stage = 0; stage < sweepStageMarker; stage++) {

        const int index     = initStepMarker * nstages + stage;
        int       partStage = particle->getInitStepSweepStateProgression()[index];

        if (partStage == 0)
          errors++;
      }

      // If there are any errors, print out a detailed report and exit.
      if (errors > 0)
        dependencyCheckInitStepDebugOutput(
          particle, initStepMarkerEnum, sweepStageMarkerEnum, errors, "localParticle check each step and sweep stage"
        );

      // Now check everything *after* this stage, and check that everything
      // that shouldn't have run until now actually hasn't run.
      for (int step = initStepMarker + 1; step < nsteps; step++) {

        for (int stage = 0; stage < nstages; stage++) {

          const int index     = step * nstages + stage;
          int       partStage = particle->getInitStepSweepStateProgression()[index];

          if (partStage != 0)
            errors++;
        }
      }

      // for the current sweep, only check beginning with the next stage.
      for (int stage = sweepStageMarker + 1; stage < nstages; stage++) {

        const int index     = initStepMarker * nstages + stage;
        int       partStage = particle->getInitStepSweepStateProgression()[index];

        if (partStage != 0)
          errors++;
      }

      // If there are any errors, print out a detailed report and exit.
      if (errors > 0)
        dependencyCheckInitStepDebugOutput(
          particle,
          initStepMarkerEnum,
          sweepStageMarkerEnum,
          errors, "localParticle check each step and sweep stage - Something in the future has been done too early (or markers haven't been reset correctly)"
          );
    }
  }


  /**
   * @TODO documentation
   * @param activeParticles: set of local Particles to perform checks on
   * @param initStepMarkerEnum: Enum representing current main algorithm step
   */
  template <typename ParticleContainer>
  static void dependencyCheckActiveParticlesInitStep(
    const ParticleContainer&                                                             activeParticles,
    typename std::remove_pointer<typename ParticleContainer::value_type>::type::InitStep initStepMarkerEnum
  ) {

    using PartType = std::remove_pointer<typename ParticleContainer::value_type>::type;

    const int initStepMarker      = static_cast<int>(initStepMarkerEnum);
    const int touchVertexLastTime = static_cast<int>(PartType::SweepStage::touchVertexLastTime);

    const int nstages = static_cast<int>(PartType::SweepStage::count);
    const int nsteps  = static_cast<int>(PartType::InitStep::count);

    const int index_this_stage = initStepMarker * nstages + touchVertexLastTime;

    for (auto* particle : activeParticles) {

      // For local particles, we first mark down what step the particle completed.
      // Don't do that for active particles.

      // Now check everything else before this stage, and check that everything
      // that should've run until now actually has run.
      int errors = 0;
      for (int step = 0; step < initStepMarker; step++) {

        for (int stage = 0; stage < nstages; stage++) {

          const int index     = step * nstages + stage;
          int       partStage = particle->getInitStepSweepStateProgression()[index];

          if (partStage == 0)
            errors++;
        }
      }

      // for the current sweep, only check up to the current stage.
      for (int stage = 0; stage < touchVertexLastTime; stage++) {

        const int index     = initStepMarker * nstages + stage;
        int       partStage = particle->getInitStepSweepStateProgression()[index];

        if (partStage == 0)
          errors++;
      }

      // If there are any errors, print out a detailed report and exit.
      if (errors > 0)
        dependencyCheckInitStepDebugOutput(
          particle,
          initStepMarkerEnum,
          PartType::SweepStage::touchVertexLastTime,
          errors,
          "activeParticles dependency check in TouchVertexLastTime"
        );

      // Now check everything after this stage, and check that everything
      // that shouldn't have run until now actually hasn't run.
      for (int step = initStepMarker + 1; step < nsteps; step++) {

        for (int stage = 0; stage < nstages; stage++) {

          const int index     = step * nstages + stage;
          int       partStage = particle->getInitStepSweepStateProgression()[index];

          if (partStage != 0)
            errors++;
        }
      }

      // There shouldn't be any sweep stages after touchVertexLastTime, so no need to
      // check what would come after in this algorithm step.

      // If there are any errors, print out a detailed report and exit.
      if (errors > 0)
        dependencyCheckInitStepDebugOutput(
          particle,
          initStepMarkerEnum,
          PartType::SweepStage::touchVertexLastTime,
          errors,
          "activeParticles dependency check in TouchVertexLastTime - Something in the future has been done too early (or markers haven't been reset correctly)"
        );
    }
  }


  /**
   * @brief Clear the dependency check counters of the main algorithm step dependency checks. Intended to be called
   * first thing every new simulation step.
   *
   * @param localParticles: Set of particles to work on.
   */
  template <typename ParticleContainer>
  static void clearDependencyChecksAlgorithmStep(const ParticleContainer& localParticles) {

    using PartType    = std::remove_pointer<typename ParticleContainer::value_type>::type;
    const int nstages = static_cast<int>(PartType::SweepStage::count);
    const int nsteps  = static_cast<int>(PartType::AlgorithmStep::count);

    for (auto* particle : localParticles) {
      for (int i = 0; i < nsteps * nstages; i++) {
        particle->setAlgorithmStepSweepStateProgression(i, 0);
      }
    }
  }


  /**
   * @brief Clear the dependency check counters of the initialisation step dependency checks. Intended to be called
   * first thing every new simulation step.
   *
   * @param localParticles: Set of particles to work on.
   */
  template <typename ParticleContainer>
  static void clearDependencyChecksInitStep(const ParticleContainer& localParticles) {

    using PartType    = std::remove_pointer<typename ParticleContainer::value_type>::type;
    const int nstages = static_cast<int>(PartType::SweepStage::count);
    const int nsteps  = static_cast<int>(PartType::InitStep::count);

    for (auto* particle : localParticles) {
      for (int i = 0; i < nsteps * nstages; i++) {
        particle->setInitStepSweepStateProgression(i, 0);
      }
    }
  }

/* #else */
/* #error "You're trying to use dependency debug checks without debugging turned on." */
/* #endif */
} // namespace dependencyChecks
