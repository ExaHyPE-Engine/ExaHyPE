// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::grid::tests {
  class GridVertexTest;
}

class peano4::grid::tests::GridVertexTest final: public tarch::tests::TestCase {
private:
  void testCreateGridVertex();

public:
  GridVertexTest();
  ~GridVertexTest() override final = default;

  /**
   * This routine is triggered by the TestCaseCollection
   */
  void run() override final;
};
