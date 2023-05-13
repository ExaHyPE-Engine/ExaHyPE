// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch {
  namespace la {
    namespace tests {
      class DynamicMatrixTest;
    }
  }
}

/**
 * Provides tests for types Vector, DynamicVector and all Vector functionality.
 */
class tarch::la::tests::DynamicMatrixTest: public tarch::tests::TestCase {
  private:

  /**
   * Tests constructors.
   */
  void testBatchedMultiplyAoS();

public:

  /**
   * Cosntructor.
   */
  DynamicMatrixTest ();

  /**
   * Destructor, empty.
   */
  virtual ~DynamicMatrixTest () {}

  /**
   * This routine is triggered by the TestCaseCollection
   */
  virtual void run() override;
};

