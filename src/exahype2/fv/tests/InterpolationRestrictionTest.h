
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
    void testPiecewiseConstantInterpolationWithTensorProduct1();

    void testPiecewiseConstantInterpolationWithTensorProduct2();

    /**
     * Simple averaging.
     *
     * I test a patch size of 5, work with an overlap of 3 and assume that
     * we have two unknowns per finite differences point.
     *
     * The tangential restriction matrix thus is a 5 times 15 matrix, as we
     * take the five volumes that are tangential to a face in one direction.
     * We assume that the normal points in the x-direction and we speak about
     * the right face of a cell, i.e. the face index equals Dimensions.
     *
     * As we restrict, the right half of the fine grid face (inside) has to
     * be restricted to the right half of the coarse grid face (outside).
     */
    void testAverageRestriction();

  public:
    InterpolationRestrictionTest();

    /**
     * This routine is triggered by the TestCaseCollection
     */
    virtual void run();
};
