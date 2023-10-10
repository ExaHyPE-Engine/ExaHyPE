#include <iostream>
#include <iomanip>
#pragma once

/* namespace benchmarks{ */
/* namespace swift2{ */
/* namespace globaldata{ */


template <typename Particle>
static void dependencyCheckDumpDebugOutput(const Particle* localParticle,
    const benchmarks::swift2::sod::globaldata::hydroPart::AlgorithmStep AlgorithmStep,
    const benchmarks::swift2::sod::globaldata::hydroPart::SweepStage SweepStage,
    const int errors){

  using StepEnum = std::remove_pointer<Particle>::type::AlgorithmStep;
  using StageEnum = std::remove_pointer<Particle>::type::SweepStage;
  using PartClass = std::remove_pointer<Particle>::type;

  std::cout << "Particle " << localParticle->getPartid() << " - found " << errors << " dependency errors" << std::endl;
  std::cout << "Stage: " << PartClass::castAlgorithmStepToString(AlgorithmStep) << " / " << PartClass::castSweepStageToString(SweepStage) << std::endl;
  std::cout << "Particle data:" << localParticle->toString() << std::endl;

  // Set up "header" of the table
  std::cout << "Particle Dependency Checks This Step:\n";
  std::cout << std::left << std::setw(40) << "Algorithm Step " ;
  for (int s = 0; s < static_cast<int>(StageEnum::count); s++) {
    // all stages that we account for
    std::cout << PartClass::castSweepStageToString(static_cast<StageEnum>(s));
    if (s < static_cast<int>(StageEnum::count) - 1) {
      std::cout << std::setw(3) << " / ";
    }
  }
  std::cout << "\n";


  // write out what particle has actually done
  int i = 0;
  for (int a = 0; a < static_cast<int>(StepEnum::count); a++) {

    std::cout << std::left << std::setw(40) << PartClass::castAlgorithmStepToString(static_cast<StepEnum>(a));

    for (int s = 0; s < static_cast<int>(StageEnum::count); s++) {
      std::cout << std::setw(5) << localParticle->getSweepStateProgression(i);
      if (s < static_cast<int>(StageEnum::count) - 1) {
        std::cout << std::setw(3) << " / ";
      }
      i++;
    }
    std::cout << "\n";
  }


  // flush before you quit
  std::cout << std::endl;


  abort();


}


/* template <typename ParticleContainer, enum AlgorithmStep, enum SweepStage> */
template <typename ParticleContainer>
static void dependencyCheck(const ParticleContainer& localParticles,
    /* HydroPart::AlgorithmStep, */
    /* HydroPart::SweepStage){ */
    /* AlgorithmStep A, */
    /* SweepStage S){ */
    const benchmarks::swift2::sod::globaldata::hydroPart::AlgorithmStep AlgorithmStep,
    const benchmarks::swift2::sod::globaldata::hydroPart::SweepStage SweepStage){

  const int nstages = static_cast<int>(benchmarks::swift2::sod::globaldata::hydroPart::SweepStage::count);
  const int nsteps = static_cast<int>(benchmarks::swift2::sod::globaldata::hydroPart::AlgorithmStep::count);
  const int Aint = static_cast<int>(AlgorithmStep);
  const int Sint = static_cast<int>(SweepStage);

  const int index_this_stage = Aint * nstages + Sint;

  for (auto* particle : localParticles) {

    // First mark what we have just completed
    particle->setSweepStateProgression(index_this_stage, particle->getSweepStateProgression()[index_this_stage] + 1);


    // Now check everything else before this stage, and check that everything
    // that should've run until now actually has run.
    int errors = 0;
    for (int step = 0; step < Aint; step++){

      for (int stage = 0; stage < nstages; stage++) {

        const int index = step * nstages + stage;
        int partStage = particle->getSweepStateProgression()[index];

        if (partStage == 0) errors++;

      }

    }

    // for the current sweep, only check up to the current stage.
    for (int stage = 0; stage < Sint; stage++) {

      const int index = Aint * nstages + stage;
      int partStage = particle->getSweepStateProgression()[index];

      if (partStage == 0) errors++;
    }


    if (errors > 0) dependencyCheckDumpDebugOutput(particle, AlgorithmStep, SweepStage, errors);

  }
}


/* template <typename ParticleContainer, enum AlgorithmStep, enum SweepStage> */
template <typename ParticleContainer>
static void clearDependencyChecks(const ParticleContainer& localParticles){

  const int nstages = static_cast<int>(benchmarks::swift2::sod::globaldata::hydroPart::SweepStage::count);
  const int nsteps = static_cast<int>(benchmarks::swift2::sod::globaldata::hydroPart::AlgorithmStep::count);


  for (auto* particle : localParticles) {

    for (int i = 0; i < nsteps * nstages; i++){
      particle->setSweepStateProgression(i, 0);
    }
  }
}


/* } */
/* } */
/* } */
