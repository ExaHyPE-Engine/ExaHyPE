// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/fv/particles/ParticleHelper.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"

namespace exahype2::fv::particles::forces {
  /**
   * One bit per force, so a set of forces combines through the bitwise operators.
   */
  enum class ForceType : int {
    None            = 0,
    CentralGravity  = 1 << 0,
    Buoyancy        = 1 << 1,
    Drag            = 1 << 2,
    ConstantGravity = 1 << 3
  };

  INLINE_METHOD ForceType operator|(ForceType left, ForceType right);

  INLINE_METHOD ForceType operator&(ForceType left, ForceType right);

  INLINE_METHOD ForceType& operator|=(ForceType& left, ForceType right);

  INLINE_METHOD ForceType& operator&=(ForceType& left, ForceType right);

  /** Bitmask of ForceType values. setForceMethod() writes it. */
  extern int globalForceMethod;

  /** Gravitational parameter G*M of the central body. */
  extern double globalGravitationalParameter;

  /** Magnitude of the acceleration the constant gravity and the buoyancy read. */
  extern double globalGravityMagnitude;

  /** Dynamic viscosity of the fluid the Stokes drag reads. */
  extern double globalDynamicViscosity;

  void setForceMethod(ForceType forces);

  void setForceMethod(int forceMethod);

  void enableForce(ForceType force);

  void disableForce(ForceType force);

  /** Returns true if globalForceMethod carries every bit of force. */
  INLINE_METHOD bool isForceEnabled(ForceType force);

  void disableAllForces();

  void setGravitationalParameter(double gravitationalParameter);

  void setGravityMagnitude(double gravityMagnitude);

  void setDynamicViscosity(double dynamicViscosity);

  void setAllParameters(
    int    forceMethod,
    double gravitationalParameter,
    double gravityMagnitude,
    double dynamicViscosity
  );

  /** Writes the sum of the enabled forces into the particle's acceleration entries. */
  template <typename Particle>
  void applyForces(Particle& particle, tarch::la::Vector<DIMENSIONS, double> particlePosition);

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
