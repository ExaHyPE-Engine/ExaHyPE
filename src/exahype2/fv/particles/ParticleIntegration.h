// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#pragma once

#include "ParticleForces.h"
#include "ParticleHelper.h"
#include "ParticleIntegrationHelper.h"
#include "ParticleInterpolation.h"

#include "tarch/la/Vector.h"
#include "toolbox/particles/potentials/Springs.h"

using namespace exahype2::fv::particles::utils;

namespace exahype2 {
  namespace fv {
    namespace particles {
      namespace integration {
        //=============================================================================
        // Global Configuration
        //=============================================================================

        /**
         * Global time step size used by all integrators.
         */
        extern double _timeStepSize;

        /**
         * Integration method selector.
         * 1 = Euler, 2 = Semi-implicit Euler, 3 = Velocity Verlet, 4 = Midpoint, 5 = RK4
         */
        extern int _integrationMethod;

        /**
         * Set global integration parameters.
         *
         * @param timeStepSize     Time step for integration
         * @param _integrationMethod  Integer key for the integration scheme
         */
        void setGlobalIntegrationParameters(double timeStepSize, int _integrationMethod);

        //=============================================================================
        // Integration Methods
        //=============================================================================

        /**
         * Explicit Euler integration.
         *
         * Formula:
         *   x(t+dt) = x(t) + v(t) * dt
         *
         * @param particle Particle to move
         * @param timeStepSize Time step size
         * @return Updated position
         */
        template <typename Particle>
        tarch::la::Vector<DIMENSIONS, double> explicitEulerIntegration(Particle& particle);

        /**
         * Semi-implicit Euler (Symplectic Euler).
         *
         * Formula:
         *   v(t+dt) = v(t) + a(t) * dt
         *   x(t+dt) = x(t) + v(t+dt) * dt
         *
         * @param particle Particle to move
         * @param timeStepSize Time step size
         * @return Updated position
         */
        template <typename Particle>
        tarch::la::Vector<DIMENSIONS, double> semiImplicitEulerIntegration(Particle& particle);

        /**
         * Midpoint (Second-order Runge-Kutta) integration.
         *
         * Formula:
         *   x_half = x(t) + 0.5 * v(t) * dt
         *   v_half = v(t) + 0.5 * a(t) * dt
         *   x(t+dt) = x(t) + v_half * dt
         *
         * Requires evaluating force/interpolation at midpoint.
         *
         * @tparam Indices Particle field interpolation indices
         * @param particle Particle to move
         * @param marker Cell context
         * @param Q Cell-based field data
         * @return Updated position
         */
        template <int... Indices, typename Particle>
        tarch::la::Vector<DIMENSIONS, double> midpointIntegration(
          Particle&                                 particle,
          const peano4::datamanagement::CellMarker& marker,
          const double* __restrict__ Q
        );
        /**
         * Classical third-order Runge-Kutta (RK3) integration.
         *
         * Formula:
         *   k1 = f(t, x)
         *   k2 = f(t + dt/2, x + dt/2 * k1)
         *   k3 = f(t + dt,   x - dt * k1 + 2*dt * k2)
         *   x(t+dt) = x + dt/6 * (k1 + 4*k2 + k3)
         *
         * @tparam Indices Particle field interpolation indices
         * @param particle Particle to move
         * @param marker Cell context
         * @param Q Cell-based field data
         * @return Updated position
         */
        template <int... Indices, typename Particle>
        tarch::la::Vector<DIMENSIONS, double> RK3Integration(
          Particle&                                 particle,
          const peano4::datamanagement::CellMarker& marker,
          const double* __restrict__ Q
        );
        /**
         * Classical fourth-order Runge-Kutta (RK4) integration.
         *
         * Formula:
         *   k1 = f(t, x)
         *   k2 = f(t + dt/2, x + dt/2 * k1)
         *   k3 = f(t + dt/2, x + dt/2 * k2)
         *   k4 = f(t + dt,   x + dt * k3)
         *   x(t+dt) = x + dt/6 * (k1 + 2*k2 + 2*k3 + k4)
         *
         * @tparam Indices Particle field interpolation indices
         * @param particle Particle to move
         * @param marker Cell context
         * @param Q Cell-based field data
         * @return Updated position
         */
        template <int... Indices, typename Particle>
        tarch::la::Vector<DIMENSIONS, double> RK4Integration(
          Particle&                                 particle,
          const peano4::datamanagement::CellMarker& marker,
          const double* __restrict__ Q
        );

        /**
         * Velocity Verlet integration.
         *
         * Formula:
         *   x(t+dt) = x(t) + v(t) * dt + 0.5 * a(t) * dt²
         *   v(t+dt) = v(t) + 0.5 * (a(t) + a(t+dt)) * dt
         *
         * @tparam Indices Particle field interpolation indices
         * @param particle Particle to move
         * @param marker Cell context
         * @param Q Cell-based field data
         * @return Updated position
         */
        template <int... Indices, typename Particle>
        tarch::la::Vector<DIMENSIONS, double> velocityVerletIntegration(
          Particle&                                 particle,
          const peano4::datamanagement::CellMarker& marker,
          const double* __restrict__ Q
        );

        /**
         * Apply the globally selected integration method.
         *
         * Dispatches to the appropriate integration method based on `_integrationMethod`.
         *
         * @tparam Indices Particle field interpolation indices
         * @param particle Particle to move
         * @param marker Cell context
         * @param Q Cell-based field data
         * @return Updated position
         */
        template <int... Indices, typename Particle>
        tarch::la::Vector<DIMENSIONS, double> applyIntegration(
          Particle&                                 particle,
          const peano4::datamanagement::CellMarker& marker,
          const double* __restrict__ Q
        );
      } // namespace integration
    } // namespace particles
  } // namespace fv
} // namespace exahype2

#include "ParticleIntegration.cpph"
