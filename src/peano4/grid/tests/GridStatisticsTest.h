// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::grid::tests {
  class GridStatisticsTest;
} // namespace peano4::grid::tests

class peano4::grid::tests::GridStatisticsTest final: public tarch::tests::TestCase {
private:
  void testStatistics();
  void testClear();
  void testOperators();
  void testMemoryUsage();

public:
  GridStatisticsTest();
  ~GridStatisticsTest() override final = default;

  /**
   * This routine is triggered by the TestCaseCollection
   */
  void run() override final;
};
