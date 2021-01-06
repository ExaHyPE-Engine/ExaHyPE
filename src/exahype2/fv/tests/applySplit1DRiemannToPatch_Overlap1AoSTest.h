
// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once
#include "tarch/tests/TestCase.h"

namespace exahype2 {
  namespace fv {
    namespace tests {
      class applySplit1DRiemannToPatch_Overlap1AoSTest;
    }
  }
}


class exahype2::fv::tests::applySplit1DRiemannToPatch_Overlap1AoSTest : public tarch::tests::TestCase
{
private:

  void testMe();

public:

  /**
   * Constructor.
   */
  applySplit1DRiemannToPatch_Overlap1AoSTest();

  /**
   * Destructor, empty.
   */
  virtual ~applySplit1DRiemannToPatch_Overlap1AoSTest() {}

  /**
   * This routine is triggered by the TestCaseCollection
   */
  virtual void run();

  /**
   * Setup your test case.
   */
  virtual void setUp() {}
};
