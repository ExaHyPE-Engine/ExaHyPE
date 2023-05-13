// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch {
  namespace la {
    namespace tests {
      class GramSchmidtTest;
    }
  }
}

/**
 * Provides tests for types Vector, DynamicVector and all Vector functionality.
 */
class tarch::la::tests::GramSchmidtTest: public tarch::tests::TestCase {
  private:

  /**
   * Tests constructors.
   */
  void testModifiedGramSchmidt ();

  public:

    /**
     * Cosntructor.
     */
    GramSchmidtTest ();

    /**
     * Destructor, empty.
     */
    virtual ~GramSchmidtTest () {}

    /**
     * This routine is triggered by the TestCaseCollection
     */
    virtual void run() override;
};

