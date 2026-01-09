#pragma once

#include "../Constants.h"

#include <cmath>
#include <utility>

namespace projection {
  namespace albers_equal_area {
    namespace sphere {
      namespace constants = ::applications::exahype2::ShallowWater;

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
      inline auto forward(const auto lon, const auto lat) {
        constexpr auto lon0         = constants::aeaLon0;
        constexpr auto lat0         = constants::aeaLat0;
        constexpr auto stdParallel1 = constants::aeaStdParallel1;
        constexpr auto stdParallel2 = constants::aeaStdParallel2;
        constexpr auto R            = constants::aeaEarthRadius;

        constexpr auto toRad = M_PI / 180.0;

        constexpr auto stdParallel1Rad = stdParallel1 * toRad;
        constexpr auto stdParallel2Rad = stdParallel2 * toRad;
        constexpr auto lat0Rad         = lat0 * toRad;
        constexpr auto lon0Rad         = lon0 * toRad;

        const auto lonRad = lon * toRad;
        const auto latRad = lat * toRad;

        static const auto n = (std::sin(stdParallel1Rad) + std::sin(stdParallel2Rad)) / 2;
        static const auto C = (std::cos(stdParallel1Rad) * std::cos(stdParallel1Rad))
                              + (2 * n * std::sin(stdParallel1Rad));
        static const auto p0 = R * std::sqrt(C - (2 * n * std::sin(lat0Rad))) / n;

        const auto p     = R * std::sqrt(C - (2 * n * std::sin(latRad))) / n;
        const auto theta = n * (lonRad - lon0Rad);

        const auto x = p * std::sin(theta);
        const auto y = p0 - (p * std::cos(theta));

        return std::pair{x, y};
      };

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
      inline auto inverse(const auto x, const auto y) {
        constexpr auto lon0         = constants::aeaLon0;
        constexpr auto lat0         = constants::aeaLat0;
        constexpr auto stdParallel1 = constants::aeaStdParallel1;
        constexpr auto stdParallel2 = constants::aeaStdParallel2;
        constexpr auto R            = constants::aeaEarthRadius;

        constexpr auto toRad = M_PI / 180.0;

        constexpr auto stdParallel1Rad = stdParallel1 * toRad;
        constexpr auto stdParallel2Rad = stdParallel2 * toRad;
        constexpr auto lat0Rad         = lat0 * toRad;
        constexpr auto lon0Rad         = lon0 * toRad;

        static const auto n = (std::sin(stdParallel1Rad) + std::sin(stdParallel2Rad)) / 2;
        static const auto C = (std::cos(stdParallel1Rad) * std::cos(stdParallel1Rad))
                              + (2 * n * std::sin(stdParallel1Rad));
        static const auto p0 = R * std::sqrt(C - (2 * n * std::sin(lat0Rad))) / n;

        const auto p     = std::sqrt((x * x) + ((p0 - y) * (p0 - y)));
        const auto theta = std::atan2(x, p0 - y);

        const auto lat = std::asin((C - ((p * n / R) * (p * n / R))) / (2 * n));
        const auto lon = lon0Rad + (theta / n);

        return std::pair{lon / toRad, lat / toRad};
      };
    } // namespace sphere
  } // namespace albers_equal_area
} // namespace projection
