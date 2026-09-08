// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::grid::tests {
  class GridControlEventTest;
} // namespace peano4::grid::tests

class peano4::grid::tests::GridControlEventTest final : public tarch::tests::TestCase {
public:
  GridControlEventTest();
  GridControlEventTest(const GridControlEventTest&)            = delete;
  GridControlEventTest(GridControlEventTest&&)                 = delete;
  GridControlEventTest& operator=(const GridControlEventTest&) = delete;
  GridControlEventTest& operator=(GridControlEventTest&&)      = delete;
  ~GridControlEventTest() override                             = default;

  void run() override;

private:
  void testOperatorEquals();
  void testOperatorLess();
  void testOrderIsATotalOrder();
  void testCanonicalFormIsIndependentOfInputOrder();
  void testToString();
  void testMPI();
};
