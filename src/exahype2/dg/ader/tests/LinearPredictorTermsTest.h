// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::ader::tests {
  class LinearPredictorTermsTest;
} // namespace exahype2::dg::ader::tests

/**
 * Checks the terms one Taylor step of the linear space-time predictor evaluates.
 */
class exahype2::dg::ader::tests::LinearPredictorTermsTest final : public tarch::tests::TestCase {
public:
  LinearPredictorTermsTest();
  LinearPredictorTermsTest(const LinearPredictorTermsTest&)            = delete;
  LinearPredictorTermsTest(LinearPredictorTermsTest&&)                 = delete;
  LinearPredictorTermsTest& operator=(const LinearPredictorTermsTest&) = delete;
  LinearPredictorTermsTest& operator=(LinearPredictorTermsTest&&)      = delete;
  ~LinearPredictorTermsTest() override                                 = default;

  void run() override;

private:
  /**
   * The flux of every node and axis reaches the block of that axis.
   */
  void testTheFluxFillsEveryNodeAndAxis();

  /**
   * A state that is constant over the cell has a zero gradient, so the product adds nothing.
   */
  void testAConstantStateLeavesTheNonconservativeProductAtZero();

  /**
   * A state that is linear along one axis adds the product of its exact slope to the flux of that
   * axis alone.
   */
  void testALinearStateGivesTheExactNonconservativeProduct();

  /**
   * The material parameter matrix multiplies the unknowns of every node and axis and leaves the
   * padding lanes.
   */
  void testTheMaterialParameterMatrixActsOnEveryAxis();
};
