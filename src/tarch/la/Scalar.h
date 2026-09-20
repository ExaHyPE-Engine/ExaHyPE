// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/accelerator.h"

TARGET_DECLARE
namespace tarch::la {
  constexpr double PI                        = 3.1415926535897932384626433832795028841972;
  constexpr double E                         = 2.7182818284590452353602874713526624977572;
  constexpr double NUMERICAL_ZERO_DIFFERENCE = 1.0e-8;

  /**
   * Like `relativeEps()`, but never scales by values smaller than `1.0`.
   */
  double relativeEpsNormalisedAgainstValueGreaterOne(
    double valueA,
    double valueB = std::numeric_limits<double>::min(),
    double eps    = NUMERICAL_ZERO_DIFFERENCE
  );

  /**
   * Scale `eps` with the larger absolute input value.
   */
  double relativeEps(
    double valueA,
    double valueB = std::numeric_limits<double>::min(),
    double eps    = NUMERICAL_ZERO_DIFFERENCE
  );

  double max(double a, double b, double c);

  double min(double a, double b, double c);

  double pow(double base, double exponent);

  double convertAbsoluteIntoRelativeValue(double referenceValue, double value);
} // namespace tarch::la
TARGET_DECLARE_END
