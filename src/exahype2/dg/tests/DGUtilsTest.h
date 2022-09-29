// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_DG_TESTS_DGUTILS_TEST_H_
#define _EXAHYPE2_DG_TESTS_DGUTILS_TEST_H_


#include "tarch/tests/TestCase.h"


namespace exahype2 {
  namespace dg {
    namespace tests {
      class DGUtilsTest;
    }
  }
}


class exahype2::dg::tests::DGUtilsTest : public tarch::tests::TestCase {
  private:
    void testGetIndex();
    void testComputeGradientOnConstantSolution();
    void testEvaluatePolynomialOrder1();

  public:
    DGUtilsTest();

    virtual ~DGUtilsTest() = default;

    void run() override;
};


#endif
