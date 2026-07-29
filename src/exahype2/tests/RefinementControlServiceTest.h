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

  void run() override;

private:
  void testRefineCommandBecomesBandedEvent();
  void testCompleteEraseSiblingGroupBecomesParentEvent();
  void testEraseSiblingGroupIsAnchoredAtDomainOffset();
  void testEraseSiblingGroupIsAnchoredPerAxis();
  void testIncompleteEraseSiblingGroupIsDiscarded();
  void testShiftedEraseSiblingGroupIsDiscarded();
  void testTriggerEventIsRedeliveredForRealisation();
  void testMixedTriggerAndRealiseBatchRedeliversTrigger();
  void testSiblingGroupCompletedAcrossSweepsRedeliversParent();
  void testDeferredTopologyChangeKeepsEventsAlive();
  void testUnchangedEventSetIsSuppressed();
  void testEraseOnlyBatchIsCommitted();
  void testComplementaryEraseSubsetsOfTwoSolversAreDiscarded();
  void testEraseNeedsUnanimousSolverConsent();
  void testReportTracksEveryStageOfARequest();
  void testDeliveryFiltersAgainstTreeExtents();
};
