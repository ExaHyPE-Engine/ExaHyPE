// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#pragma once

#include "ParticleHelper.h"

#include <tarch/la/Vector.h>

namespace exahype2 {
  namespace fv {
    namespace particles {
      namespace forces {
        /**
         * Force types as enum flags - can be combined using bitwise operations.
         * Using enum class for type safety and clarity.
         */
        enum class ForceType : int {
          None            = 0,
          CentralGravity  = 1 << 0, // 1
          Buoyancy        = 1 << 1, // 2
          Drag            = 1 << 2, // 4
          ConstantGravity = 1 << 3, // 8
                                    // Add more forces here as needed:
                                    // ElectroMagnetic = 1 << 4,  // 16
                                    // SpringForce     = 1 << 5,  // 32
                                    // CustomForce1    = 1 << 6,  // 64
                                    // CustomForce2    = 1 << 7,  // 128
        };

        // Operator overloading to allow combining force flags
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

        /**
         * Active force configuration (stored as int for compatibility).
         */
        extern int _forceMethod;

        /**
         * Gravitational constant (G*M), used in central gravity model.
         */
        extern double _mu;

        /**
         * Constant gravity magnitude (e.g., 9.81 m/s^2).
         */
        extern double _gravityMagnitude;

        /**
         * Dynamic viscosity of the fluid.
         */
        extern double _dynamicViscosity;

        /**
         * Default fluid density (can be overridden by local values).
         */
        extern double _fluidDensity;

        /**
         * Set force method using ForceType enum.
         */
        void setForceMethod(ForceType forces);

        /**
         * Set force method using integer (for backward compatibility).
         */
        void setForceMethod(int forceMethod);

        /**
         * Enable a specific force.
         */
        void enableForce(ForceType force);

        /**
         * Disable a specific force.
         */
        void disableForce(ForceType force);

        /**
         * Check if a force is enabled.
         */
        bool isForceEnabled(ForceType force);
        // Internal helper for templates
        inline bool isForceEnabledInternal(ForceType force) {
          return (_forceMethod & static_cast<int>(force)) == static_cast<int>(force);
        }
        /**
         * Disable all forces.
         */
        void disableAllForces();

        /**
         * Set gravitational parameter for central force.
         */
        void setGravitationalParameter(double mu);

        /**
         * Set constant gravity magnitude.
         */
        void setGravityMagnitude(double g);

        /**
         * Set dynamic viscosity.
         */
        void setDynamicViscosity(double viscosity);

        /**
         * Set default fluid density.
         */
        void setDefaultFluidDensity(double density);

        /**
         * Set all force parameters at once.
         */
        void setAllParameters(int forceMethod, double mu, double g, double viscosity, double density);
        /**
         * Set force method and gravitational parameter.
         *
         * @param forceMethod  Bitmask selecting active forces
         * @param mu          Gravitational parameter (G*M)
         */

        /**
         * Set constant gravity magnitude.
         *
         * @param g Gravity magnitude (m/s^2)
         */
        void setGravityMagnitude(double g);

        /**
         * Set dynamic viscosity.
         *
         * @param viscosity Dynamic viscosity of fluid
         */
        void setDynamicViscosity(double viscosity);

        /**
         * Set default fluid density.
         *
         * @param density Fluid density (kg/m^3)
         */
        void setDefaultFluidDensity(double density);

        /**
         * Set all force parameters at once.
         *
         * @param forceMethod Bitmask selecting active forces
         * @param mu         Gravitational parameter (G*M)
         * @param g          Gravity magnitude
         * @param viscosity  Dynamic viscosity
         * @param density    Fluid density
         */
        void setAllParameters(int forceMethod, double mu, double g, double viscosity, double density);

        /**
         * Apply external forces to a particle based on its current position.
         * This sets the particle's acceleration field accordingly.
         *
         * @param particle Particle to which forces should be applied
         * @param pos      Current position of the particle
         */
        template <typename Particle>
        void applyForces(Particle& particle, tarch::la::Vector<DIMENSIONS, double> pos);

        /**
         * Compute central gravitational force acting on a particle at a given position.
         *
         * @param particlePosition Current position of the particle
         * @return                 Force vector due to central gravity
         */
        template <typename Particle>
        tarch::la::Vector<DIMENSIONS, double> computeCentralGravitationalForce(
          const tarch::la::Vector<DIMENSIONS, double>& particlePosition,
          const Particle&                              particle
        );

        /**
         * Compute Stokes drag force.
         *
         * @param particleVelocity Velocity of the particle
         * @param fluidVelocity    Velocity of the fluid
         * @param particle         Particle object (for mass and radius)
         * @return                 Force vector due to drag
         */
        template <typename Particle>
        tarch::la::Vector<DIMENSIONS, double> computeStokesDragForce(const Particle& particle);

        /**
         * Compute buoyancy force on a particle.
         *
         * @param particle     Particle object
         * @param fluidDensity Local fluid density (use -1 for default)
         * @return             Force vector due to buoyancy
         */
        template <typename Particle>
        tarch::la::Vector<DIMENSIONS, double> computeBuoyancyForce(const Particle& particle);

        /**
         * Compute constant gravity force.
         *
         * @param particle Particle object
         * @return         Force vector due to gravity
         */
        template <typename Particle>
        tarch::la::Vector<DIMENSIONS, double> computeGravityForce(const Particle& particle);
      } // namespace forces
    } // namespace particles
  } // namespace fv
} // namespace exahype2

#include "ParticleForces.cpph"
