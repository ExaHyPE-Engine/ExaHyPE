// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace applications::exahype2::ShallowWater::GulfOfMexico {
  /**
   * @brief Computes the moving atmospheric pressure disturbance for the Gulf of Mexico test case.
   *
   * The pressure field is modelled as an asymmetric Gaussian pulse that travels along
   * a great-circle–like path between two geographic points (A,B), defined in lon/lat.
   * The function takes Cartesian model coordinates (x,y) in the Albers Equal-Area projection,
   * maps them back to lon/lat, aligns the (A,B) path with a local x-axis, and evaluates
   * the pressure profile in this rotated coordinate system.
   *
   * @tparam pointLonA Longitude of path start point A [deg].
   * @tparam pointLatA Latitude of path start point A [deg].
   * @tparam pointLonB Longitude of path end point B [deg].
   * @tparam pointLatB Latitude of path end point B [deg].
   *
   * @param x  Cartesian x-coordinate in projected space [m].
   * @param y  Cartesian y-coordinate in projected space [m].
   * @param t  Physical time [s].
   * @param projectionFunctionForward  Callable that maps (lon,lat) → (x,y) in the chosen projection.
   * @param projectionFunctionInverse  Callable that maps (x,y) → (lon,lat) in the chosen projection.
   *
   * @return Atmospheric pressure perturbation at (x,y,t) [Pa].
   */
  template <auto pointLonA, auto pointLatA, auto pointLonB, auto pointLatB>
  static inline auto atmosphericPressure(
    const auto x,
    const auto y,
    const auto t,
    auto       projectionFunctionForward,
    auto       projectionFunctionInverse
  );
} // namespace applications::exahype2::ShallowWater::GulfOfMexico

#include "AtmosphericPressure.cpph"
