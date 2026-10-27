// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/particles/ParticleSet.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace exahype2::particles {
  enum class InterpolationScheme : int;
  enum class IntegrationScheme : int;
  enum class ForceType : int;
  struct Forces;
  struct State;
} // namespace exahype2::particles

/**
 * How a particle reads the fluid state at its position.
 */
enum class exahype2::particles::InterpolationScheme : int { None = 0, PiecewiseConstant = 1, PiecewiseLinear = 2 };

/**
 * How a particle advances over one time step.
 */
enum class exahype2::particles::IntegrationScheme : int {
  Static            = 0,
  ExplicitEuler     = 1,
  SemiImplicitEuler = 2,
  VelocityVerlet    = 3,
  Midpoint          = 4,
  RungeKutta3       = 5,
  RungeKutta4       = 6
};

/**
 * One bit per force. A set of forces is the sum of its members.
 */
enum class exahype2::particles::ForceType : int {
  None            = 0,
  CentralGravity  = 1,
  Buoyancy        = 2,
  StokesDrag      = 4,
  ConstantGravity = 8
};

TARGET_DECLARE
/**
 * Motion parameters of a tracer set. forceMethod is the sum of the enabled ForceType values.
 */
struct exahype2::particles::Forces final {
  InterpolationScheme interpolationScheme{InterpolationScheme::None};
  IntegrationScheme   integrationScheme{IntegrationScheme::Static};
  int                 forceMethod{0};
  double              gravitationalParameter{10.0};
  double              gravityMagnitude{1.0};
  double              dynamicViscosity{1.0};

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD bool enables(ForceType force) const;
};

struct exahype2::particles::State final {
  tarch::la::Vector<DIMENSIONS, double> position{};
  tarch::la::Vector<DIMENSIONS, double> velocity{};
  tarch::la::Vector<DIMENSIONS, double> acceleration{};
};

namespace exahype2::particles {
  constexpr int FluidDensityAttribute         = 0;
  constexpr int FluidVelocityAttribute        = 1;
  constexpr int ParticleVelocityAttribute     = 1 + DIMENSIONS;
  constexpr int ParticleAccelerationAttribute = 1 + 2 * DIMENSIONS;
  constexpr int ParticleMassAttribute         = 1 + 3 * DIMENSIONS;
  constexpr int ParticleRadiusAttribute       = 2 + 3 * DIMENSIONS;

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD double getMass(const Particle& particle);
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD double getRadius(const Particle& particle);
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD double getFluidDensity(const Particle& particle);

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, double> getFluidVelocity(
    const Particle& particle
  );

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, double> getVelocity(
    const Particle& particle
  );

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, double> getAcceleration(
    const Particle& particle
  );

  INLINE_METHOD GPU_CALLABLE_METHOD void setVelocity(
    Particle&                                    particle,
    const tarch::la::Vector<DIMENSIONS, double>& velocity
  );

  INLINE_METHOD GPU_CALLABLE_METHOD void setAcceleration(
    Particle&                                    particle,
    const tarch::la::Vector<DIMENSIONS, double>& acceleration
  );

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD State getState(const Particle& particle);
  INLINE_METHOD GPU_CALLABLE_METHOD void setState(Particle& particle, const State& state);

  INLINE_METHOD GPU_CALLABLE_METHOD void applyForces(
    Particle&                                    particle,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const Forces&                                forces,
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
    const tarch::la::Vector<DIMENSIONS, double>& domainWidth
  );
} // namespace exahype2::particles
TARGET_DECLARE_END

#include "exahype2/particles/Forces.cpph"
