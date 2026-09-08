// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Forcing.h"

/**
 * Coriolis parameter of a rotating sphere at a given latitude.
 *
 * A scenario small enough to treat the parameter as constant passes its own value
 * straight to the source term and never calls this.
 */
double applications::exahype2::exaskred::forcing::coriolisParameterOnSphere(
  const double latitudeInDegrees,
  const double angularVelocity
) {
  return 2.0 * angularVelocity * std::sin(std::numbers::pi_v<double> * latitudeInDegrees / 180.0);
}

/**
 * Manning bed drag, returned as the rate that multiplies the momentum.
 *
 * The depth enters as its seventh third power in the denominator, which reaches the
 * smallest representable number long before the depth itself does, so the quotient
 * is formed against a floored depth. A column at or below that floor carries no
 * resolved flow and is left undamped.
 */
double applications::exahype2::exaskred::forcing::manningFriction(
  const double waterHeight,
  const double momentumX,
  const double momentumY,
  const double gravity,
  const double manningCoefficient,
  const double minimumWaterHeight
) {
  if (not std::isfinite(waterHeight) or waterHeight <= minimumWaterHeight) {
    return 0.0;
  }
  const double momentumMagnitude = std::hypot(momentumX, momentumY);
  if (not std::isfinite(momentumMagnitude)) {
    return 0.0;
  }
  const double flooredHeight       = std::max(waterHeight, minimumWaterHeight);
  const double heightToSevenThirds = flooredHeight * flooredHeight * std::cbrt(flooredHeight);
  return -gravity * manningCoefficient * manningCoefficient * momentumMagnitude / heightToSevenThirds;
}
