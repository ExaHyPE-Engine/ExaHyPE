// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch {
  namespace la {
    namespace tests {
      class ScalarTest;
    }
  }
}

class tarch::la::tests::ScalarTest : public tarch::tests::TestCase
{
private:

  void testComparison ();

  void testAbs ();

public:

  /**
   * Constructor.
   */
  ScalarTest ();

  /**
   * Destructor, empty.
   */
  virtual ~ScalarTest () {};

  /**
   * Runs all tests.
   */
  virtual void run ();

  /**
   * Sets up test environment.
   */
  virtual void setUp () {}
};

