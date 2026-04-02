// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::grid::tests {
  class TreeIdSetTest;
} // namespace peano4::grid::tests

class peano4::grid::tests::TreeIdSetTest final: public tarch::tests::TestCase {
private:
  void testInsertKeepsSetSemantics();
  void testScalesToLargeNumbersOfTreeIds();

public:
  TreeIdSetTest();
  ~TreeIdSetTest() override final = default;

  void run() override final;
};
