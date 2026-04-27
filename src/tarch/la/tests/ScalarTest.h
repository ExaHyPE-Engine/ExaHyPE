// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::la::tests {
  class ScalarTest;
} // namespace tarch::la::tests

class tarch::la::tests::ScalarTest final: public tarch::tests::TestCase {
private:
  void testComparison();
  void testAbs();
  void testAdditionalScalarFunctions();

public:
  ScalarTest();
  ~ScalarTest() override = default;
  void run() override;
};
