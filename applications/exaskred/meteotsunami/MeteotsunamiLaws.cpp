// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MeteotsunamiLaws.h"

#include <cmath>
#include <numbers>

namespace applications::exahype2::exaskred::meteotsunami {
  double coriolisForce::constant(double param) { return param; }

  double coriolisForce::betaPlane(double y, double f0, double beta) { return f0 + beta * y; }

  double coriolisForce::sphere(double latDeg, double omega) {
    return 2.0 * omega * std::sin(std::numbers::pi_v<double> * latDeg / 180.0);
  }

  double bottomFriction::manningFriction(double h, double hu, double hv, double gravity, double param) {
    if (h <= 0.0) {
      return 0.0;
    }

    const double hToSevenThirds = h * h * std::cbrt(h);
    return -gravity * param * param * std::hypot(hu, hv) / hToSevenThirds;
  }
} // namespace applications::exahype2::exaskred::meteotsunami
