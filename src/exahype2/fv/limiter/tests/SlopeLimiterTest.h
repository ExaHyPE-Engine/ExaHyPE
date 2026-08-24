// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::fv::limiter::tests {
  class SlopeLimiterTest;
} // namespace exahype2::fv::limiter::tests

class exahype2::fv::limiter::tests::SlopeLimiterTest final : public tarch::tests::TestCase {
public:
  SlopeLimiterTest();

  void run() override;

private:
  void testZero();
  void testAverage();
  void testMinmod();
  void testVanAlbada();
  void testSuperbee();
  void testMinmodCentral();
  void testTotalVariationBounds();
  void testSinglePrecision();
};
