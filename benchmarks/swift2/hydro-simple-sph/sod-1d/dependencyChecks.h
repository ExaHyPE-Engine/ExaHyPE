#include <iostream>
#pragma once

/* namespace benchmarks{ */
/* namespace swift2{ */
/* namespace globaldata{ */


template <typename particle>
static void dependencyCheckDumpDebugOutput(const particle* localParticle,
    const benchmarks::swift2::sod::globaldata::hydroPart::AlgorithmStep AlgorithmStep,
    const benchmarks::swift2::sod::globaldata::hydroPart::SweepStage SweepStage,
    const int errors){


  std::cout << "Particle " << localParticle->getPartid() << " - found " << errors << " dependency errors" << std::endl;
  std::cout << "Stage: " << AlgorithmStep << " " << SweepStage << std::endl;
  std::cout << "Particle data:" << localParticle->toString() << std::endl;



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
