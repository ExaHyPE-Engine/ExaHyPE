
// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once
#include "tarch/tests/TestCase.h"

namespace exahype2 {
  namespace fv {
    namespace tests {
      class CopyPatchTest;
    }
  }
}


class exahype2::fv::tests::CopyPatchTest : public tarch::tests::TestCase
{
private:

  void testMe();

public:

  /**
   * Constructor.
   */
  CopyPatchTest();

  /**
   * Destructor, empty.
   */
  virtual ~CopyPatchTest() {}

  /**
   * This routine is triggered by the TestCaseCollection
   */
  virtual void run();

  /**
   * Setup your test case.
   */
  virtual void setUp() {}
};
