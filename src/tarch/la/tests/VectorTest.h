// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch {
  namespace la {
    namespace tests {
      class VectorTest;
    }
  }
}

/**
 * Provides tests for types Vector, DynamicVector and all Vector functionality.
 */
class tarch::la::tests::VectorTest : public tarch::tests::TestCase
{
private:

  /**
   * Tests constructors.
   */
  void testConstruction();

  /**
   * Tests methods from VectorAssign.h and operator= from Vector types.
   */
  void testAssignment();

  /**
   * Tests methods from VectorOperations.h.
   */
  void testVectorOperations();

  /**
   * Tests methods from VectorScalarOperations.h.
   */
  void testVectorScalarOperations();

  /**
   * Tests methods from VectorVectorOperations.h.
   */
  void testVectorVectorOperations();

  /**
   * Tests wrapping a raw array with (static) vector semantics.
   */
  void testWrappedVector();

  /**
   * Tests methods from VectorCompare.h.
   */
  void testVectorCompare();

  void testVectorVectorCompare();

public:

  /**
   * Cosntructor.
   */
  VectorTest ();

  /**
   * Destructor, empty.
   */
  virtual ~VectorTest () {};

  /**
   * This routine is triggered by the TestCaseCollection
   */
  virtual void run();

  /**
   * Setup your test case.
   */
  virtual void setUp() {};
};

