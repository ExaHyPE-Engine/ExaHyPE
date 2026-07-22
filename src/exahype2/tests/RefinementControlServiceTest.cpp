// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RefinementControlServiceTest.h"

#include "exahype2/RefinementControl.h"
#include "exahype2/RefinementControlService.h"
#include "peano4/Globals.h"
#include "peano4/grid/GridControlEvent.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"

namespace {
  // The unit tests bring up and tear down MPI exactly like a full application, so we drive
  // the service through the same collective path the generated solver repository uses. The
  // collective exchange is self-synchronising, so every rank holds an identical
  // committed-event set after each step and the data-dependent drain below stays in
  // lockstep across ranks.
  void drainRefinementControlService() {
    auto& service = exahype2::RefinementControlService::getInstance();
    // Drain pending re-delivered events so each test starts from a clean service state.
    // The iteration cap is only a safety net against an unexpectedly non-draining set.
    int remainingSweeps = 16;
    do {
      service.finishDynamicMeshRefinementStep(true);
    } while (not service.getGridControlEvents().empty() and --remainingSweeps > 0);
  }

  tarch::la::Vector<DIMENSIONS, double> uniformVector(double value) {
    return tarch::la::Vector<DIMENSIONS, double>(value);
  }

  bool isParentEraseEvent(
    const peano4::grid::GridControlEvent&        event,
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset
  ) {
    return event.getRefinementControl() == peano4::grid::GridControlEvent::RefinementControl::Erase
      and event.offset == domainOffset and event.width == uniformVector(3.0) and event.targetH == uniformVector(3.0);
  }

  // Add one erase command, but assign each sibling to a single rank in round-robin order.
  // Under MPI the sibling group is therefore split across the ranks, so the collective
  // gather inside finishRefinementCriterionStep is what reassembles it; in a serial run
  // every command lands on the only rank, so behaviour is unchanged.
  void addEraseCommand(
    exahype2::RefinementControl&                 control,
    const tarch::la::Vector<DIMENSIONS, double>& offset,
    int                                          siblingIndex
  ) {
    if (siblingIndex % tarch::mpi::Rank::getInstance().getNumberOfRanks()
        != tarch::mpi::Rank::getInstance().getRank()) {
      return;
    }
    control.addCommand(offset + 0.5 * uniformVector(1.0), uniformVector(1.0), exahype2::RefinementCommand::Erase);
  }

  // Refine commands need no sibling group, so one rank raising it is enough; the collective
  // gather hands it to everybody else.
  void addRefineCommand(exahype2::RefinementControl& control, const tarch::la::Vector<DIMENSIONS, double>& offset) {
    if (tarch::mpi::Rank::getInstance().getRank() != 0) {
      return;
    }
    control.addCommand(offset + 0.5 * uniformVector(1.0), uniformVector(1.0), exahype2::RefinementCommand::Refine);
  }

  void addEraseSiblingGroupChildRange(
    exahype2::RefinementControl&                 control,
    const tarch::la::Vector<DIMENSIONS, double>& parentOffset,
    int                                          firstChild,
    int                                          childEnd,
    int                                          skippedChild
  ) {
    int child = 0;
#if DIMENSIONS == 3
    for (int k = 0; k < 3; k++) {
#endif
      for (int j = 0; j < 3; j++) {
        for (int i = 0; i < 3; i++) {
          tarch::la::Vector<DIMENSIONS, double> offset = parentOffset;
          offset(0) += static_cast<double>(i);
          offset(1) += static_cast<double>(j);
#if DIMENSIONS == 3
          offset(2) += static_cast<double>(k);
#endif
          if (child >= firstChild and child < childEnd and child != skippedChild) {
            addEraseCommand(control, offset, child);
          }
          child++;
        }
      }
#if DIMENSIONS == 3
    }
#endif
  }

  void addEraseSiblingGroup(
    exahype2::RefinementControl&                 control,
    const tarch::la::Vector<DIMENSIONS, double>& parentOffset,
    int                                          skippedChild
  ) {
    addEraseSiblingGroupChildRange(control, parentOffset, 0, THREE_POWER_D, skippedChild);
  }

  void commitCriterionStep(exahype2::RefinementControl& control) {
    auto& service = exahype2::RefinementControlService::getInstance();
    service.merge(control);
    service.finishRefinementCriterionStep(true);
  }
} // namespace

exahype2::tests::RefinementControlServiceTest::RefinementControlServiceTest() :
  TestCase("exahype2::tests::RefinementControlServiceTest") {}

void exahype2::tests::RefinementControlServiceTest::run() {
  testMethod(testRefineCommandBecomesBandedEvent);
  testMethod(testCompleteEraseSiblingGroupBecomesParentEvent);
  testMethod(testEraseSiblingGroupIsAnchoredAtDomainOffset);
  testMethod(testEraseSiblingGroupIsAnchoredPerAxis);
  testMethod(testIncompleteEraseSiblingGroupIsDiscarded);
  testMethod(testShiftedEraseSiblingGroupIsDiscarded);
  testMethod(testTriggerEventIsRedeliveredForRealisation);
  testMethod(testMixedTriggerAndRealiseBatchRedeliversTrigger);
  testMethod(testSiblingGroupCompletedAcrossSweepsRedeliversParent);
  testMethod(testDeferredTopologyChangeKeepsEventsAlive);
  testMethod(testUnchangedEventSetIsSuppressed);
  testMethod(testEraseOnlyBatchIsCommitted);
  testMethod(testComplementaryEraseSubsetsOfTwoSolversAreDiscarded);
  testMethod(testEraseNeedsUnanimousSolverConsent);
}

void exahype2::tests::RefinementControlServiceTest::testEraseOnlyBatchIsCommitted() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();
  service.setDomainOffset(uniformVector(0.0));

  // A batch whose committed events only erase still realises: once a feature has decayed
  // or left the domain the criterion never refines again, and the remaining coarsening
  // must not wait for a refining batch that never comes.
  exahype2::RefinementControl control;
  addEraseSiblingGroup(control, uniformVector(42.0), -1);
  commitCriterionStep(control);

  const auto events = service.getGridControlEvents();
  validateEquals(events.size(), 1u);
  validateTrue(isParentEraseEvent(events[0], uniformVector(42.0)));

  drainRefinementControlService();
}

void exahype2::tests::RefinementControlServiceTest::testRefineCommandBecomesBandedEvent() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();
  service.setDomainOffset(uniformVector(0.0));

  exahype2::RefinementControl control;
  addRefineCommand(control, uniformVector(1.0));
  commitCriterionStep(control);

  const auto events = service.getGridControlEvents();
  validateEquals(events.size(), 1u);
  validateTrue(events[0].getRefinementControl() == peano4::grid::GridControlEvent::RefinementControl::Refine);
  validateTrue(events[0].offset == uniformVector(0.0));
  validateTrue(events[0].width == uniformVector(3.0));
  validateTrue(events[0].targetH == uniformVector(1.0 / 3.0));

  drainRefinementControlService();
}

void exahype2::tests::RefinementControlServiceTest::testCompleteEraseSiblingGroupBecomesParentEvent() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();
  service.setDomainOffset(uniformVector(0.0));

  // The refine command keeps the committed set distinct from every set realised before, so
  // the commit is not suppressed.
  exahype2::RefinementControl control;
  addEraseSiblingGroup(control, uniformVector(0.0), -1);
  addRefineCommand(control, uniformVector(50.0));
  commitCriterionStep(control);

  const auto events = service.getGridControlEvents();
  validateEquals(events.size(), 2u);
  validateTrue(isParentEraseEvent(events[0], uniformVector(0.0)) or isParentEraseEvent(events[1], uniformVector(0.0)));

  drainRefinementControlService();
}

void exahype2::tests::RefinementControlServiceTest::testEraseSiblingGroupIsAnchoredAtDomainOffset() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();
  // The parent grid starts at the domain offset, so a group anchored there is complete even
  // though none of its coordinates is a multiple of the parent size.
  const auto domainOffset = uniformVector(0.5);
  service.setDomainOffset(domainOffset);

  exahype2::RefinementControl control;
  addEraseSiblingGroup(control, domainOffset, -1);
  addRefineCommand(control, uniformVector(53.0));
  commitCriterionStep(control);

  const auto events = service.getGridControlEvents();
  validateEquals(events.size(), 2u);
  validateTrue(isParentEraseEvent(events[0], domainOffset) or isParentEraseEvent(events[1], domainOffset));

  drainRefinementControlService();
  service.setDomainOffset(uniformVector(0.0));
}

// createEraseParent() anchors each axis on its own component of the domain offset. An offset
// that differs per axis is what separates that from a routine reading component zero for all
// axes, or from two axes being transposed.
void exahype2::tests::RefinementControlServiceTest::testEraseSiblingGroupIsAnchoredPerAxis() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();

  tarch::la::Vector<DIMENSIONS, double> domainOffset;
  for (int d = 0; d < DIMENSIONS; d++) {
    domainOffset(d) = 0.5 + 0.25 * static_cast<double>(d);
  }
  service.setDomainOffset(domainOffset);

  exahype2::RefinementControl control;
  addEraseSiblingGroup(control, domainOffset, -1);
  addRefineCommand(control, uniformVector(53.0));
  commitCriterionStep(control);

  const auto events = service.getGridControlEvents();
  validateEquals(events.size(), 2u);
  validateTrue(isParentEraseEvent(events[0], domainOffset) or isParentEraseEvent(events[1], domainOffset));

  drainRefinementControlService();
  service.setDomainOffset(uniformVector(0.0));
}

void exahype2::tests::RefinementControlServiceTest::testIncompleteEraseSiblingGroupIsDiscarded() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();
  service.setDomainOffset(uniformVector(0.0));

  // Leave out the centre child, the one a feature would keep refined. The refine command
  // proves the batch is processed: an incorrectly promoted group would surface as a
  // second event.
  exahype2::RefinementControl control;
  addEraseSiblingGroup(control, uniformVector(0.0), THREE_POWER_D / 2);
  addRefineCommand(control, uniformVector(56.0));
  commitCriterionStep(control);

  validateEquals(service.getGridControlEvents().size(), 1u);
  validateTrue(
    service.getGridControlEvents()[0].getRefinementControl()
    == peano4::grid::GridControlEvent::RefinementControl::Refine
  );

  drainRefinementControlService();
}

void exahype2::tests::RefinementControlServiceTest::testShiftedEraseSiblingGroupIsDiscarded() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();
  service.setDomainOffset(uniformVector(0.0));

  // A full block of 3^d leaves that spreads over two parents fills neither of them.
  exahype2::RefinementControl           control;
  tarch::la::Vector<DIMENSIONS, double> shiftedOrigin = uniformVector(0.0);
  shiftedOrigin(0)                                    = 1.0;
  addEraseSiblingGroup(control, shiftedOrigin, -1);
  addRefineCommand(control, uniformVector(59.0));
  commitCriterionStep(control);

  validateEquals(service.getGridControlEvents().size(), 1u);
  validateTrue(
    service.getGridControlEvents()[0].getRefinementControl()
    == peano4::grid::GridControlEvent::RefinementControl::Refine
  );

  drainRefinementControlService();
}

void exahype2::tests::RefinementControlServiceTest::testTriggerEventIsRedeliveredForRealisation() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();
  service.setDomainOffset(uniformVector(0.0));

  // The service remembers the last realised set across tests, so every test works on a cell
  // of its own and none of them is suppressed by a predecessor.
  exahype2::RefinementControl control;
  addRefineCommand(control, uniformVector(10.0));
  commitCriterionStep(control);

  const auto triggerEvents = service.getGridControlEvents();
  validateEquals(triggerEvents.size(), 1u);

  // Peano needs the event once more to realise the topology change it just triggered.
  service.finishDynamicMeshRefinementStep(true);
  const auto realiseEvents = service.getGridControlEvents();
  validateEquals(realiseEvents.size(), 1u);
  validateTrue(realiseEvents[0] == triggerEvents[0]);

  // After the realisation the service has nothing left to hand out.
  service.finishDynamicMeshRefinementStep(true);
  validateTrue(service.getGridControlEvents().empty());

  drainRefinementControlService();
}

void exahype2::tests::RefinementControlServiceTest::testMixedTriggerAndRealiseBatchRedeliversTrigger() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();
  service.setDomainOffset(uniformVector(0.0));

  // First commit: trigger A. Its re-delivery for the realisation traversal is now pending.
  exahype2::RefinementControl controlA;
  addRefineCommand(controlA, uniformVector(30.0));
  commitCriterionStep(controlA);
  validateEquals(service.getGridControlEvents().size(), 1u);

  // Second commit mixes A's pending realisation with a fresh trigger B (a cell far enough
  // away that the two banded refine boxes cannot fuse). Both must be active for the next
  // traversal.
  exahype2::RefinementControl controlB;
  addRefineCommand(controlB, uniformVector(36.0));
  service.merge(controlB);
  service.finishDynamicMeshRefinementStep(true);
  validateEquals(service.getGridControlEvents().size(), 2u);

  // B still needs its own realisation traversal, so it survives into a third delivery even
  // though it travelled in one batch with A's realisation.
  service.finishDynamicMeshRefinementStep(true);
  const auto realiseEvents = service.getGridControlEvents();
  validateEquals(realiseEvents.size(), 1u);
  validateTrue(realiseEvents[0].offset == uniformVector(35.0));

  service.finishDynamicMeshRefinementStep(true);
  validateTrue(service.getGridControlEvents().empty());

  drainRefinementControlService();
}

void exahype2::tests::RefinementControlServiceTest::testSiblingGroupCompletedAcrossSweepsRedeliversParent() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();
  service.setDomainOffset(uniformVector(0.0));

  // First commit: all children but one request the erase, plus a refine command that keeps
  // the batch from being suppressed. The group is incomplete, so only the refine is
  // committed, and the children are staged for their second delivery.
  exahype2::RefinementControl incompleteControl;
  addEraseSiblingGroupChildRange(incompleteControl, uniformVector(39.0), 0, THREE_POWER_D - 1, -1);
  addRefineCommand(incompleteControl, uniformVector(62.0));
  commitCriterionStep(incompleteControl);
  validateEquals(service.getGridControlEvents().size(), 1u);
  validateTrue(
    service.getGridControlEvents()[0].getRefinementControl()
    == peano4::grid::GridControlEvent::RefinementControl::Refine
  );

  // The missing child arrives while the other children are on their spent second delivery.
  // The group completes, so the parent must be committed now and once more for the
  // realisation traversal, even though its constituents' deliveries are used up.
  exahype2::RefinementControl completingControl;
  addEraseSiblingGroupChildRange(completingControl, uniformVector(39.0), THREE_POWER_D - 1, THREE_POWER_D, -1);
  service.merge(completingControl);
  service.finishDynamicMeshRefinementStep(true);

  const auto triggerEvents = service.getGridControlEvents();
  validateEquals(triggerEvents.size(), 2u);
  validateTrue(
    isParentEraseEvent(triggerEvents[0], uniformVector(39.0))
    or isParentEraseEvent(triggerEvents[1], uniformVector(39.0))
  );

  service.finishDynamicMeshRefinementStep(true);
  const auto realiseEvents = service.getGridControlEvents();
  validateEquals(realiseEvents.size(), 1u);
  validateTrue(isParentEraseEvent(realiseEvents[0], uniformVector(39.0)));

  service.finishDynamicMeshRefinementStep(true);
  validateTrue(service.getGridControlEvents().empty());

  drainRefinementControlService();
}

void exahype2::tests::RefinementControlServiceTest::testDeferredTopologyChangeKeepsEventsAlive() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();
  service.setDomainOffset(uniformVector(0.0));

  exahype2::RefinementControl control;
  addRefineCommand(control, uniformVector(40.0));
  commitCriterionStep(control);
  validateEquals(service.getGridControlEvents().size(), 1u);

  // The realisation traversal deferred vertex changes: the event set stays alive for
  // another delivery, so it is not yet recorded as realised.
  service.finishDynamicMeshRefinementStep(true, 3);
  validateEquals(service.getGridControlEvents().size(), 1u);
  service.finishDynamicMeshRefinementStep(true, 1);
  validateEquals(service.getGridControlEvents().size(), 1u);

  // A traversal that defers nothing drains the set and records it as realised.
  service.finishDynamicMeshRefinementStep(true, 0);
  validateTrue(service.getGridControlEvents().empty());

  // Only now is the identical command suppressed as already realised.
  exahype2::RefinementControl identicalControl;
  addRefineCommand(identicalControl, uniformVector(40.0));
  commitCriterionStep(identicalControl);
  validateTrue(service.getGridControlEvents().empty());

  drainRefinementControlService();
}

void exahype2::tests::RefinementControlServiceTest::testUnchangedEventSetIsSuppressed() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();
  service.setDomainOffset(uniformVector(0.0));

  {
    exahype2::RefinementControl control;
    addRefineCommand(control, uniformVector(20.0));
    commitCriterionStep(control);
    validateEquals(service.getGridControlEvents().size(), 1u);
  }
  drainRefinementControlService();

  // Reissuing the very same command cannot change the mesh again, so it is dropped.
  {
    exahype2::RefinementControl control;
    addRefineCommand(control, uniformVector(20.0));
    commitCriterionStep(control);
    validateTrue(service.getGridControlEvents().empty());
  }

  // A command for a different cell is a different set and survives.
  {
    exahype2::RefinementControl control;
    addRefineCommand(control, uniformVector(23.0));
    commitCriterionStep(control);
    validateEquals(service.getGridControlEvents().size(), 1u);
  }

  drainRefinementControlService();
}

void exahype2::tests::RefinementControlServiceTest::testComplementaryEraseSubsetsOfTwoSolversAreDiscarded() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();
  service.setDomainOffset(uniformVector(0.0));
  service.setNumberOfSolvers(2);

  // Two solvers split one sibling group between them: together they cover all children, but
  // neither solver consents to the complete group, so nothing may coarsen. The refine
  // command proves the batch is processed.
  exahype2::RefinementControl controlOfFirstSolver(0);
  exahype2::RefinementControl controlOfSecondSolver(1);
  addEraseSiblingGroupChildRange(controlOfFirstSolver, uniformVector(30.0), 0, THREE_POWER_D / 2, -1);
  addEraseSiblingGroupChildRange(controlOfSecondSolver, uniformVector(30.0), THREE_POWER_D / 2, THREE_POWER_D, -1);
  addRefineCommand(controlOfFirstSolver, uniformVector(65.0));
  service.merge(controlOfFirstSolver);
  commitCriterionStep(controlOfSecondSolver);

  validateEquals(service.getGridControlEvents().size(), 1u);
  validateTrue(
    service.getGridControlEvents()[0].getRefinementControl()
    == peano4::grid::GridControlEvent::RefinementControl::Refine
  );

  drainRefinementControlService();
  service.setNumberOfSolvers(1);
}

void exahype2::tests::RefinementControlServiceTest::testEraseNeedsUnanimousSolverConsent() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();
  service.setDomainOffset(uniformVector(0.0));
  service.setNumberOfSolvers(2);

  // Both solvers request the complete group: the erase is unanimous and realised. The
  // refine command keeps the committed set distinct from every set realised before.
  {
    exahype2::RefinementControl controlOfFirstSolver(0);
    exahype2::RefinementControl controlOfSecondSolver(1);
    addEraseSiblingGroup(controlOfFirstSolver, uniformVector(33.0), -1);
    addEraseSiblingGroup(controlOfSecondSolver, uniformVector(33.0), -1);
    addRefineCommand(controlOfFirstSolver, uniformVector(68.0));
    service.merge(controlOfFirstSolver);
    commitCriterionStep(controlOfSecondSolver);

    const auto events = service.getGridControlEvents();
    validateEquals(events.size(), 2u);
    validateTrue(
      isParentEraseEvent(events[0], uniformVector(33.0)) or isParentEraseEvent(events[1], uniformVector(33.0))
    );
  }
  drainRefinementControlService();

  // One solver keeps every sibling (and hence submits no erase request): its missing consent
  // vetoes the other solver's complete group.
  {
    exahype2::RefinementControl controlOfFirstSolver(0);
    addEraseSiblingGroup(controlOfFirstSolver, uniformVector(36.0), -1);
    addRefineCommand(controlOfFirstSolver, uniformVector(71.0));
    commitCriterionStep(controlOfFirstSolver);

    validateEquals(service.getGridControlEvents().size(), 1u);
    validateTrue(
      service.getGridControlEvents()[0].getRefinementControl()
      == peano4::grid::GridControlEvent::RefinementControl::Refine
    );
  }

  drainRefinementControlService();
  service.setNumberOfSolvers(1);
}
