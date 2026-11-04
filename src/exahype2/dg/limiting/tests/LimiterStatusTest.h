// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::limiting::tests {
  class LimiterStatusTest;
} // namespace exahype2::dg::limiting::tests

class exahype2::dg::limiting::tests::LimiterStatusTest final : public tarch::tests::TestCase {
public:
  LimiterStatusTest();
  LimiterStatusTest(const LimiterStatusTest&)            = delete;
  LimiterStatusTest(LimiterStatusTest&&)                 = delete;
  LimiterStatusTest& operator=(const LimiterStatusTest&) = delete;
  LimiterStatusTest& operator=(LimiterStatusTest&&)      = delete;
  ~LimiterStatusTest() override                          = default;

  void run() override;

private:
  void testMergeKeepsTheMoreLimitedStatus();
  void testOnlyTroubledAndLimitedRunTheSubcellScheme();
  void testSpreadingGrowsTwoRings();
};
