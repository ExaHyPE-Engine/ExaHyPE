// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::grid::tests {
  class GridVertexTest;
} // namespace peano4::grid::tests

class peano4::grid::tests::GridVertexTest final: public tarch::tests::TestCase {
private:
  void testFlagsAndState();
  void testCreateGridVertex();
  void testMPIDatatype();

public:
  GridVertexTest();
  ~GridVertexTest() override final = default;

  void run() override final;
};
