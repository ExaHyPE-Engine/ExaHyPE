// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/fv/particles/ParticleHelper.h"

#include <tarch/la/Vector.h>

namespace exahype2::fv::particles::forces {
  /**
   * One bit per force, so a set of forces combines through the bitwise
   * operators below.
   */
  enum class ForceType : int {
    None            = 0,
    CentralGravity  = 1 << 0,
    Buoyancy        = 1 << 1,
    Drag            = 1 << 2,
    ConstantGravity = 1 << 3
  };

  inline ForceType operator|(ForceType a, ForceType b) {
    return static_cast<ForceType>(static_cast<int>(a) | static_cast<int>(b));
  }

  inline ForceType operator&(ForceType a, ForceType b) {
    return static_cast<ForceType>(static_cast<int>(a) & static_cast<int>(b));
  }

  inline ForceType& operator|=(ForceType& a, ForceType b) {
    a = a | b;
    return a;
  }

  inline ForceType& operator&=(ForceType& a, ForceType b) {
    a = a & b;
    return a;
  }

  /** Bitmask of ForceType values. */
  extern int _forceMethod;

  /** Gravitational parameter G*M of the central body. */
  extern double _mu;

  extern double _gravityMagnitude;

  extern double _dynamicViscosity;

  extern double _fluidDensity;

  void setForceMethod(ForceType forces);

  void setForceMethod(int forceMethod);

  void enableForce(ForceType force);

  void disableForce(ForceType force);

  bool isForceEnabled(ForceType force);

  /** Reads _forceMethod without a call into the translation unit that defines it. */
  inline bool isForceEnabledInternal(ForceType force) {
    return (_forceMethod & static_cast<int>(force)) == static_cast<int>(force);
  }

  void disableAllForces();

  void setGravitationalParameter(double mu);

  void setGravityMagnitude(double g);

  void setDynamicViscosity(double viscosity);

  void setDefaultFluidDensity(double density);

  void setAllParameters(int forceMethod, double mu, double g, double viscosity, double density);

  /** Writes the sum of the enabled forces into the particle's acceleration entries. */
  template <typename Particle>
  void applyForces(Particle& particle, tarch::la::Vector<DIMENSIONS, double> pos);

  template <typename Particle>
  tarch::la::Vector<DIMENSIONS, double> computeCentralGravitationalForce(
    const tarch::la::Vector<DIMENSIONS, double>& particlePosition,
    const Particle&                              particle
  );

  template <typename Particle>
  tarch::la::Vector<DIMENSIONS, double> computeStokesDragForce(const Particle& particle);

  template <typename Particle>
  tarch::la::Vector<DIMENSIONS, double> computeBuoyancyForce(const Particle& particle);

  template <typename Particle>
  tarch::la::Vector<DIMENSIONS, double> computeGravityForce(const Particle& particle);
} // namespace exahype2::fv::particles::forces

#include "exahype2/fv/particles/ParticleForces.cpph"
