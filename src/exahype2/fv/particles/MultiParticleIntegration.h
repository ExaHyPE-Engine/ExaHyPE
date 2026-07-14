#pragma once

#include "ParticleHelper.h"
#include "ParticleForces.h"
#include "ParticleIntegration.h"
#include "ParticleIntegrationHelper.h"
#include "ParticleInterpolation.h"
#include "tarch/la/Vector.h"
#include "toolbox/particles/potentials/Springs.h"

namespace exahype2 {
  namespace fv {
    namespace particles {
      namespace ppintegration {
        //=============================================================================
        // Global Constants
        //=============================================================================

        // Time step size for integration
        inline double _timeStepSize = exahype2::fv::particles::integration::_timeStepSize;

        // Universal gravitational constant (default 0.1)
        inline double _G = 0.1;

        // Damping factor for boundary collisions (1.0 = perfectly elastic)
        inline double _boundaryDamping = 1.0;
        inline double _springDistance  = 0.1;
        // Domain properties
        inline tarch::la::Vector<DIMENSIONS, double> _domainOffset(0.0);
        inline tarch::la::Vector<DIMENSIONS, double> _domainSize(1.0);
        inline tarch::la::Vector<DIMENSIONS, double> _periodicBC(0.0);
        [[maybe_unused]] inline static int           _ppInteractionMethod = 0; // default = no interaction
        // PP Collision parameters

        //=============================================================================
        // Particle State Definitions & Helper Functions
        //=============================================================================

        /**
         * @brief Computes pairwise acceleration between two particles
         */
        template <typename Particle>
        void centralGravityInteraction(
          Particle&                                    p1,
          const tarch::la::Vector<DIMENSIONS, double>& pos1,
          Particle&                                    p2,
          const tarch::la::Vector<DIMENSIONS, double>& pos2
        );

        /**
         * @brief Zeros out acceleration data in all particles
         */
        template <typename Particle>
        void clearAccelerations(const std::vector<Particle*>& particles);

        template <typename Particle>
        void applyIntegration(std::vector<Particle*>& particles);

        //=============================================================================
        // Integration Schemes
        //=============================================================================

        /**
         * @brief First-order explicit Euler integration for a single particle
         */
        template <typename Particle>
        tarch::la::Vector<DIMENSIONS, double> explicitEulerIntegration(
          std::vector<Particle*>& particles,
          double                  timeStepSize
        );

        /**
         * @brief First-order semi-implicit Euler integration for a single particle
         */
        template <typename Particle>
        tarch::la::Vector<DIMENSIONS, double> semiImplicitEulerIntegration(
          std::vector<Particle*>& particles,
          double                  timeStepSize
        );

        /**
         * @brief Second-order Velocity Verlet integration for particle systems
         */
        template <typename Particle>
        void velocityVerletIntegration(std::vector<Particle*>& particles);

        /**
         * @brief Second-order Midpoint integration for particle systems
         */
        template <typename Particle>
        void midpointIntegration(std::vector<Particle*>& particles);

        /**
         * @brief Third-order Kutta's method integration for particle systems
         */
        template <typename Particle>
        void RK3Integration(std::vector<Particle*>& particles);

        /**
         * @brief Fourth-order Runge-Kutta integration for particle systems
         */
        template <typename Particle>
        void RK4Integration(std::vector<Particle*>& particles);
        template <typename Particle>
        void applyPPInteraction(
          Particle&                                    p1,
          const tarch::la::Vector<DIMENSIONS, double>& pos1,
          Particle&                                    p2,
          const tarch::la::Vector<DIMENSIONS, double>& pos2
        );
        template <typename Particle>
        void centralGravityInteraction(
          Particle&                                    p1,
          const tarch::la::Vector<DIMENSIONS, double>& pos1,
          Particle&                                    p2,
          const tarch::la::Vector<DIMENSIONS, double>& pos2
        );
      } // namespace ppintegration
    } // namespace particles
  } // namespace fv
} // namespace exahype2

#include "MultiParticleIntegration.cpph"
