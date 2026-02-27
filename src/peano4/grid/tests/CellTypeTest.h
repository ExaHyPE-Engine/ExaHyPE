// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::grid::tests {
  class CellTypeTest;
} // namespace peano4::grid::tests

class peano4::grid::tests::CellTypeTest final: public tarch::tests::TestCase {
private:
  void testGetCellType();

public:
  CellTypeTest();
  ~CellTypeTest() override final = default;

  /**
   * This routine is triggered by the TestCaseCollection
   */
  void run() override final;
};
