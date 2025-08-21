// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/logging/Log.h"
#include "tarch/tests/TestCase.h"

namespace peano4::grid::tests {
  class GridControlEventTest;
} // namespace peano4::grid::tests

class peano4::grid::tests::GridControlEventTest: public tarch::tests::TestCase {
private:
  static tarch::logging::Log _log;

  void testMerge1();
  void testMerge2();
  void testMerge3();
  void testSortWithinMerge();

public:
  GridControlEventTest();
  virtual void run() override;
};
