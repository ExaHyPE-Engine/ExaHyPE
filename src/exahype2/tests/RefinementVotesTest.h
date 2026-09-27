// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::tests {
  class RefinementVotesTest;
} // namespace exahype2::tests

/**
 * Checks the vote one cell casts on the grid and the two kernels that keep and count votes.
 */
class exahype2::tests::RefinementVotesTest final : public tarch::tests::TestCase {
public:
  RefinementVotesTest();
  RefinementVotesTest(const RefinementVotesTest&)            = delete;
  RefinementVotesTest(RefinementVotesTest&&)                 = delete;
  RefinementVotesTest& operator=(const RefinementVotesTest&) = delete;
  RefinementVotesTest& operator=(RefinementVotesTest&&)      = delete;
  ~RefinementVotesTest() override                            = default;

  void run() override;

private:
  void testACellAboveTheLargestSizeRefines();
  void testTheCriterionDecidesWithinTheAdmissibleSizes();
  void testARefineVoteAtTheSmallestSizeTurnsIntoKeep();
  void testAnEraseVoteThatWouldPassTheLargestSizeTurnsIntoKeep();
  void testTheCountNamesTheVotesThatChangeTheGrid();
  void testTheVotesOnEveryCellReachTheRequests();
  void testGridConstructionRefinesEveryCellAboveTheLargestSize();
  void testOnlyTheListedCellsVote();
};
