// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <cstddef>

// TODO: Using std=c++23 constexpr would be possible

namespace projection::AEA {
  namespace ellipsoid {
    /*
     * @brief
     * Projects geographic coordinates (longitude, latitude) onto a planar surface
     * using the Albers Equal-Area Conic projection for the WGS_1984 ellipsoid.
     *
     * This implementation is based on the equations provided in USGS Professional Paper 1395.
     * It supports standard parallels and is suitable for regional-scale mapping where
     * area preservation is important.
     *
     * @see https://doi.org/10.3133/pp1395
     */
    template <auto EllipsoidA, auto EllipsoidE, auto StdParallel1Deg, auto StdParallel2Deg, auto Lon0Deg, auto Lat0Deg>
    static inline auto forward(const auto lonDeg, const auto latDeg);

    /**
     * @brief
     * Inverts the Albers Equal-Area Conic projection for WGS_1984 and returns the geographic coordinates
     * (longitude, latitude in degrees) corresponding to the given planar coordinates (x, y in meters).
     *
     * The function uses an iterative Newton-like method to recover the latitude from the projected y-coordinate,
     * based on the equations in USGS Professional Paper 1395.
     *
     * @see https://doi.org/10.3133/pp1395
     */
    template <
      auto EllipsoidA,
      auto EllipsoidE,
      auto StdParallel1Deg,
      auto StdParallel2Deg,
      auto Lon0Deg,
      auto Lat0Deg,
      auto MaxIter   = 10,
      auto Tolerance = 1e-12>
    static inline auto inverse(const auto x, const auto y);
  } // namespace ellipsoid

  namespace sphere {
    /**
     * @brief
     * Projects geographic coordinates (longitude, latitude) onto a planar surface using
     * the Albers Equal-Area Conic projection for a spherical Earth model.
     *
     * This simplified version assumes a spherical Earth with radius R and is suitable
     * for regional mapping where area preservation is required but ellipsoidal precision is not necessary.
     *
     * @see https://doi.org/10.3133/pp1395
     */
    template <auto Lon0Deg, auto Lat0Deg, auto StdParallel1Deg, auto StdParallel2Deg, auto EarthRadius>
    static inline auto forward(const auto lon, const auto lat);

    /**
     * @brief
     * Inverts the spherical Albers Equal-Area Conic projection and returns the geographic coordinates
     * (longitude, latitude in degrees) corresponding to the given planar coordinates (x, y in meters).
     *
     * This implementation assumes a spherical Earth model and uses the analytical inverse
     * of the projection equations without iteration.
     *
     * @see https://doi.org/10.3133/pp1395
     */
    template <auto Lon0Deg, auto Lat0Deg, auto StdParallel1Deg, auto StdParallel2Deg, auto EarthRadius>
    static inline auto inverse(const auto x, const auto y);
  } // namespace sphere
} // namespace projection::AEA

#include "ProjectionAEA.cpph"
