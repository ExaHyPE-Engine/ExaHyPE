// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace applications::exahype2::ShallowWater {
  namespace coriolisForce {
    /**
     * @brief Returns the Coriolis parameter f for a given position.
     *
     * This function provides the Coriolis parameter using the f-plane
     * approximation (constant value).
     *
     * @return The Coriolis parameter f at the given location
     */
    template <auto param>
    static inline auto constant();

    /**
     * @brief Returns the Coriolis parameter f for a given position.
     *
     * This function provides the β-plane approximation (linearly varying with y).
     *
     * @param y Cartesian y-coordinate representing the north-south direction
     * @return The Coriolis parameter f at the given location
     */
    template <auto f0, auto beta>
    static inline auto betaPlane(const auto y);

    /**
     * @brief Returns the Coriolis parameter f for a given position.
     *
     * The full spherical model,
     * where the Coriolis parameter is computed as f = 2 * Ω * sin(φ).
     * (φ = geographic latitude, Ω = Earth's rotation rate)
     * ⇒ f varies with latitude.
     *
     * @param lat geographic latitude in [-pi, pi] rad
     * @return The Coriolis parameter f at the given location
     */
    template <auto omega>
    static inline auto sphere(const auto latDeg);
  } // namespace coriolisForce

  namespace bottomFriction {
    /**
     * @brief Computes the Manning bottom friction.
     *
     * This function calculates the frictional using the empirical Manning formula. The result depends on the local
     * water height and momentum components.
     *
     * The friction coefficient is defined by the Manning parameter, which may need to be
     * adjusted depending on the bathymetry, resolution, or physical characteristics of
     * the domain.
     *
     * @param h  Water height
     * @param hu x-direction
     * @param hv y-direction
     * @return Bottom friction force contribution to the momentum equations
     */
    template <auto gravity, auto param>
    static inline auto manningFriction(const auto h, const auto hu, const auto hv);
  } // namespace bottomFriction
} // namespace applications::exahype2::ShallowWater

#include "MeteotsunamiLaws.cpph"
