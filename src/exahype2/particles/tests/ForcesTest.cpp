// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ForcesTest.h"

#include "exahype2/particles/Forces.h"
#include "exahype2/particles/Integration.h"
#include "exahype2/particles/ParticleSet.h"
#include "tarch/la/Scalar.h"
#include "tarch/la/Vector.h"

namespace {
  constexpr int NumberOfData = exahype2::particles::ParticleRadiusAttribute + 1;

  const tarch::la::Vector<DIMENSIONS, double> DomainOffset(0.0);
  const tarch::la::Vector<DIMENSIONS, double> DomainWidth(1.0);

  struct Values final {
    std::array<double, DIMENSIONS + NumberOfData> values{};

    Values() {
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        values[static_cast<std::size_t>(axis)] = 0.5;
      }
      values[DIMENSIONS + exahype2::particles::ParticleMassAttribute]   = 1.0;
      values[DIMENSIONS + exahype2::particles::ParticleRadiusAttribute] = 0.5;
    }

    exahype2::particles::Particle getParticle() { return exahype2::particles::Particle{values.data(), NumberOfData}; }
  };

  const auto NoInterpolation = [](
                                 [[maybe_unused]] exahype2::particles::Particle&               particle,
                                 [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position
                               ) {};
} // namespace

exahype2::particles::tests::ForcesTest::ForcesTest() :
  tarch::tests::TestCase("exahype2::particles::tests::ForcesTest") {}

void
exahype2::particles::tests::ForcesTest::run() {
  testMethod(testAMasslessParticleCarriesNoAcceleration);
  testMethod(testEveryForceWritesItsAcceleration);
  testMethod(testTheEulerSchemesAdvanceOneStep);
  testMethod(testTheHigherOrderSchemesAreExactForAConstantAcceleration);
  testMethod(testHigherOrderSchemesConvergeForDrag);
  testMethod(testStaticIntegrationNeedsNoDataAttributes);
  testMethod(testReflectionAndWrapAtTheWalls);
}

void
exahype2::particles::tests::ForcesTest::testHigherOrderSchemesConvergeForDrag() {
  exahype2::particles::Forces forces;
  forces.forceMethod                = static_cast<int>(exahype2::particles::ForceType::StokesDrag);
  forces.dynamicViscosity           = 1.0 / (3.0 * tarch::la::PI);
  constexpr double EndTime          = 0.5;
  const double     decayRate        = 3.0 * tarch::la::PI * forces.dynamicViscosity + 1e-12;
  const double     exactVelocity    = std::exp(-decayRate * EndTime);
  const auto       getVelocityError = [&](const int steps) {
    Values values;
    auto   particle = values.getParticle();
    particle.setData(exahype2::particles::ParticleVelocityAttribute, 1.0);
    for (int step = 0; step < steps; step++) {
      exahype2::particles::applyForces(particle, particle.getX(), forces, DomainOffset, DomainWidth);
      particle.setX(
        exahype2::particles::advance(particle, forces, EndTime / steps, NoInterpolation, DomainOffset, DomainWidth)
      );
    }
    return std::abs(exahype2::particles::getVelocity(particle)(0) - exactVelocity);
  };
  int order = 2;
  for (const auto scheme :
       {exahype2::particles::IntegrationScheme::Midpoint,
        exahype2::particles::IntegrationScheme::RungeKutta3,
        exahype2::particles::IntegrationScheme::RungeKutta4}) {
    forces.integrationScheme = scheme;
    const double coarseError = getVelocityError(4);
    const double fineError   = getVelocityError(8);
    validate(fineError < coarseError / (0.9 * (1 << order)));
    order++;
  }
}

void
exahype2::particles::tests::ForcesTest::testStaticIntegrationNeedsNoDataAttributes() {
  std::array<double, DIMENSIONS> values{};
  values.fill(0.5);
  exahype2::particles::Particle     particle{values.data(), 0};
  const exahype2::particles::Forces forces;
  const auto position = exahype2::particles::advance(particle, forces, 0.5, NoInterpolation, DomainOffset, DomainWidth);
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    validateEquals(position(axis), 0.5);
  }
}

void
exahype2::particles::tests::ForcesTest::testAMasslessParticleCarriesNoAcceleration() {
  Values values;
  auto   particle = values.getParticle();
  particle.setData(exahype2::particles::ParticleMassAttribute, 0.0);
  particle.setData(exahype2::particles::ParticleAccelerationAttribute, 4.0);
  exahype2::particles::Forces forces;
  forces.forceMethod = static_cast<int>(exahype2::particles::ForceType::ConstantGravity);
  exahype2::particles::applyForces(particle, particle.getX(), forces, DomainOffset, DomainWidth);
  validateNumericalEquals(exahype2::particles::getAcceleration(particle)(0), 0.0);
  validateNumericalEquals(exahype2::particles::getAcceleration(particle)(DIMENSIONS - 1), 0.0);
}

void
exahype2::particles::tests::ForcesTest::testEveryForceWritesItsAcceleration() {
  exahype2::particles::Forces forces;
  forces.gravityMagnitude       = 3.0;
  forces.dynamicViscosity       = 2.0;
  forces.gravitationalParameter = 10.0;

  Values gravityValues;
  auto   gravity     = gravityValues.getParticle();
  forces.forceMethod = static_cast<int>(exahype2::particles::ForceType::ConstantGravity);
  exahype2::particles::applyForces(gravity, gravity.getX(), forces, DomainOffset, DomainWidth);
  validateNumericalEquals(exahype2::particles::getAcceleration(gravity)(DIMENSIONS - 1), -3.0);
  validateNumericalEquals(exahype2::particles::getAcceleration(gravity)(0), 0.0);

  Values buoyancyValues;
  auto   buoyancy = buoyancyValues.getParticle();
  buoyancy.setData(exahype2::particles::FluidDensityAttribute, 2.0);
  forces.forceMethod = static_cast<int>(exahype2::particles::ForceType::Buoyancy);
  exahype2::particles::applyForces(buoyancy, buoyancy.getX(), forces, DomainOffset, DomainWidth);
  const double volume = (4.0 / 3.0) * tarch::la::PI * 0.125;
  validateNumericalEquals(exahype2::particles::getAcceleration(buoyancy)(DIMENSIONS - 1), 2.0 * volume * 3.0);

  Values dragValues;
  auto   drag = dragValues.getParticle();
  drag.setData(exahype2::particles::FluidVelocityAttribute, 1.0);
  forces.forceMethod = static_cast<int>(exahype2::particles::ForceType::StokesDrag);
  exahype2::particles::applyForces(drag, drag.getX(), forces, DomainOffset, DomainWidth);
  const double relaxationTime = 1.0 / (6.0 * tarch::la::PI * 2.0 * 0.5 + 1e-12);
  validateNumericalEquals(exahype2::particles::getAcceleration(drag)(0), 1.0 / relaxationTime);

  Values centralValues;
  auto   central = centralValues.getParticle();
  central.setX(0, 0.75);
  forces.forceMethod = static_cast<int>(exahype2::particles::ForceType::CentralGravity);
  exahype2::particles::applyForces(central, central.getX(), forces, DomainOffset, DomainWidth);
  validateNumericalEquals(exahype2::particles::getAcceleration(central)(0), -10.0 / 0.0625);
  validateNumericalEquals(exahype2::particles::getAcceleration(central)(DIMENSIONS - 1), 0.0);

  forces.forceMethod = static_cast<int>(exahype2::particles::ForceType::ConstantGravity)
    + static_cast<int>(exahype2::particles::ForceType::CentralGravity);
  validate(forces.enables(exahype2::particles::ForceType::ConstantGravity));
  validate(forces.enables(exahype2::particles::ForceType::CentralGravity));
  validate(not forces.enables(exahype2::particles::ForceType::Buoyancy));
}

/**
 * Explicit Euler moves with the old velocity, semi-implicit Euler with the new one.
 */
void
exahype2::particles::tests::ForcesTest::testTheEulerSchemesAdvanceOneStep() {
  exahype2::particles::Forces forces;
  forces.gravityMagnitude = 1.0;
  forces.forceMethod      = static_cast<int>(exahype2::particles::ForceType::ConstantGravity);

  Values explicitValues;
  auto   explicitEuler = explicitValues.getParticle();
  explicitEuler.setData(exahype2::particles::ParticleVelocityAttribute, 1.0);
  exahype2::particles::applyForces(explicitEuler, explicitEuler.getX(), forces, DomainOffset, DomainWidth);
  forces.integrationScheme = exahype2::particles::IntegrationScheme::ExplicitEuler;
  const auto explicitPosition =
    exahype2::particles::advance(explicitEuler, forces, 0.5, NoInterpolation, DomainOffset, DomainWidth);
  validateNumericalEquals(explicitPosition(0), 1.0);
  validateNumericalEquals(explicitPosition(DIMENSIONS - 1), 0.5);
  validateNumericalEquals(exahype2::particles::getVelocity(explicitEuler)(DIMENSIONS - 1), -0.5);

  Values semiImplicitValues;
  auto   semiImplicit = semiImplicitValues.getParticle();
  semiImplicit.setData(exahype2::particles::ParticleVelocityAttribute, 1.0);
  exahype2::particles::applyForces(semiImplicit, semiImplicit.getX(), forces, DomainOffset, DomainWidth);
  forces.integrationScheme = exahype2::particles::IntegrationScheme::SemiImplicitEuler;
  const auto semiPosition =
    exahype2::particles::advance(semiImplicit, forces, 0.5, NoInterpolation, DomainOffset, DomainWidth);
  validateNumericalEquals(semiPosition(0), 1.0);
  validateNumericalEquals(semiPosition(DIMENSIONS - 1), 0.25);

  Values fixedValues;
  auto   fixed             = fixedValues.getParticle();
  forces.integrationScheme = exahype2::particles::IntegrationScheme::Static;
  const auto fixedPosition =
    exahype2::particles::advance(fixed, forces, 0.5, NoInterpolation, DomainOffset, DomainWidth);
  validateNumericalEquals(fixedPosition(0), 0.5);
}

void
exahype2::particles::tests::ForcesTest::testTheHigherOrderSchemesAreExactForAConstantAcceleration() {
  exahype2::particles::Forces forces;
  forces.gravityMagnitude       = 2.0;
  forces.forceMethod            = static_cast<int>(exahype2::particles::ForceType::ConstantGravity);
  constexpr double TimeStepSize = 0.25;

  for (const auto scheme :
       {exahype2::particles::IntegrationScheme::VelocityVerlet,
        exahype2::particles::IntegrationScheme::Midpoint,
        exahype2::particles::IntegrationScheme::RungeKutta3,
        exahype2::particles::IntegrationScheme::RungeKutta4}) {
    Values values;
    auto   particle = values.getParticle();
    particle.setData(exahype2::particles::ParticleVelocityAttribute, 1.0);
    particle.setData(exahype2::particles::ParticleVelocityAttribute + DIMENSIONS - 1, 0.5);
    exahype2::particles::applyForces(particle, particle.getX(), forces, DomainOffset, DomainWidth);
    forces.integrationScheme = scheme;
    const auto position =
      exahype2::particles::advance(particle, forces, TimeStepSize, NoInterpolation, DomainOffset, DomainWidth);
    validateNumericalEqualsWithParams1(position(0), 0.5 + TimeStepSize, static_cast<int>(scheme));
    validateNumericalEqualsWithParams1(
      position(DIMENSIONS - 1),
      0.5 + 0.5 * TimeStepSize - 0.5 * 2.0 * TimeStepSize * TimeStepSize,
      static_cast<int>(scheme)
    );
    validateNumericalEqualsWithParams1(
      exahype2::particles::getVelocity(particle)(DIMENSIONS - 1),
      0.5 - 2.0 * TimeStepSize,
      static_cast<int>(scheme)
    );
  }
}

void
exahype2::particles::tests::ForcesTest::testReflectionAndWrapAtTheWalls() {
  Values values;
  auto   particle = values.getParticle();
  particle.setData(exahype2::particles::ParticleVelocityAttribute, 2.0);
  particle.setData(exahype2::particles::ParticleVelocityAttribute + DIMENSIONS - 1, -1.0);

  tarch::la::Vector<DIMENSIONS, double> position(0.5);
  position(0)                = 1.4;
  position(DIMENSIONS - 1)   = -0.25;
  const int lastAxisPeriodic = 1 << (DIMENSIONS - 1);
  exahype2::particles::reflectAtWalls(particle, position, 0.8, DomainOffset, DomainWidth, lastAxisPeriodic);
  validateNumericalEquals(position(0), 0.6);
  validateNumericalEquals(exahype2::particles::getVelocity(particle)(0), -1.6);
  validateNumericalEquals(position(DIMENSIONS - 1), -0.25);
  validateNumericalEquals(exahype2::particles::getVelocity(particle)(DIMENSIONS - 1), -1.0);

  exahype2::particles::wrapIntoDomain(position, DomainOffset, DomainWidth, lastAxisPeriodic);
  validateNumericalEquals(position(DIMENSIONS - 1), 0.75);
  validateNumericalEquals(position(0), 0.6);

  tarch::la::Vector<DIMENSIONS, double> onTheUpperWall(0.5);
  onTheUpperWall(DIMENSIONS - 1) = 1.0;
  exahype2::particles::wrapIntoDomain(onTheUpperWall, DomainOffset, DomainWidth, lastAxisPeriodic);
  validateNumericalEquals(onTheUpperWall(DIMENSIONS - 1), 0.0);

  validate(exahype2::particles::isPeriodic(lastAxisPeriodic, DIMENSIONS - 1));
  validate(not exahype2::particles::isPeriodic(lastAxisPeriodic, 0));
}
