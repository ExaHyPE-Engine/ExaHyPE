// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SlopeLimiterTest.h"

#include "exahype2/fv/limiter/SlopeLimiter.h"

exahype2::fv::limiter::tests::SlopeLimiterTest::SlopeLimiterTest() :
  tarch::tests::TestCase("exahype2::fv::limiter::tests::SlopeLimiterTest") {}

void exahype2::fv::limiter::tests::SlopeLimiterTest::run() {
  testMethod(testZero);
  testMethod(testAverage);
  testMethod(testMinmod);
  testMethod(testVanAlbada);
  testMethod(testSuperbee);
  testMethod(testMinmodCentral);
  testMethod(testTotalVariationBounds);
  testMethod(testSinglePrecision);
}

void exahype2::fv::limiter::tests::SlopeLimiterTest::testZero() {
  validateNumericalEquals(exahype2::fv::limiter::Zero::limitedDifference(3.0, 1.0), 0.0);
  validateNumericalEquals(exahype2::fv::limiter::Zero::limitedDifference(-2.0, 5.0), 0.0);
  validateNumericalEquals(exahype2::fv::limiter::Zero::limitedDifference(0.0, 0.0), 0.0);
}

void exahype2::fv::limiter::tests::SlopeLimiterTest::testAverage() {
  validateNumericalEquals(exahype2::fv::limiter::Average::limitedDifference(3.0, 1.0), 2.0);
  validateNumericalEquals(exahype2::fv::limiter::Average::limitedDifference(-1.0, 2.0), 0.5);
  validateNumericalEquals(exahype2::fv::limiter::Average::limitedDifference(0.0, 0.0), 0.0);
}

void exahype2::fv::limiter::tests::SlopeLimiterTest::testMinmod() {
  validateNumericalEquals(exahype2::fv::limiter::Minmod::limitedDifference(2.0, 1.0), 1.0);
  validateNumericalEquals(exahype2::fv::limiter::Minmod::limitedDifference(1.0, 2.0), 1.0);
  validateNumericalEquals(exahype2::fv::limiter::Minmod::limitedDifference(-1.0, -3.0), -1.0);
  validateNumericalEquals(exahype2::fv::limiter::Minmod::limitedDifference(-1.0, 2.0), 0.0);
  validateNumericalEquals(exahype2::fv::limiter::Minmod::limitedDifference(2.0, 0.0), 0.0);
  validateNumericalEquals(exahype2::fv::limiter::Minmod::limitedDifference(0.0, 1.0), 0.0);
  validateNumericalEquals(
    exahype2::fv::limiter::Minmod::limitedDifference(0.3, 0.7),
    exahype2::fv::limiter::Minmod::limitedDifference(0.7, 0.3)
  );
}

void exahype2::fv::limiter::tests::SlopeLimiterTest::testVanAlbada() {
  validateNumericalEquals(exahype2::fv::limiter::VanAlbada::limitedDifference(1.0, 0.0), 0.0);
  validateNumericalEquals(exahype2::fv::limiter::VanAlbada::limitedDifference(1.0, 1.0), 1.0);
  validateNumericalEquals(exahype2::fv::limiter::VanAlbada::limitedDifference(2.0, 1.0), 1.2);
  // The smooth rational formula has no positivity clamp: opposite-sign
  // differences yield a small counter-slope instead of zero.
  validateNumericalEquals(exahype2::fv::limiter::VanAlbada::limitedDifference(-1.0, 2.0), -0.4);
  validateNumericalEquals(
    exahype2::fv::limiter::VanAlbada::limitedDifference(1.0, 3.0),
    exahype2::fv::limiter::VanAlbada::limitedDifference(3.0, 1.0)
  );
}

void exahype2::fv::limiter::tests::SlopeLimiterTest::testSuperbee() {
  validateNumericalEquals(exahype2::fv::limiter::Superbee::limitedDifference(1.0, 0.0), 0.0);
  validateNumericalEquals(exahype2::fv::limiter::Superbee::limitedDifference(1.0, 1.0), 1.0);
  validateNumericalEquals(exahype2::fv::limiter::Superbee::limitedDifference(2.0, 1.0), 2.0);
  validateNumericalEquals(exahype2::fv::limiter::Superbee::limitedDifference(0.5, 1.0), 1.0);
  validateNumericalEquals(exahype2::fv::limiter::Superbee::limitedDifference(0.25, 1.0), 0.5);
  validateNumericalEquals(exahype2::fv::limiter::Superbee::limitedDifference(-1.0, 1.0), 0.0);
}

void exahype2::fv::limiter::tests::SlopeLimiterTest::testMinmodCentral() {
  validateNumericalEquals(exahype2::fv::limiter::MinmodCentral::limitedDifference(1.0, 0.0), 0.0);
  validateNumericalEquals(exahype2::fv::limiter::MinmodCentral::limitedDifference(1.0, 1.0), 1.0);
  validateNumericalEquals(exahype2::fv::limiter::MinmodCentral::limitedDifference(4.0, 1.0), 2.0);
  validateNumericalEquals(exahype2::fv::limiter::MinmodCentral::limitedDifference(0.2, 1.0), 0.4);
  validateNumericalEquals(exahype2::fv::limiter::MinmodCentral::limitedDifference(-1.0, 1.0), 0.0);
}

void exahype2::fv::limiter::tests::SlopeLimiterTest::testTotalVariationBounds() {
  using ScalarLimiter                    = double (*)(double forwardDifference, double backwardDifference);
  const ScalarLimiter monotoneLimiters[] = {
    static_cast<ScalarLimiter>(&exahype2::fv::limiter::Minmod::limitedDifference<double>),
    static_cast<ScalarLimiter>(&exahype2::fv::limiter::Superbee::limitedDifference<double>),
    static_cast<ScalarLimiter>(&exahype2::fv::limiter::MinmodCentral::limitedDifference<double>),
  };
  const double samples[] = {-2.5, -1.0, -0.3, 0.0, 0.4, 1.0, 1.7, 3.0};

  for (const auto limiterFunction : monotoneLimiters) {
    for (const double forwardDifference : samples) {
      for (const double backwardDifference : samples) {
        const double limitedDifference = limiterFunction(forwardDifference, backwardDifference);
        if (forwardDifference * backwardDifference <= 0.0) {
          validateNumericalEquals(limitedDifference, 0.0);
        } else {
          const double smallerMagnitude = std::min(std::abs(forwardDifference), std::abs(backwardDifference));
          validateWithParams3(
            limitedDifference * backwardDifference >= 0.0,
            forwardDifference,
            backwardDifference,
            limitedDifference
          );
          validateWithParams3(
            std::abs(limitedDifference) <= 2.0 * smallerMagnitude + 1e-12,
            forwardDifference,
            backwardDifference,
            limitedDifference
          );
        }
      }
    }
  }
}

void exahype2::fv::limiter::tests::SlopeLimiterTest::testSinglePrecision() {
  validateEquals(exahype2::fv::limiter::Zero::limitedDifference(3.0F, 2.0F), 0.0F);
  validateEquals(exahype2::fv::limiter::Average::limitedDifference(3.0F, 1.0F), 2.0F);
  validateEquals(exahype2::fv::limiter::Minmod::limitedDifference(2.0F, 1.0F), 1.0F);
  validateEquals(exahype2::fv::limiter::VanAlbada::limitedDifference(2.0F, 1.0F), 1.2F);
  validateEquals(exahype2::fv::limiter::Superbee::limitedDifference(2.0F, 1.0F), 2.0F);
  validateEquals(exahype2::fv::limiter::MinmodCentral::limitedDifference(4.0F, 1.0F), 2.0F);
}
