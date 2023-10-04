#pragma once

namespace dastgenTest {

  /**
   * Main testing function.
   * Should call getters and setters for all available variables,
   * where applicable. Also try constructor versions of particles.
   **/
  template <typename ParticleContainer>
  void checkDastgen(ParticleContainer& localParticles);

  /**
   * A dummy function to move the simulation forward in time.
   * The time step size is hard-coded such that the sim runs
   * for 2 steps. */
  template <typename Particle>
  void dummyMoveForwardInTime();


  /**
   * Report time-step size and current time at the end of a step.
   **/
  template <typename Particle>
  void reportStep(const std::string& particleName);

} // namespace dastgenTest


#include "dastgenTest.cpph"
