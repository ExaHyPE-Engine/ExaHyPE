#pragma once

#include "../Constants.h"

#include <cmath>
#include <utility>

namespace projection {
  namespace albers_equal_area {
    namespace ellipsoid {
      namespace constants = ::applications::exahype2::swe;
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
      inline auto forward(const auto lon, const auto lat) {
        constexpr auto a            = constants::aeaEllipsoidA; // Halbachse des WGS_1984-Ellipsoids (Meter)
        constexpr auto e            = constants::aeaEllipsoidE;
        constexpr auto stdParallel1 = constants::aeaStdParallel1; // Standard Parallel 1 (Grad)
        constexpr auto stdParallel2 = constants::aeaStdParallel2; // Standard Parallel 2 (Grad)
        constexpr auto lon0         = constants::aeaLon0;         // Central Meridian (Grad)
        constexpr auto lat0         = constants::aeaLat0;         // Latitude of Origin (Grad)

        constexpr auto toRad = M_PI / 180.0;

        constexpr auto stdParallel1Rad = stdParallel1 * toRad;
        constexpr auto stdParallel2Rad = stdParallel2 * toRad;
        constexpr auto lat0Rad         = lat0 * toRad;
        constexpr auto lon0Rad         = lon0 * toRad;

        const auto lonRad = lon * toRad;
        const auto latRad = lat * toRad;

        constexpr auto e2         = e * e;
        constexpr auto oneMinusE2 = 1.0 - (e * e);

        auto m = [](const auto phi) { return std::cos(phi) / std::sqrt(1.0 - (e2 * std::sin(phi) * std::sin(phi))); };

        static const auto m1 = m(stdParallel1Rad);
        static const auto m2 = m(stdParallel2Rad);

        auto q = [](const auto phi) {
          const auto sinPhi  = std::sin(phi);
          const auto sinPhi2 = sinPhi * sinPhi;
          return oneMinusE2
                 * ((std::sin(phi) / (1.0 - (e2 * sinPhi2))) - ((1.0 / (2.0 * e)) * std::log((1.0 - (e * std::sin(phi))) / (1.0 + (e * std::sin(phi))))));
        };

        static const auto q0 = q(lat0Rad);
        static const auto q1 = q(stdParallel1Rad);
        static const auto q2 = q(stdParallel2Rad);


        const auto qLat = q(latRad);

        static const auto n  = ((m1 * m1) - (m2 * m2)) / (q2 - q1);
        static const auto C  = (m1 * m1) + (n * q1);
        static const auto p0 = a * std::sqrt(C - (n * q0)) / n;

        const auto p     = a * std::sqrt(C - (n * qLat)) / n;
        const auto theta = n * (lonRad - lon0Rad);

        const auto x = p * std::sin(theta);
        const auto y = p0 - (p * std::cos(theta));

        return std::pair{x, y};
      };

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
      inline auto inverse(const auto x, const auto y) {
        constexpr size_t maxIter = 10;
        constexpr auto   tol     = 1e-12;

        constexpr auto a            = constants::aeaEllipsoidA; // Halbachse des WGS_1984-Ellipsoids (Meter)
        constexpr auto e            = constants::aeaEllipsoidE;
        constexpr auto stdParallel1 = constants::aeaStdParallel1; // Standard Parallel 1 (Grad)
        constexpr auto stdParallel2 = constants::aeaStdParallel2; // Standard Parallel 2 (Grad)
        constexpr auto lon0         = constants::aeaLon0;         // Central Meridian (Grad)
        constexpr auto lat0         = constants::aeaLat0;         // Latitude of Origin (Grad)

        constexpr auto toRad = M_PI / 180.0;

        constexpr auto stdParallel1Rad = stdParallel1 * toRad;
        constexpr auto stdParallel2Rad = stdParallel2 * toRad;
        constexpr auto lat0Rad         = lat0 * toRad;
        constexpr auto lon0Rad         = lon0 * toRad;

        constexpr auto e2         = e * e;
        constexpr auto oneMinusE2 = 1.0 - (e * e);

        auto m = [](const auto phi) { return std::cos(phi) / std::sqrt(1.0 - (e2 * std::sin(phi) * std::sin(phi))); };

        static const auto m1 = m(stdParallel1Rad);
        static const auto m2 = m(stdParallel2Rad);

        auto q = [](const auto phi) {
          const auto sinPhi  = std::sin(phi);
          const auto sinPhi2 = sinPhi * sinPhi;
          return oneMinusE2
                 * ((std::sin(phi) / (1.0 - (e2 * sinPhi2))) - ((1.0 / (2.0 * e)) * std::log((1.0 - (e * std::sin(phi))) / (1.0 + (e * std::sin(phi))))));
        };

        static const auto q0 = q(lat0Rad);
        static const auto q1 = q(stdParallel1Rad);
        static const auto q2 = q(stdParallel2Rad);

        static const auto n  = ((m1 * m1) - (m2 * m2)) / (q2 - q1);
        static const auto C  = (m1 * m1) + (n * q1);
        static const auto p0 = a * std::sqrt(C - (n * q0)) / n;

        const auto p    = std::sqrt((x * x) + ((p0 - y) * (p0 - y)));
        const auto qnew = (C - (p * p * n * n / (a * a))) / n;

        auto phi = std::asin(qnew / 2);

#pragma unroll
        for (int i = 0; i < maxIter; ++i) {
          const auto sinPhi  = std::sin(phi);
          const auto cosPhi  = std::cos(phi);
          const auto sinPhi2 = sinPhi * sinPhi;
          const auto denom   = 1.0 - (e2 * sinPhi2);

          const auto num1 = qnew / oneMinusE2;
          const auto num2 = sinPhi / denom;
          const auto num3 = (1.0 / (2.0 * e)) * std::log((1.0 - e * sinPhi) / (1.0 + e * sinPhi));

          const auto deltaPhi = (1.0 - (e2 * sinPhi2)) * (1.0 - (e2 * sinPhi2)) / (2.0 * cosPhi) * (num1 - num2 + num3);
          const auto phiNew   = phi + deltaPhi;

          if (std::abs(phiNew - phi) < tol) {
            break;
          }
          phi = phiNew;
        }


        const auto theta = std::atan2(x, p0 - y);
        const auto lon   = lon0Rad + (theta / n);

        return std::pair{
          lon / toRad,
          phi / toRad,
        };
      };
    } // namespace ellipsoid
  } // namespace albers_equal_area
} // namespace projection
