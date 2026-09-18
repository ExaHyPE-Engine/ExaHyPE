// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"
#include "tarch/la/VectorOperations.h"

namespace exahype2::fv::particles::utils {
  /**
   * A particle stores the interpolated fluid density in its first data entry, then one entry per
   * axis for the interpolated fluid velocity, the particle velocity and the particle acceleration,
   * and closes with the mass and the radius.
   */
  inline constexpr int FluidDensityAttributeOffset         = 0;
  inline constexpr int FluidVelocityAttributeOffset        = 1;
  inline constexpr int ParticleVelocityAttributeOffset     = 1 + DIMENSIONS;
  inline constexpr int ParticleAccelerationAttributeOffset = 1 + 2 * DIMENSIONS;
  inline constexpr int ParticleMassAttributeOffset         = 1 + 3 * DIMENSIONS;
  inline constexpr int ParticleRadiusAttributeOffset       = 2 + 3 * DIMENSIONS;

  struct ParticleState {
    tarch::la::Vector<DIMENSIONS, double> position{};
    tarch::la::Vector<DIMENSIONS, double> velocity{};
    tarch::la::Vector<DIMENSIONS, double> acceleration{};
  };

  extern tarch::la::Vector<DIMENSIONS, double> domainOffset;
  extern tarch::la::Vector<DIMENSIONS, double> domainSize;
  extern tarch::la::Vector<DIMENSIONS, int>    isPeriodic;

  /** Overwrites domainOffset, domainSize and isPeriodic. */
  void setGlobalParameters(
    const tarch::la::Vector<DIMENSIONS, double>& offset,
    const tarch::la::Vector<DIMENSIONS, double>& size,
    const tarch::la::Vector<DIMENSIONS, int>&    periodic
  );

  template <typename Particle>
  double getMass(const Particle& particle);

  template <typename Particle>
  void setMass(Particle& particle, double mass);

  template <typename Particle>
  double getInterpolatedRho(const Particle& particle);

  template <typename Particle>
  tarch::la::Vector<DIMENSIONS, double> getVelocity(const Particle& particle);

  template <typename Particle>
  void setVelocity(Particle& particle, const tarch::la::Vector<DIMENSIONS, double>& velocity);

  template <typename Particle>
  tarch::la::Vector<DIMENSIONS, double> getAcceleration(const Particle& particle);

  template <typename Particle>
  void setAcceleration(Particle& particle, const tarch::la::Vector<DIMENSIONS, double>& acceleration);

  template <typename Particle>
  void resetAcceleration(Particle& particle);

  template <typename Particle>
  double getRadius(const Particle& particle);

  template <typename Particle>
  void setRadius(Particle& particle, double radius);

  /** Returns half of the mass times the squared velocity. */
  template <typename Particle>
  double computeKineticEnergy(const Particle& particle);

  /** Returns the mass times the velocity. */
  template <typename Particle>
  tarch::la::Vector<DIMENSIONS, double> computeMomentum(const Particle& particle);

  template <typename Particle>
  ParticleState getParticleState(const Particle& particle);

  template <typename Particle>
  void setParticleState(Particle& particle, const ParticleState& state);
} // namespace exahype2::fv::particles::utils

#include "exahype2/fv/particles/ParticleHelper.cpph"
