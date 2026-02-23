// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::grid::tests {
  class GridControlEventTest;
} // namespace peano4::grid::tests

class peano4::grid::tests::GridControlEventTest final: public tarch::tests::TestCase {
private:
  void testOperatorLess();
  void testSort();
  void testSortDeterministicGrid();
  void testMergeTolerance();
  void testMergeAdjacency();
  void testMergeEraseAdjacency();
  void testMergeEraseDoesNotOverMergeLShape();
  void testMerge1();
  void testMerge2();
  void testMerge3();
  void testMPI();

public:
  GridControlEventTest();
  ~GridControlEventTest() override final = default;

  void run() override final;
};
