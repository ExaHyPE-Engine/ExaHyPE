// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


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
    void testEvaluatePolynomialOrder2();

  public:
    DGUtilsTest();

    virtual ~DGUtilsTest() = default;

    void run() override;
};


