// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::ader::tests {
  class PredictorScratchTest;
} // namespace exahype2::dg::ader::tests

/**
 * Checks the layouts of the predictor scratch.
 */
class exahype2::dg::ader::tests::PredictorScratchTest final : public tarch::tests::TestCase {
public:
  PredictorScratchTest();
  PredictorScratchTest(const PredictorScratchTest&)            = delete;
  PredictorScratchTest(PredictorScratchTest&&)                 = delete;
  PredictorScratchTest& operator=(const PredictorScratchTest&) = delete;
  PredictorScratchTest& operator=(PredictorScratchTest&&)      = delete;
  ~PredictorScratchTest() override                             = default;

  void run() override;

private:
  /**
   * A node of the state and a node of an unknown field grow to the next multiple of the vector width.
   */
  void testEveryNodeIsPaddedToTheVectorWidth();

  /**
   * A cell holds one node per axis combination and a space-time block holds one cell per time node.
   */
  void testTheNodeCountsFollowTheDimensionCount();

  /**
   * The two blocks of the nonlinear scratch place every buffer behind its predecessor without a gap.
   */
  void testNonlinearBlocksFollowEachOtherWithoutOverlap();

  /**
   * The block of the linear scratch places every buffer behind its predecessor without a gap.
   */
  void testLinearBlocksFollowEachOtherWithoutOverlap();

  /**
   * A term the solver switches off occupies no entry and shortens the block it belongs to.
   */
  void testSwitchedOffTermsHaveZeroExtent();

  /**
   * A default-constructed table bundle points nowhere, and every field keeps the table it is given.
   */
  void testTheBasisTablesPointNowhereUntilTheyAreSet();
};
