// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::tests {
  class RefinementControlServiceTest;
} // namespace exahype2::tests

class exahype2::tests::RefinementControlServiceTest final : public tarch::tests::TestCase {
public:
  RefinementControlServiceTest();
  RefinementControlServiceTest(const RefinementControlServiceTest&)            = delete;
  RefinementControlServiceTest(RefinementControlServiceTest&&)                 = delete;
  RefinementControlServiceTest& operator=(const RefinementControlServiceTest&) = delete;
  RefinementControlServiceTest& operator=(RefinementControlServiceTest&&)      = delete;
  ~RefinementControlServiceTest() override                                     = default;

  void run() override;

private:
  void testRefineCommandNamesTheRequestingCell();
  void testEveryCellIsNamedExactlyOnAnyDomain();
  void testCompleteEraseSiblingGroupBecomesParentEvent();
  void testEraseSiblingGroupIsAnchoredAtDomainOffset();
  void testEraseSiblingGroupIsAnchoredPerAxis();
  void testIncompleteEraseSiblingGroupIsDiscarded();
  void testShiftedEraseSiblingGroupIsDiscarded();
  void testCommittedEventIsConsumedByOneTraversal();
  void testEachBatchCarriesOnlyItsOwnRequests();
  void testSiblingGroupNeedsEveryChildInOneBatch();
  void testUnchangedEventSetIsSuppressed();
  void testRepeatedContributionsReuseTheCommittedSet();
  void testARealisedSetWithoutSweepSuppressesItsRepetition();
  void testARealisedSetWithoutSweepKeepsTheShippedLists();
  void testAPlanChangingSetSurvivesTheRejectionCheck();
  void testAPlanRejectedSetIsRealisedWithoutSweep();
  void testEraseOnlyBatchIsCommitted();
  void testComplementaryEraseSubsetsOfTwoSolversAreDiscarded();
  void testEraseNeedsUnanimousSolverConsent();
  void testReportTracksEveryStageOfARequest();
  void testTheDeltaExchangeTracksEveryRanksWishlist();
};
