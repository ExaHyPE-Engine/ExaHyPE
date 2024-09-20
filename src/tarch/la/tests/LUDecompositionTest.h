// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch {
  namespace la {
    namespace tests {
      class LUDecompositionTest;
    }
  }
}

class tarch::la::tests::LUDecompositionTest : public tarch::tests::TestCase
{
private:

  void testLUNoPivoting();

  void testLU();

  void testInversion0();
  void testInversion1();

public:

  /**
   * Constructor.
   */
  LUDecompositionTest();

  /**
   * Destructor, empty.
   */
  virtual ~LUDecompositionTest() {}

  /**
   * This routine is triggered by the TestCaseCollection
   */
  virtual void run();

  /**
   * Setup your test case.
   */
  virtual void setUp() {}
};

