// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::grid::tests {
  class GridVertexTest;
} // namespace peano4::grid::tests

class peano4::grid::tests::GridVertexTest final : public tarch::tests::TestCase {
public:
  GridVertexTest();
  GridVertexTest(const GridVertexTest&)            = delete;
  GridVertexTest(GridVertexTest&&)                 = delete;
  GridVertexTest& operator=(const GridVertexTest&) = delete;
  GridVertexTest& operator=(GridVertexTest&&)      = delete;
  ~GridVertexTest() override                       = default;

  void run() override;

private:
  void testState();
  void testTheByteRepresentationCarriesEveryField();
  void testSendAndReceiveInARing();
};
