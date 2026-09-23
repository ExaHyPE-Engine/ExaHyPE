// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::plotter::tests {
  class PieceTest;
} // namespace exahype2::plotter::tests

class exahype2::plotter::tests::PieceTest final : public tarch::tests::TestCase {
public:
  PieceTest();
  PieceTest(const PieceTest&)            = delete;
  PieceTest(PieceTest&&)                 = delete;
  PieceTest& operator=(const PieceTest&) = delete;
  PieceTest& operator=(PieceTest&&)      = delete;
  ~PieceTest() override                  = default;

  void run() override;

private:
  void testWritesTheMarkersOfEveryCell();
  void testCreatesTheFieldsInTheOrderOfTheCalls();
  void testMergesTheVerticesOfOnePosition();
  void testKeepsTheVerticesOfOnePositionDistinct();
};
