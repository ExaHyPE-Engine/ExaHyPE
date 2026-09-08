// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"
#include "tarch/la/VectorOperations.h"

namespace exahype2::fv::particles::utils {
  struct ParticleState {
    tarch::la::Vector<DIMENSIONS, double> position;
    tarch::la::Vector<DIMENSIONS, double> velocity;
    tarch::la::Vector<DIMENSIONS, double> acceleration;
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
  double getMass(const Particle& p);

  template <typename Particle>
  void setMass(Particle& p, double mass);

  template <typename Particle>
  double getInterpolatedRho(const Particle& p);

  template <typename Particle>
  tarch::la::Vector<DIMENSIONS, double> getVelocity(const Particle& p);

  template <typename Particle>
  void setVelocity(Particle& p, const tarch::la::Vector<DIMENSIONS, double>& v);

  template <typename Particle>
  tarch::la::Vector<DIMENSIONS, double> getAcceleration(const Particle& p);

  template <typename Particle>
  void setAcceleration(Particle& p, const tarch::la::Vector<DIMENSIONS, double>& a);

  template <typename Particle>
  void resetAcceleration(Particle& particle);

  template <typename Particle>
  double getRadius(const Particle& p);

  template <typename Particle>
  void setRadius(Particle& p, double radius);

  /** Returns 0.5 * m * v * v. */
  template <typename Particle>
  double computeKineticEnergy(const Particle& p);

  /** Returns m * v. */
  template <typename Particle>
  tarch::la::Vector<DIMENSIONS, double> computeMomentum(const Particle& p);

  template <typename Particle>
  ParticleState getParticleState(const Particle& particle);
} // namespace exahype2::fv::particles::utils

#include "exahype2/fv/particles/ParticleHelper.cpph"
