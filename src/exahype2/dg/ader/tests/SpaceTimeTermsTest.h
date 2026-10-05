// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::ader::tests {
  class SpaceTimeTermsTest;
} // namespace exahype2::dg::ader::tests

/**
 * Checks the kernels that evaluate one time slice of the nonlinear space-time predictor.
 */
class exahype2::dg::ader::tests::SpaceTimeTermsTest final : public tarch::tests::TestCase {
public:
  SpaceTimeTermsTest();
  SpaceTimeTermsTest(const SpaceTimeTermsTest&)            = delete;
  SpaceTimeTermsTest(SpaceTimeTermsTest&&)                 = delete;
  SpaceTimeTermsTest& operator=(const SpaceTimeTermsTest&) = delete;
  SpaceTimeTermsTest& operator=(SpaceTimeTermsTest&&)      = delete;
  ~SpaceTimeTermsTest() override                           = default;

  void run() override;

private:
  /**
   * The node index carries one entry per axis and rebuilds the flattened index with the first axis running fastest.
   */
  void testTheNodeIndexSplitsTheFlattenedIndex();

  /**
   * A state that holds one value at every node has a zero gradient along every axis.
   */
  void testTheGradientOfAConstantVanishes();

  /**
   * A state that grows linearly along every axis has the exact physical derivative along every axis.
   */
  void testALinearStateHasTheExactGradient();

  /**
   * The flux of one time slice fills one block per axis and enters the time-averaged flux with its weight.
   */
  void testTheFluxIsAskedForOncePerNodeAndAxis();

  /**
   * The diffusive flux of a node reads the gradient of every axis at that node.
   */
  void testTheDiffusiveFluxSeesTheNodalGradients();

  /**
   * Every combination of source terms sums into the right-hand side and the time-averaged source.
   */
  void testEveryTermCombinationAccumulatesIntoOneBuffer();

  /**
   * The first time node overwrites the time averages of the flux and the source with its weighted values.
   */
  void testTheFirstTimeNodeRestartsTheTimeAverages();

  /**
   * Compares the right-hand side and the time-averaged source against the terms the combination switches on.
   */
  void validateSourceCombination(
    const double* rightHandSide,
    const double* timeAveragedSource,
    bool          evaluateNonconservativeProduct,
    bool          evaluateSource,
    bool          evaluateDiffusiveSource
  );
};
