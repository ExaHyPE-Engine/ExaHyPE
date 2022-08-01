// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_DG_TESTS_CELL_INTEGRAL_TEST_H_
#define _EXAHYPE2_DG_TESTS_CELL_INTEGRAL_TEST_H_


#include "tarch/tests/TestCase.h"


namespace exahype2 {
  namespace dg {
    namespace tests {
      class CellIntegralTest;
    }
  }
}


class exahype2::dg::tests::CellIntegralTest : public tarch::tests::TestCase {
  private:
    const double QuadraturePoints[3+1];
    const double QuadratureWeights[3+1];
    const double MassMatrix[(3+1)*(3+1)];
    const double StiffnessOperator[(3+1)*(3+1)];

    void runDGEuler();
    void runDGElastic();
    void testMultiplyWithInvertedMassMatrix_GaussLegendre();

  public:
    CellIntegralTest();

    virtual ~CellIntegralTest() = default;

    void run() override;
};


#endif
