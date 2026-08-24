#pragma once

#include "tarch/la/Vector.h"
#include "tarch/la/VectorOperations.h"

namespace exahype2 {
  namespace fv {
    namespace particles {
      namespace utils {
        /**
         * Lightweight container for a particle's state variables.
         * Stores position, velocity, and acceleration as fixed-size vectors.
         */
        struct ParticleState {
          tarch::la::Vector<DIMENSIONS, double> position;     //!< Position vector xₙ
          tarch::la::Vector<DIMENSIONS, double> velocity;     //!< Velocity vector vₙ
          tarch::la::Vector<DIMENSIONS, double> acceleration; //!< Acceleration vector aₙ
        };

        // ======================================================================================
        // Domain Configuration Parameters
        // ======================================================================================

        extern tarch::la::Vector<DIMENSIONS, double> domainOffset;
        extern tarch::la::Vector<DIMENSIONS, double> domainSize;
        extern tarch::la::Vector<DIMENSIONS, int>    isPeriodic;

        /**
         * Set global domain information and periodicity flags.
         */
        void setGlobalParameters(
          const tarch::la::Vector<DIMENSIONS, double>& offset,
          const tarch::la::Vector<DIMENSIONS, double>& size,
          const tarch::la::Vector<DIMENSIONS, int>&    periodic
        );
        // ======================================================================================
        // Particle Mass Accessors
        // ======================================================================================
        /**
         * Get the mass of a particle.
         */
        template <typename Particle>
        double getMass(const Particle& p);
        /**
         * Set the mass of a particle.
         */
        template <typename Particle>
        void setMass(Particle& p, double mass);
        // ======================================================================================
        // Particle Interpolated rho Accessor
        // ======================================================================================
        /**
         * Get the interpolated density (rho) on a particle.
         */
        template <typename Particle>
        double getInterpolatedRho(const Particle& p);

        // ======================================================================================
        // Velocity Accessors
        // ======================================================================================

        /**
         * Get the velocity vector of a particle.
         */
        template <typename Particle>
        tarch::la::Vector<DIMENSIONS, double> getVelocity(const Particle& p);

        /**
         * Set the velocity vector of a particle.
         */
        template <typename Particle>
        void setVelocity(Particle& p, const tarch::la::Vector<DIMENSIONS, double>& v);

        // ======================================================================================
        // Acceleration Accessors
        // ======================================================================================

        /**
         * Get the acceleration vector of a particle.
         */
        template <typename Particle>
        tarch::la::Vector<DIMENSIONS, double> getAcceleration(const Particle& p);

        /**
         * Set the acceleration vector of a particle.
         */
        template <typename Particle>
        void setAcceleration(Particle& p, const tarch::la::Vector<DIMENSIONS, double>& a);

        /**
         * Reset acceleration vector to zero.
         */
        template <typename Particle>
        void resetAcceleration(Particle& particle);

        // ======================================================================================
        // Physics and State Accessors
        // ======================================================================================
        // Radius accessors
        template <typename Particle>
        double getRadius(const Particle& p);
        template <typename Particle>
        void setRadius(Particle& p, double radius);
        /**
         * Compute kinetic energy: (1/2) · m · v²
         */
        template <typename Particle>
        double computeKineticEnergy(const Particle& p);

        /**
         * Compute momentum: m · v
         */
        template <typename Particle>
        tarch::la::Vector<DIMENSIONS, double> computeMomentum(const Particle& p);

        /**
         * Extract the current particle state as a ParticleState struct.
         */
        template <typename Particle>
        ParticleState getParticleState(const Particle& particle);
      } // namespace utils
    } // namespace particles
  } // namespace fv
} // namespace exahype2

#include "ParticleHelper.cpph"
