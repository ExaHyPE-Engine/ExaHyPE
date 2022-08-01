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
    const double DerivativeOperator[(3+1)*(3+1)];
    const double BasisFunctionValuesLeft[(3+1)];
    const double BasisFunctionValuesRight[(3+1)];

    void UtilsTest();
    void runDGEuler();
    void runDGElastic();

  public:
    CellIntegralTest();

    virtual ~CellIntegralTest() = default;

    void run() override;
};


#endif
