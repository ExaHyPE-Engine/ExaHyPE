// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/particles/Forces.h"

#include "exahype2/particles/Integration.h"
#include "exahype2/particles/ParticleSet.h"
#include "tarch/la/Scalar.h"
#include "tarch/la/Vector.h"
#include "tarch/testing/UnitTest.h"

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

UNIT_TEST("The forces write a zero acceleration into a particle without mass") {
  GIVEN("A particle without mass with the acceleration four along the first axis and the constant gravity") {
    Values values;
    auto   particle = values.getParticle();
    particle.setData(exahype2::particles::ParticleMassAttribute, 0.0);
    particle.setData(exahype2::particles::ParticleAccelerationAttribute, 4.0);
    exahype2::particles::Forces forces;
    forces.forceMethod = static_cast<int>(exahype2::particles::ForceType::ConstantGravity);

    WHEN("The forces act on the particle") {
      exahype2::particles::applyForces(particle, particle.getX(), forces, DomainOffset, DomainWidth);
      const auto acceleration = exahype2::particles::getAcceleration(particle);

      THEN("The acceleration is zero along the first and the last axis") {
        VALIDATE_NUMERICAL_EQUALS(acceleration(0), 0.0);
        VALIDATE_NUMERICAL_EQUALS(acceleration(DIMENSIONS - 1), 0.0);
      }
    }
  }
}

UNIT_TEST("Every force writes its acceleration into a particle and a sum of forces enables its members only") {
  GIVEN("Forces with the gravity magnitude three, the viscosity two and the gravitational parameter ten") {
    exahype2::particles::Forces forces;
    forces.gravityMagnitude       = 3.0;
    forces.dynamicViscosity       = 2.0;
    forces.gravitationalParameter = 10.0;

    WHEN("The constant gravity acts on a particle at the domain centre") {
      Values values;
      auto   particle    = values.getParticle();
      forces.forceMethod = static_cast<int>(exahype2::particles::ForceType::ConstantGravity);
      exahype2::particles::applyForces(particle, particle.getX(), forces, DomainOffset, DomainWidth);
      const auto acceleration = exahype2::particles::getAcceleration(particle);

      THEN("The acceleration is minus the gravity magnitude along the last axis and zero along the first axis") {
        VALIDATE_NUMERICAL_EQUALS(acceleration(DIMENSIONS - 1), -3.0);
        VALIDATE_NUMERICAL_EQUALS(acceleration(0), 0.0);
      }
    }

    WHEN("The buoyancy acts on a particle in a fluid of density two") {
      Values values;
      auto   particle = values.getParticle();
      particle.setData(exahype2::particles::FluidDensityAttribute, 2.0);
      forces.forceMethod = static_cast<int>(exahype2::particles::ForceType::Buoyancy);
      exahype2::particles::applyForces(particle, particle.getX(), forces, DomainOffset, DomainWidth);
      const auto acceleration = exahype2::particles::getAcceleration(particle);

      THEN("The buoyancy pushes the particle along the last axis with the density times its volume times the gravity") {
        constexpr double Volume = (4.0 / 3.0) * tarch::la::PI * 0.125;
        VALIDATE_NUMERICAL_EQUALS(acceleration(DIMENSIONS - 1), 2.0 * Volume * 3.0);
      }
    }

    WHEN("The Stokes drag acts on a particle at rest in a fluid of velocity one along the first axis") {
      Values values;
      auto   particle = values.getParticle();
      particle.setData(exahype2::particles::FluidVelocityAttribute, 1.0);
      forces.forceMethod = static_cast<int>(exahype2::particles::ForceType::StokesDrag);
      exahype2::particles::applyForces(particle, particle.getX(), forces, DomainOffset, DomainWidth);
      const auto acceleration = exahype2::particles::getAcceleration(particle);

      THEN("The drag accelerates the particle towards the fluid velocity over the relaxation time") {
        constexpr double RelaxationTime = 1.0 / (6.0 * tarch::la::PI * 2.0 * 0.5 + 1e-12);
        VALIDATE_NUMERICAL_EQUALS(acceleration(0), 1.0 / RelaxationTime);
      }
    }

    WHEN("The central gravity acts on a particle a quarter off the centre along the first axis") {
      Values values;
      auto   particle = values.getParticle();
      particle.setX(0, 0.75);
      forces.forceMethod = static_cast<int>(exahype2::particles::ForceType::CentralGravity);
      exahype2::particles::applyForces(particle, particle.getX(), forces, DomainOffset, DomainWidth);
      const auto acceleration = exahype2::particles::getAcceleration(particle);

      THEN("The gravity pulls the particle towards the centre with the parameter over the squared distance") {
        VALIDATE_NUMERICAL_EQUALS(acceleration(0), -10.0 / 0.0625);
        VALIDATE_NUMERICAL_EQUALS(acceleration(DIMENSIONS - 1), 0.0);
      }
    }

    WHEN("The force method sums the constant and the central gravity") {
      forces.forceMethod = static_cast<int>(exahype2::particles::ForceType::ConstantGravity)
        + static_cast<int>(exahype2::particles::ForceType::CentralGravity);

      THEN("The forces enable both gravities only") {
        VALIDATE(forces.enables(exahype2::particles::ForceType::ConstantGravity));
        VALIDATE(forces.enables(exahype2::particles::ForceType::CentralGravity));
        VALIDATE(not forces.enables(exahype2::particles::ForceType::Buoyancy));
      }
    }
  }
}

UNIT_TEST("The explicit Euler scheme moves with the old velocity, the semi-implicit one with the new velocity") {
  GIVEN("The constant gravity one and a particle at the domain centre with the velocity one along the first axis") {
    exahype2::particles::Forces forces;
    forces.gravityMagnitude = 1.0;
    forces.forceMethod      = static_cast<int>(exahype2::particles::ForceType::ConstantGravity);
    Values values;
    auto   particle = values.getParticle();
    particle.setData(exahype2::particles::ParticleVelocityAttribute, 1.0);
    exahype2::particles::applyForces(particle, particle.getX(), forces, DomainOffset, DomainWidth);

    WHEN("The explicit Euler scheme advances the particle by the time step one half") {
      forces.integrationScheme = exahype2::particles::IntegrationScheme::ExplicitEuler;
      const auto position =
        exahype2::particles::advance(particle, forces, 0.5, NoInterpolation, DomainOffset, DomainWidth);
      const auto velocity = exahype2::particles::getVelocity(particle);

      THEN("The particle moves with its old velocity and the gravity reduces its velocity along the last axis") {
        VALIDATE_NUMERICAL_EQUALS(position(0), 1.0);
        VALIDATE_NUMERICAL_EQUALS(position(DIMENSIONS - 1), 0.5);
        VALIDATE_NUMERICAL_EQUALS(velocity(DIMENSIONS - 1), -0.5);
      }
    }

    WHEN("The semi-implicit Euler scheme advances the particle by the time step one half") {
      forces.integrationScheme = exahype2::particles::IntegrationScheme::SemiImplicitEuler;
      const auto position =
        exahype2::particles::advance(particle, forces, 0.5, NoInterpolation, DomainOffset, DomainWidth);

      THEN("The particle moves with its new velocity") {
        VALIDATE_NUMERICAL_EQUALS(position(0), 1.0);
        VALIDATE_NUMERICAL_EQUALS(position(DIMENSIONS - 1), 0.25);
      }
    }
  }

  GIVEN("The constant gravity one with the static scheme and a particle at rest at the domain centre") {
    exahype2::particles::Forces forces;
    forces.gravityMagnitude  = 1.0;
    forces.forceMethod       = static_cast<int>(exahype2::particles::ForceType::ConstantGravity);
    forces.integrationScheme = exahype2::particles::IntegrationScheme::Static;
    Values values;
    auto   particle = values.getParticle();

    WHEN("The static scheme advances the particle by the time step one half") {
      const auto position =
        exahype2::particles::advance(particle, forces, 0.5, NoInterpolation, DomainOffset, DomainWidth);

      THEN("The particle stays at the domain centre") { VALIDATE_NUMERICAL_EQUALS(position(0), 0.5); }
    }
  }
}

UNIT_TEST("Every scheme of second or higher order advances exactly under a constant acceleration") {
  GIVEN("The constant gravity two and, for every scheme of second or higher order, a particle that leaves the centre") {
    static constexpr std::array<exahype2::particles::IntegrationScheme, 4> Schemes{
      exahype2::particles::IntegrationScheme::VelocityVerlet,
      exahype2::particles::IntegrationScheme::Midpoint,
      exahype2::particles::IntegrationScheme::RungeKutta3,
      exahype2::particles::IntegrationScheme::RungeKutta4
    };
    exahype2::particles::Forces forces;
    forces.gravityMagnitude = 2.0;
    forces.forceMethod      = static_cast<int>(exahype2::particles::ForceType::ConstantGravity);
    std::array<Values, Schemes.size()> valuesOfScheme;
    for (auto& values : valuesOfScheme) {
      auto particle = values.getParticle();
      particle.setData(exahype2::particles::ParticleVelocityAttribute, 1.0);
      particle.setData(exahype2::particles::ParticleVelocityAttribute + DIMENSIONS - 1, 0.5);
      exahype2::particles::applyForces(particle, particle.getX(), forces, DomainOffset, DomainWidth);
    }

    WHEN("Every scheme advances its particle by the time step one quarter") {
      constexpr double                                   TimeStepSize = 0.25;
      std::vector<tarch::la::Vector<DIMENSIONS, double>> positionOfScheme;
      std::vector<double>                                lastVelocityOfScheme;
      for (std::size_t scheme = 0; scheme < Schemes.size(); scheme++) {
        auto particle            = valuesOfScheme[scheme].getParticle();
        forces.integrationScheme = Schemes[scheme];
        positionOfScheme.push_back(
          exahype2::particles::advance(particle, forces, TimeStepSize, NoInterpolation, DomainOffset, DomainWidth)
        );
        lastVelocityOfScheme.push_back(exahype2::particles::getVelocity(particle)(DIMENSIONS - 1));
      }

      THEN("Every scheme reaches the position x + v dt + a dt^2 / 2 and the velocity v + a dt") {
        for (std::size_t scheme = 0; scheme < Schemes.size(); scheme++) {
          VALIDATE_NUMERICAL_EQUALS(positionOfScheme[scheme](0), 0.5 + TimeStepSize, static_cast<int>(Schemes[scheme]));
          VALIDATE_NUMERICAL_EQUALS(
            positionOfScheme[scheme](DIMENSIONS - 1),
            0.5 + 0.5 * TimeStepSize - 0.5 * 2.0 * TimeStepSize * TimeStepSize,
            static_cast<int>(Schemes[scheme])
          );
          VALIDATE_NUMERICAL_EQUALS(
            lastVelocityOfScheme[scheme],
            0.5 - 2.0 * TimeStepSize,
            static_cast<int>(Schemes[scheme])
          );
        }
      }
    }
  }
}

UNIT_TEST("The velocity of the midpoint and the Runge-Kutta schemes converges with their order under drag") {
  GIVEN("The Stokes drag, the end time one half, the exact velocity of the exponential decay and the scheme orders") {
    exahype2::particles::Forces forces;
    forces.forceMethod             = static_cast<int>(exahype2::particles::ForceType::StokesDrag);
    forces.dynamicViscosity        = 1.0 / (3.0 * tarch::la::PI);
    constexpr double EndTime       = 0.5;
    const double     decayRate     = 3.0 * tarch::la::PI * forces.dynamicViscosity + 1e-12;
    const double     exactVelocity = std::exp(-decayRate * EndTime);

    static constexpr std::array<exahype2::particles::IntegrationScheme, 3> Schemes{
      exahype2::particles::IntegrationScheme::Midpoint,
      exahype2::particles::IntegrationScheme::RungeKutta3,
      exahype2::particles::IntegrationScheme::RungeKutta4
    };
    static constexpr std::array<int, 3> Orders{2, 3, 4};

    WHEN("Every scheme integrates a particle of velocity one up to the end time in four and in eight steps") {
      const auto getVelocityError = [&](const int steps) {
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
      std::vector<double> coarseErrorOfScheme;
      std::vector<double> fineErrorOfScheme;
      for (const auto scheme : Schemes) {
        forces.integrationScheme = scheme;
        coarseErrorOfScheme.push_back(getVelocityError(4));
        fineErrorOfScheme.push_back(getVelocityError(8));
      }

      THEN("Eight steps divide the error of four steps by more than 0.9 times two to the power of the order") {
        for (std::size_t scheme = 0; scheme < Schemes.size(); scheme++) {
          VALIDATE(fineErrorOfScheme[scheme] < coarseErrorOfScheme[scheme] / (0.9 * (1 << Orders[scheme])));
        }
      }
    }
  }
}

UNIT_TEST("The static scheme keeps the position of a particle without reading its data") {
  GIVEN("A particle at the domain centre without data entries and the static scheme") {
    std::array<double, DIMENSIONS> values{};
    values.fill(0.5);
    exahype2::particles::Particle     particle{values.data(), 0};
    const exahype2::particles::Forces forces;

    WHEN("The static scheme advances the particle by the time step one half") {
      const auto position =
        exahype2::particles::advance(particle, forces, 0.5, NoInterpolation, DomainOffset, DomainWidth);

      THEN("The particle stays at the domain centre") {
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          VALIDATE_EQUALS(position(axis), 0.5);
        }
      }
    }
  }
}

UNIT_TEST("A particle reflects at the walls of an axis that is not periodic and wraps around along a periodic axis") {
  GIVEN("A moving particle past the upper wall of the first axis and below the lower wall of the periodic last axis") {
    constexpr int LastAxisPeriodic = 1 << (DIMENSIONS - 1);
    Values        values;
    auto          particle = values.getParticle();
    particle.setData(exahype2::particles::ParticleVelocityAttribute, 2.0);
    particle.setData(exahype2::particles::ParticleVelocityAttribute + DIMENSIONS - 1, -1.0);
    tarch::la::Vector<DIMENSIONS, double> position(0.5);
    position(0)              = 1.4;
    position(DIMENSIONS - 1) = -0.25;

    WHEN("The particle reflects at the walls with the damping factor 0.8") {
      exahype2::particles::reflectAtWalls(particle, position, 0.8, DomainOffset, DomainWidth, LastAxisPeriodic);
      const auto velocity = exahype2::particles::getVelocity(particle);

      THEN("Along the first axis the position mirrors and the damped velocity reverses and the last axis keeps both") {
        VALIDATE_NUMERICAL_EQUALS(position(0), 0.6);
        VALIDATE_NUMERICAL_EQUALS(velocity(0), -1.6);
        VALIDATE_NUMERICAL_EQUALS(position(DIMENSIONS - 1), -0.25);
        VALIDATE_NUMERICAL_EQUALS(velocity(DIMENSIONS - 1), -1.0);
      }

      AND_WHEN("The position wraps into the domain") {
        exahype2::particles::wrapIntoDomain(position, DomainOffset, DomainWidth, LastAxisPeriodic);

        THEN("The position moves by one domain width along the periodic last axis and stays along the first axis") {
          VALIDATE_NUMERICAL_EQUALS(position(DIMENSIONS - 1), 0.75);
          VALIDATE_NUMERICAL_EQUALS(position(0), 0.6);
        }
      }
    }
  }

  GIVEN("A position on the upper wall of the periodic last axis") {
    constexpr int                         LastAxisPeriodic = 1 << (DIMENSIONS - 1);
    tarch::la::Vector<DIMENSIONS, double> onTheUpperWall(0.5);
    onTheUpperWall(DIMENSIONS - 1) = 1.0;

    WHEN("The position wraps into the domain") {
      exahype2::particles::wrapIntoDomain(onTheUpperWall, DomainOffset, DomainWidth, LastAxisPeriodic);

      THEN("The position moves to the lower wall") { VALIDATE_NUMERICAL_EQUALS(onTheUpperWall(DIMENSIONS - 1), 0.0); }
    }
  }

  GIVEN("The periodic axes of a domain that is periodic along the last axis only") {
    constexpr int LastAxisPeriodic = 1 << (DIMENSIONS - 1);

    THEN("The last axis is periodic and the first axis is not") {
      VALIDATE(exahype2::particles::isPeriodic(LastAxisPeriodic, DIMENSIONS - 1));
      VALIDATE(not exahype2::particles::isPeriodic(LastAxisPeriodic, 0));
    }
  }
}
