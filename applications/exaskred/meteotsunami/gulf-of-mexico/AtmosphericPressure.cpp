// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "AtmosphericPressure.h"

#include "Constants.h"
#include "Projection.h"

#include <cmath>

double applications::exahype2::exaskred::meteotsunami::gulf_of_mexico::atmosphericPressure(double x,
                                                                                           double y,
                                                                                           double t) {
  namespace projection = ::applications::exahype2::exaskred::meteotsunami::projection::aea;

  const auto [lon, lat] = projection::inverseEllipsoid(x, y);

  auto pressureGauss = [](double xr, double yr) {
    constexpr double c  = 1.0;
    constexpr double al = 37.61;
    constexpr double ar = 4.665;
    constexpr double bl = 0.31;
    constexpr double br = 0.5;
    const double     y2 = (yr / c) * (yr / c);

    if (xr < 0.0) {
      return -al * 100.0 * xr * std::exp(-y2 - (xr / bl) * (xr / bl));
    }
    if (xr > 0.0) {
      return -ar * 100.0 * xr * std::exp(-y2 - (xr / br) * (xr / br));
    }
    return 0.0;
  };

  auto pointOnTrack = [](double distance) {
    static const auto   pointA = projection::forwardEllipsoid(pointLonA, pointLatA);
    static const auto   pointB = projection::forwardEllipsoid(pointLonB, pointLatB);
    static const double dx     = pointB.first - pointA.first;
    static const double dy     = pointB.second - pointA.second;
    static const double len    = std::hypot(dx, dy);

    const double alpha = len > 0.0 ? distance / len : 0.0;
    return std::pair{pointA.first + alpha * dx, pointA.second + alpha * dy};
  };

  auto rotateOffsets = [](double lon, double lat, double centerLon, double centerLat) {
    static const auto   pointA = projection::forwardEllipsoid(pointLonA, pointLatA);
    static const auto   pointB = projection::forwardEllipsoid(pointLonB, pointLatB);
    static const double dx     = pointB.first - pointA.first;
    static const double dy     = pointB.second - pointA.second;
    static const double rad    = std::atan2(dy, dx);
    static const double s      = std::sin(rad);
    static const double c      = std::cos(rad);

    const double offsetLon    = std::remainder(lon - centerLon, 360.0);
    const double offsetLat    = lat - centerLat;
    const double scaling      = std::cos(centerLat * std::acos(-1.0) / 180.0);
    const double scaledOffset = offsetLon * scaling;

    const double rotatedLon = scaledOffset * c + offsetLat * s;
    const double rotatedLat = -scaledOffset * s + offsetLat * c;
    return std::pair{std::abs(scaling) > 1e-12 ? rotatedLon / scaling : 0.0, rotatedLat};
  };

  const auto [centerX, centerY]       = pointOnTrack(20.0 * t);
  const auto [centerLon, centerLat]   = projection::inverseEllipsoid(centerX, centerY);
  const auto [rotatedLon, rotatedLat] = rotateOffsets(lon, lat, centerLon, centerLat);

  return pressureGauss(rotatedLon, rotatedLat);
}
