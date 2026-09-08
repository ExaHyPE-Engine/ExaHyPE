// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::tests {
  class LimiterStatusTest;
} // namespace exahype2::dg::tests

class exahype2::dg::tests::LimiterStatusTest final : public tarch::tests::TestCase {
public:
  LimiterStatusTest();

  void run() override;

private:
  void testMergeKeepsTheMoreLimitedStatus();
  void testOnlyTroubledAndLimitedRunTheSubcellScheme();
  void testSpreadingGrowsTwoRings();
  void testFaceStatusMarkerIsPerSideAndMonotone();
};
