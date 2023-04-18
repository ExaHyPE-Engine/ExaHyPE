// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/tests/TestCase.h"


namespace tarch {
  namespace la {
    namespace tests {
      class MatrixVectorTest;
    }
  }
}

/**
 * Provides tests for types Vector, DynamicVector and all Vector functionality.
 */
class tarch::la::tests::MatrixVectorTest : public tarch::tests::TestCase
{
private:

  /**
   * Tests methods from MatrixVectorOperations.h.
   */
  void testMultiplication ();

  void testForwardSubstitution ();

  void testBackSubstitution ();

  void testSolveSystem3x3 ();

public:

  /**
   * Cosntructor.
   */
  MatrixVectorTest ();

  /**
   * Destructor, empty.
   */
  virtual ~MatrixVectorTest () {};

  /**
   * This routine is triggered by the TestCaseCollection
   */
  virtual void run();

  /**
   * Setup your test case.
   */
  virtual void setUp() {};
};

