#pragma once
#include <iostream>
#include <iomanip>
#include <type_traits>

namespace dependencyChecks{

template <typename Particle>
static void dependencyCheckDumpDebugOutput(const Particle* localParticle,
    typename std::remove_pointer<Particle>::type::AlgorithmStep algorithmStepMarkerEnum,
    typename std::remove_pointer<Particle>::type::SweepStage sweepStageMarkerEnum,
    const int errors){

#if PeanoDebug > 0

  using PartType = std::remove_pointer<Particle>::type;

  const int nsteps = static_cast<int>(PartType::AlgorithmStep::count);
  const int nstages = static_cast<int>(PartType::SweepStage::count);


  std::cout << "-----------------------------------------\n";
  std::cout << "Particle " << localParticle->getPartid() << " - found " << errors << " dependency errors\n";
  std::cout << "Failed at Algorithm Step/Sweep Stage: " << PartType::castAlgorithmStepToString(algorithmStepMarkerEnum) << " / " << PartType::castSweepStageToString(sweepStageMarkerEnum) << "\n";
  std::cout << "Particle data:" << localParticle->toString() << "\n";

  // Set up "header" of the table
  std::cout << "Particle Dependency Checks This Step:\n";
  std::cout << std::left << std::setw(40) << "Algorithm Step " ;
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
    std::cout << std::left << std::setw(40) << PartType::castAlgorithmStepToString(static_cast<PartType::AlgorithmStep>(a));

    // write out counters for sweep stages of that algorithm step
    for (int s = 0; s < nstages; s++) {
      std::cout << std::setw(5) << localParticle->getSweepStateProgression(i);
      if (s < nstages - 1) {
        std::cout << std::setw(3) << " / ";
      }
      i++;
    }
    std::cout << "\n";
  }

  // flush before you quit
  std::cout << std::endl;

  // TODO: is this the proper way of emergency exiting?
  abort();

#endif
}


/* template <typename ParticleContainer, enum AlgorithmStep, enum SweepStage> */
template <typename ParticleContainer>
static void dependencyCheck(const ParticleContainer& localParticles,
    typename std::remove_pointer<typename ParticleContainer::value_type>::type::AlgorithmStep algorithmStepMarkerEnum,
    typename std::remove_pointer<typename ParticleContainer::value_type>::type::SweepStage sweepStageMarkerEnum
    ){

#if PeanoDebug > 0

  using PartType = std::remove_pointer<typename ParticleContainer::value_type>::type;

  const int algorithmStepMarker = static_cast<int>(algorithmStepMarkerEnum);
  const int sweepStageMarker = static_cast<int>(sweepStageMarkerEnum);

  const int nstages = static_cast<int>(PartType::SweepStage::count);
  const int nsteps = static_cast<int>(PartType::AlgorithmStep::count);


  const int index_this_stage = algorithmStepMarker * nstages + sweepStageMarker;

  for (auto* particle : localParticles) {

    // First mark what we have just completed
    particle->setSweepStateProgression(index_this_stage, particle->getSweepStateProgression()[index_this_stage] + 1);


    // Now check everything else before this stage, and check that everything
    // that should've run until now actually has run.
    int errors = 0;
    for (int step = 0; step < algorithmStepMarker; step++){

      for (int stage = 0; stage < nstages; stage++) {

        const int index = step * nstages + stage;
        int partStage = particle->getSweepStateProgression()[index];

        if (partStage == 0) errors++;

      }
    }

    // for the current sweep, only check up to the current stage.
    for (int stage = 0; stage < sweepStageMarker; stage++) {

      const int index = algorithmStepMarker * nstages + stage;
      int partStage = particle->getSweepStateProgression()[index];

      if (partStage == 0) errors++;
    }

    // If there are any errors, print out a detailed report and exit.
    if (errors > 0) dependencyCheckDumpDebugOutput(particle, algorithmStepMarkerEnum, sweepStageMarkerEnum, errors);

  }
#else
#error "You're trying to use dependency debug checks without debugging turned on."
#endif
}


template <typename ParticleContainer>
static void clearDependencyChecks(const ParticleContainer& localParticles){

#if PeanoDebug > 0
  using PartType = std::remove_pointer<typename ParticleContainer::value_type>::type;
  const int nstages = static_cast<int>(PartType::SweepStage::count);
  const int nsteps = static_cast<int>(PartType::AlgorithmStep::count);

  for (auto* particle : localParticles) {
    for (int i = 0; i < nsteps * nstages; i++){
      particle->setSweepStateProgression(i, 0);
    }
  }

#endif
}


}
