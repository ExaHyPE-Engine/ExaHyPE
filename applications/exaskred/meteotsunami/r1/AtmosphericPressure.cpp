// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "AtmosphericPressure.h"

#include "Constants.h"

double applications::exahype2::exaskred::meteotsunami::r1::atmosphericPressure(double x, double y, double t) {
  (void)y;

  const double lambda   = speed_pressure * period_pressure;
  const double xShifted = x - 0.75 * lambda;
  const double phase    = xShifted / speed_pressure - t;
  const double tt       = std::round(2.0 * phase / period_pressure);

  if (tt >= 0.0 && tt <= 1.0) {
    const double damping = std::exp(-5.0 * tt);
    const double omega   = 2.0 * std::numbers::pi_v<double> / period_pressure;
    return background_pressure - pressure * damping * std::sin(omega * phase);
  }

  return background_pressure;
}
