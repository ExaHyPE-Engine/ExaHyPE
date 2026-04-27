// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::la::tests {
  class GramSchmidtTest;
} // namespace tarch::la::tests

class tarch::la::tests::GramSchmidtTest final: public tarch::tests::TestCase {
private:
  void testModifiedGramSchmidt();
  void testDynamicModifiedGramSchmidt();
  void testModifiedGramSchmidtWithSingularColumn();

public:
  GramSchmidtTest();
  ~GramSchmidtTest() override = default;
  void run() override;
};
