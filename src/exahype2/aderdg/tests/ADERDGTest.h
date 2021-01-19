// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_ADERDG_TESTS_ADERDGTEST_H_
#define _EXAHYPE2_ADERDG_TESTS_ADERDGTEST_H_

#include "tarch/tests/TestCase.h"

namespace exahype2 {
  namespace aderdg {
    namespace tests {
      class ADERDGTest;
    }
  }
}

/**
 * Provides tests for types Vector, DynamicVector and all Vector functionality.
 */
class exahype2::aderdg::tests::ADERDGTest : public tarch::tests::TestCase {
private:

  /**
   * Operators
   */
  const double QuadraturePoints[3+1];
  const double QuadratureWeights[3+1];
  const double BasisFunctionValuesLeft[3+1];
  const double BasisFunctionValuesRight[3+1];
  const double DerivativeOperator[(3+1)*(3+1)];
  const double MassMatrix[(3+1)*(3+1)];
  const double StiffnessOperator[(3+1)*(3+1)];
  const double InvertedPredictorLhsOperator[(3+1)*(3+1)];
  const double EquidistantGridProjector[(3+1)*(3+1)];
  const double FineGridProjector[3][(3+1)*(3+1)];

  /**
   * Test input/output data from ExaHyPE1.
   */
  const double spaceTimePredictor_PicardLoop_UIn[80];
  const double spaceTimePredictor_PicardLoop_QOut[320];

  /**
   * Tests ADER-DG Picard loop.
   */
  void test_spaceTimePredictor_PicardLoop_loop();

public:

  /**
   * Cosntructor.
   */
  ADERDGTest ();

  /**
   * Destructor, empty.
   */
  virtual ~ADERDGTest () {};

  /**
   * This routine is triggered by the TestCaseCollection
   */
  void run() override;
};



#endif /* _EXAHYPE2_ADERDG_TESTS_ADERDGTEST_H_ */
