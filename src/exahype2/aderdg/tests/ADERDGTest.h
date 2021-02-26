// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_ADERDG_TESTS_ADERDGTEST_H_
#define _EXAHYPE2_ADERDG_TESTS_ADERDGTEST_H_

#include "tarch/tests/TestCase.h"

#include <functional>
#include "tarch/la/Vector.h"

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
  const double BarycentricWeights[3+1];
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
  const double testEuler_UIn[5];
  const double testEuler_QOut[5];
  
  const double testAdvection_UIn[4];

  void runADERDGStep(
    std::function< void(
      const double * const __restrict__           Q,
      const tarch::la::Vector<Dimensions,double>& x,
      double                                      t,
      int                                         normal,
      double * __restrict__                       F
    ) >                                          flux,
    std::function< double(
      const double * const __restrict__           Q,
      const tarch::la::Vector<Dimensions,double>& x,
      double                                      t,
      int                                         normal
    ) >                                          maxEigenvalue,
    std::function< void(
      const double* Q,
      const int     i_Q,
      const int     unknown
    )>                                           validatePicardLoopHullAndCorrectorResult,
    const tarch::la::Vector<Dimensions, double>& x,
    const double                                 dx,
    const double                                 t,
    const double                                 dt,
    const int                                    unknowns,
    const int                                    auxiliaryVariables,
    const int                                    order,
    const double*                                test_UIn,
    const bool                                   verbose); 

  
  /**
   * Test interpolation operator.
   */
  void testInterpolate();

  /**
   * Tests ADER-DG with linear advection flux and eigenvalues.
   * We consider a simple PDE with a single variable, mass. 
   * Advection is driven by a constant velocity field.
   * The maximum eigenvalue equals the absolute value of
   * the velocity.
   * This is the most simple system with non-trivial flux.
   */
  void testAdvection();
  
  /**
   * Tests ADER-DG with Euler fluxes and eigenvalues.
   */
  void testEuler();

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
