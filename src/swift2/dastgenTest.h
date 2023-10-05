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
   * Perform all the checks on a particle's boolean members
   */
  template <typename Particle>
  void checkBooleans(Particle& particle);

  /**
   * Perform all the checks on a particle's double members
   */
  template <typename Particle>
  void checkDoubles(Particle& particle);

  /**
   * Perform all the checks on a particle's enum members
   */
  template <typename Particle>
  void checkEnums(Particle& particle);

  /**
   * Perform all the checks on a particle's integer members
   */
  template <typename Particle>
  void checkIntegers(Particle& particle);

  /**
   * Perform all the checks on a particle's string members
   */
  template <typename Particle>
  void checkStrings(Particle& particle);


  /**
   * Perform all the checks on a particle's boolean array members
   */
  template <typename Particle>
  void checkBooleanArrays(Particle& particle);

  /**
   * Perform all the checks on a particle's double array members
   */
  template <typename Particle>
  void checkDoubleArrays(Particle& particle);

  /**
   * Perform all the checks on a particle's integer array members
   */
  template <typename Particle>
  void checkIntegerArrays(Particle& particle);

  /**
   * Perform all the checks on a particle's string arrays members
   */
  template <typename Particle>
  void checkStringArrays(Particle& particle);


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
