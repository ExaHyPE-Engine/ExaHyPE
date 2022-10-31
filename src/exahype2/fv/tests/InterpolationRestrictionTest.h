
// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/tests/TestCase.h"


namespace exahype2 {
  namespace fv {
    namespace tests {
      class InterpolationRestrictionTest;
    }
  }
}


class exahype2::fv::tests::InterpolationRestrictionTest : public tarch::tests::TestCase {
  private:
    void testPiecewiseConstantInterpolation();

  public:
    InterpolationRestrictionTest();

    /**
     * This routine is triggered by the TestCaseCollection
     */
    virtual void run();
};
