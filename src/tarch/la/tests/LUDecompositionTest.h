// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::la::tests {
  class LUDecompositionTest;
} // namespace tarch::la::tests

class tarch::la::tests::LUDecompositionTest final: public tarch::tests::TestCase {
private:
  void testBackSubstitution();
  void testInvertUpperTriangular();
  void testStaticBackSubstitution();
  void testStaticInvertUpperTriangular();

public:
  LUDecompositionTest();
  ~LUDecompositionTest() override = default;
  void run() override;
};
