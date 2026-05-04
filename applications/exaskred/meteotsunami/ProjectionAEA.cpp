// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ProjectionAEA.h"

#include "Constants.h"

#include <cmath>
#include <numbers>

namespace applications::exahype2::exaskred::meteotsunami::projection::aea {
  std::pair<double, double> forwardEllipsoid(double lonDeg, double latDeg) {
    constexpr double toRad = std::numbers::pi_v<double> / 180.0;
    const double     lon0  = aeaLon0 * toRad;
    const double     lat0  = aeaLat0 * toRad;
    const double     phi1  = aeaStdParallel1 * toRad;
    const double     phi2  = aeaStdParallel2 * toRad;
    const double     lon   = lonDeg * toRad;
    const double     lat   = latDeg * toRad;

    const double e2         = aeaEllipsoidE * aeaEllipsoidE;
    const double oneMinusE2 = 1.0 - e2;

    auto m = [e2](double phi) { return std::cos(phi) / std::sqrt(1.0 - e2 * std::sin(phi) * std::sin(phi)); };

    auto q = [e2, oneMinusE2](double phi) {
      const double sinPhi  = std::sin(phi);
      const double sinPhi2 = sinPhi * sinPhi;
      return oneMinusE2
             * (sinPhi / (1.0 - e2 * sinPhi2) - std::log((1.0 - aeaEllipsoidE * sinPhi) / (1.0 + aeaEllipsoidE * sinPhi)) / (2.0 * aeaEllipsoidE));
    };

    const double m1 = m(phi1);
    const double m2 = m(phi2);
    const double q0 = q(lat0);
    const double q1 = q(phi1);
    const double q2 = q(phi2);
    const double ql = q(lat);

    const double n  = (m1 * m1 - m2 * m2) / (q2 - q1);
    const double c  = m1 * m1 + n * q1;
    const double p0 = aeaEllipsoidA * std::sqrt(c - n * q0) / n;
    const double p  = aeaEllipsoidA * std::sqrt(c - n * ql) / n;
    const double th = n * (lon - lon0);

    return {p * std::sin(th), p0 - p * std::cos(th)};
  }

  std::pair<double, double> inverseEllipsoid(double x, double y) {
    constexpr double toRad = std::numbers::pi_v<double> / 180.0;
    constexpr double tol   = 1e-12;
    constexpr int    iters = 10;

    const double lon0 = aeaLon0 * toRad;
    const double lat0 = aeaLat0 * toRad;
    const double phi1 = aeaStdParallel1 * toRad;
    const double phi2 = aeaStdParallel2 * toRad;

    const double e2         = aeaEllipsoidE * aeaEllipsoidE;
    const double oneMinusE2 = 1.0 - e2;

    auto m = [e2](double phi) { return std::cos(phi) / std::sqrt(1.0 - e2 * std::sin(phi) * std::sin(phi)); };

    auto q = [e2, oneMinusE2](double phi) {
      const double sinPhi  = std::sin(phi);
      const double sinPhi2 = sinPhi * sinPhi;
      return oneMinusE2
             * (sinPhi / (1.0 - e2 * sinPhi2) - std::log((1.0 - aeaEllipsoidE * sinPhi) / (1.0 + aeaEllipsoidE * sinPhi)) / (2.0 * aeaEllipsoidE));
    };

    const double m1 = m(phi1);
    const double m2 = m(phi2);
    const double q0 = q(lat0);
    const double q1 = q(phi1);
    const double q2 = q(phi2);

    const double n  = (m1 * m1 - m2 * m2) / (q2 - q1);
    const double c  = m1 * m1 + n * q1;
    const double p0 = aeaEllipsoidA * std::sqrt(c - n * q0) / n;
    const double p  = std::sqrt(x * x + (p0 - y) * (p0 - y));
    const double qn = (c - p * p * n * n / (aeaEllipsoidA * aeaEllipsoidA)) / n;

    double phi = std::asin(qn / 2.0);
    for (int i = 0; i < iters; ++i) {
      const double sinPhi  = std::sin(phi);
      const double cosPhi  = std::cos(phi);
      const double sinPhi2 = sinPhi * sinPhi;

      const double deltaPhi
        = (1.0 - e2 * sinPhi2) * (1.0 - e2 * sinPhi2)
          * (qn / oneMinusE2 - sinPhi / (1.0 - e2 * sinPhi2) + std::log((1.0 - aeaEllipsoidE * sinPhi) / (1.0 + aeaEllipsoidE * sinPhi)) / (2.0 * aeaEllipsoidE))
          / (2.0 * cosPhi);

      const double phiNew = phi + deltaPhi;
      if (std::abs(phiNew - phi) < tol) {
        phi = phiNew;
        break;
      }
      phi = phiNew;
    }

    const double theta = std::atan2(x, p0 - y);
    return {(lon0 + theta / n) / toRad, phi / toRad};
  }
} // namespace applications::exahype2::exaskred::meteotsunami::projection::aea
