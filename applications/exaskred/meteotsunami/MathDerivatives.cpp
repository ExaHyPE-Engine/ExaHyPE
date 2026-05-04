// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MathDerivatives.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace applications::exahype2::exaskred::meteotsunami::numerics {
  double partialX(ScalarField3D f, double x, double y, double t) {
    const double delta = std::cbrt(std::numeric_limits<double>::epsilon()) * std::max(1.0, std::abs(x));
    return (f(x + delta, y, t) - f(x - delta, y, t)) / (2.0 * delta);
  }

  double partialY(ScalarField3D f, double x, double y, double t) {
    const double delta = std::cbrt(std::numeric_limits<double>::epsilon()) * std::max(1.0, std::abs(y));
    return (f(x, y + delta, t) - f(x, y - delta, t)) / (2.0 * delta);
  }

  double partialT(ScalarField3D f, double x, double y, double t) {
    const double delta = std::cbrt(std::numeric_limits<double>::epsilon()) * std::max(1.0, std::abs(t));
    return (f(x, y, t + delta) - f(x, y, t - delta)) / (2.0 * delta);
  }
} // namespace applications::exahype2::exaskred::meteotsunami::numerics
