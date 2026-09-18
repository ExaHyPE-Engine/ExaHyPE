// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RefinementControlServiceTest.h"

#include "exahype2/RefinementControl.h"
#include "exahype2/RefinementControlService.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "peano4/grid/AutomatonState.h"
#include "peano4/grid/GridControlEvent.h"
#include "peano4/sfc/ActiveCurve.h"
#include "peano4/topology/Tree.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"

namespace {
  constexpr int SubdivisionFactor = peano4::sfc::ActiveCurve::SubdivisionFactor;
  constexpr int ChildrenPerCell   = peano4::sfc::ActiveCurve::ChildrenPerCell;

  // A request names the cell it was raised for, so the tests need a domain whose lattice
  // carries their unit-sized cells. The leaf level is the first whose cell count per axis
  // covers every coordinate the tests use, and a sibling group's parent then sits one level
  // above it.
  constexpr int smallestLevelCovering(const int coordinates) {
    int level        = 0;
    int cellsPerAxis = 1;
    while (cellsPerAxis < coordinates) {
      cellsPerAxis *= SubdivisionFactor;
      level++;
    }
    return level;
  }

  constexpr int    LeafLevel       = smallestLevelCovering(81);
  constexpr int    ParentLevel     = LeafLevel - 1;
  constexpr double TestDomainWidth = peano4::Power<SubdivisionFactor, LeafLevel>;

  // Three levels beyond the leaf level, so the sweep keeps a margin over the meshes a run
  // configures.
  constexpr int DeepestTestLevel = LeafLevel + 3;

  tarch::la::Vector<DIMENSIONS, double> uniformVector(double value) {
    return tarch::la::Vector<DIMENSIONS, double>(value);
  }

  /** Returns the offset of the parent cell number parentNumber along every axis, in leaf cells. */
  tarch::la::Vector<DIMENSIONS, double> parentCellOffset(int parentNumber) {
    return uniformVector(static_cast<double>(parentNumber * SubdivisionFactor));
  }

  struct TestDomain {
    double offset[3];
    double width[3];
  };

  constexpr TestDomain TestDomains[] = {
    {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}},
    {{-1.0, -1.0, -1.0}, {2.0, 2.0, 2.0}},
    {{0.0, 0.0, 0.0}, {10.0, 10.0, 10.0}},
    {{-4.5, 0.0, -4.5}, {9.0, 9.0, 9.0}},
    {{0.0, 0.0, 0.0}, {1.0, 3.0, 7.0}},
    {{-0.25, -7.5, 3.0}, {0.5, 15.0, 6.0}},
    {{414895.5, 6489600.0, -3000.0}, {1900.0, 1400.0, 3000.0}},
    {{0.001, -0.002, 0.0}, {0.001, 0.002, 0.0005}},
    {{-1.0e6, 0.0, 1.0e6}, {1.0e6, 3.0e6, 2.0e6}},
  };

  constexpr int numberOfTestDomains() { return static_cast<int>(std::size(TestDomains)); }

  tarch::la::Vector<DIMENSIONS, double> testDomainOffset(int domainIndex) {
    tarch::la::Vector<DIMENSIONS, double> offset;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      offset(axis) = TestDomains[domainIndex].offset[axis];
    }
    return offset;
  }

  tarch::la::Vector<DIMENSIONS, double> testDomainWidth(int domainIndex) {
    tarch::la::Vector<DIMENSIONS, double> width;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      width(axis) = TestDomains[domainIndex].width[axis];
    }
    return width;
  }

  // Each axis carries its own index, the level's cell count divided by the subdivision
  // factor apart from its neighbour.
  tarch::la::Vector<DIMENSIONS, int> sweepPosition(int index, int cellsPerAxisOnLevel) {
    tarch::la::Vector<DIMENSIONS, int> position;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      position(axis) = (index + axis * (cellsPerAxisOnLevel / SubdivisionFactor)) % cellsPerAxisOnLevel;
    }
    return position;
  }

  // Descends to one cell along its lattice position read in the base of the subdivision
  // factor, so the command carries the width and centre a traversal computes.
  peano4::grid::AutomatonState descendToCell(
    const peano4::grid::AutomatonState&       rootState,
    int                                       targetLevel,
    const tarch::la::Vector<DIMENSIONS, int>& latticeIndex
  ) {
    auto state = rootState;
    for (int level = 1; level <= targetLevel; level++) {
      peano4::grid::AutomatonState children[ChildrenPerCell];
      peano4::grid::AutomatonState::refineState(state, children);

      tarch::la::Vector<DIMENSIONS, int> childPosition;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        childPosition(axis) =
          (latticeIndex(axis) / peano4::topology::cellsPerAxis(targetLevel - level)) % SubdivisionFactor;
      }
      state = children[peano4::dLinearised(childPosition, SubdivisionFactor)];
    }
    return state;
  }

  // Every rank holds the same committed-event set after each step, so this loop runs the
  // same number of sweeps on every rank.
  void drainRefinementControlService() {
    auto& service         = exahype2::RefinementControlService::getInstance();
    int   remainingSweeps = 16;
    do {
      service.finishDynamicMeshRefinementStep();
    } while (not service.getGridControlEvents().empty() and --remainingSweeps > 0);
  }

  // A promoted group names the parent cell one level above the erased leaves, at the
  // lattice position the group's own position divided by the subdivision factor.
  bool isParentEraseEvent(const peano4::grid::GridControlEvent& event, int parentLatticePosition) {
    bool namesTheParent = event.getRefinementControl() == peano4::grid::GridControlEvent::RefinementControl::Erase
      and event.level == ParentLevel;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      namesTheParent = namesTheParent and event.latticeIndex(axis) == parentLatticePosition;
    }
    return namesTheParent;
  }

  // Adds one erase command on the rank the sibling index selects in round-robin order, so
  // the gather inside finishRefinementCriterionStep reassembles the sibling group.
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

  // Raises the refine command on the rank the raiser number selects in round-robin order,
  // so on several ranks the wishlists of all ranks change between steps.
  void addRefineCommandOnRank(
    exahype2::RefinementControl&                 control,
    const tarch::la::Vector<DIMENSIONS, double>& offset,
    int                                          raiserNumber
  ) {
    if (raiserNumber % tarch::mpi::Rank::getInstance().getNumberOfRanks()
        != tarch::mpi::Rank::getInstance().getRank()) {
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
    for (int child = 0; child < ChildrenPerCell; child++) {
      tarch::la::Vector<DIMENSIONS, double> offset          = parentOffset;
      int                                   remainingDigits = child;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        offset(axis) += static_cast<double>(remainingDigits % SubdivisionFactor);
        remainingDigits /= SubdivisionFactor;
      }
      if (child >= firstChild and child < childEnd and child != skippedChild) {
        addEraseCommand(control, offset, child);
      }
    }
  }

  void addEraseSiblingGroup(
    exahype2::RefinementControl&                 control,
    const tarch::la::Vector<DIMENSIONS, double>& parentOffset,
    int                                          skippedChild
  ) {
    addEraseSiblingGroupChildRange(control, parentOffset, 0, ChildrenPerCell, skippedChild);
  }

  void commitCriterionStep(exahype2::RefinementControl& control) {
    auto& service = exahype2::RefinementControlService::getInstance();
    service.merge(control);
    service.finishRefinementCriterionStep();
  }

  std::string describeServiceState(
    std::size_t numberOfNewLocalEvents,
    std::size_t numberOfCommittedEvents,
    std::size_t numberOfLastRealisedEvents,
    bool        topologyChangedSinceLastTimeStep,
    int         numberOfSolvers
  ) {
    return "RefinementControlService{#NewLocalEvents=" + std::to_string(numberOfNewLocalEvents) + ", #CommittedEvents="
      + std::to_string(numberOfCommittedEvents) + ", #LastRealisedEvents=" + std::to_string(numberOfLastRealisedEvents)
      + ", TopologyChangedSinceLastTimeStep=" + (topologyChangedSinceLastTimeStep ? "true" : "false")
      + ", #Solvers=" + std::to_string(numberOfSolvers) + "}";
  }

  // addRefineCommand raises its command on rank 0 only, so only that rank holds a local
  // request before the gather hands the resulting event to everybody.
  std::size_t numberOfLocalRefineRequests() { return tarch::mpi::Rank::getInstance().getRank() == 0 ? 1U : 0U; }
} // namespace

exahype2::tests::RefinementControlServiceTest::RefinementControlServiceTest() :
  TestCase("exahype2::tests::RefinementControlServiceTest") {}

void exahype2::tests::RefinementControlServiceTest::run() {
  testMethod(testRefineCommandNamesTheRequestingCell);
  testMethod(testEveryCellIsNamedExactlyOnAnyDomain);
  testMethod(testCompleteEraseSiblingGroupBecomesParentEvent);
  testMethod(testEraseSiblingGroupIsAnchoredAtDomainOffset);
  testMethod(testEraseSiblingGroupIsAnchoredPerAxis);
  testMethod(testIncompleteEraseSiblingGroupIsDiscarded);
  testMethod(testShiftedEraseSiblingGroupIsDiscarded);
  testMethod(testCommittedEventIsConsumedByOneTraversal);
  testMethod(testEachBatchCarriesOnlyItsOwnRequests);
  testMethod(testSiblingGroupNeedsEveryChildInOneBatch);
  testMethod(testUnchangedEventSetIsSuppressed);
  testMethod(testRepeatedContributionsReuseTheCommittedSet);
  testMethod(testARealisedSetWithoutSweepSuppressesItsRepetition);
  testMethod(testARealisedSetWithoutSweepKeepsTheShippedLists);
  testMethod(testEraseOnlyBatchIsCommitted);
  testMethod(testComplementaryEraseSubsetsOfTwoSolversAreDiscarded);
  testMethod(testEraseNeedsUnanimousSolverConsent);
  testMethod(testReportTracksEveryStageOfARequest);
  testMethod(testTheDeltaExchangeTracksEveryRanksWishlist);
}

/**
 * The exchange ships deltas against the list each rank shipped last, so every rank derives
 * the same committed set while the wishlist grows, shrinks, empties and refills.
 */
void exahype2::tests::RefinementControlServiceTest::testTheDeltaExchangeTracksEveryRanksWishlist() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();
  service.setDomain(uniformVector(0.0), uniformVector(TestDomainWidth));

  const std::vector<std::vector<int>> wishlistPerStep{{70, 71, 72}, {71, 72}, {71, 72, 73, 74}, {72}, {}, {70, 72}};
  for (const auto& wishlist : wishlistPerStep) {
    exahype2::RefinementControl control;
    for (const int cell : wishlist) {
      addRefineCommandOnRank(control, uniformVector(static_cast<double>(cell)), cell);
    }
    commitCriterionStep(control);
    validateTrue(service.committedEventsAgreeAcrossRanks());

    const auto events = service.getGridControlEvents();
    validateEquals(events.size(), wishlist.size());
    for (const int cell : wishlist) {
      const auto expectedLatticeIndex = tarch::la::Vector<DIMENSIONS, int>(cell);
      int        numberOfMatches      = 0;
      for (const auto& event : events) {
        if (event.getRefinementControl() == peano4::grid::GridControlEvent::RefinementControl::Refine
            and event.level == LeafLevel and event.latticeIndex == expectedLatticeIndex) {
          numberOfMatches++;
        }
      }
      validateEqualsWithParams1(numberOfMatches, 1, cell);
    }
  }

  drainRefinementControlService();
}

void exahype2::tests::RefinementControlServiceTest::testEraseOnlyBatchIsCommitted() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();
  service.setDomain(uniformVector(0.0), uniformVector(TestDomainWidth));

  exahype2::RefinementControl control;
  addEraseSiblingGroup(control, parentCellOffset(14), -1);
  commitCriterionStep(control);

  const auto events = service.getGridControlEvents();
  validateEquals(events.size(), 1U);
  validateTrue(isParentEraseEvent(events[0], 14));

  drainRefinementControlService();
}

void exahype2::tests::RefinementControlServiceTest::testRefineCommandNamesTheRequestingCell() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();
  service.setDomain(uniformVector(0.0), uniformVector(TestDomainWidth));

  exahype2::RefinementControl control;
  addRefineCommand(control, uniformVector(1.0));
  commitCriterionStep(control);

  const auto events = service.getGridControlEvents();
  validateEquals(events.size(), 1U);
  validateTrue(events[0].getRefinementControl() == peano4::grid::GridControlEvent::RefinementControl::Refine);
  validateEquals(events[0].level, LeafLevel);
  const auto expectedLatticeIndex = tarch::la::Vector<DIMENSIONS, int>(1);
  validateTrue(events[0].latticeIndex == expectedLatticeIndex);

  drainRefinementControlService();
}

/**
 * Naming a cell inverts the chain of divisions and additions the traversal walks, so this
 * checks every cell of every level against the states the traversal itself produces.
 */
void exahype2::tests::RefinementControlServiceTest::testEveryCellIsNamedExactlyOnAnyDomain() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();

  for (int domainIndex = 0; domainIndex < numberOfTestDomains(); domainIndex++) {
    const auto domainOffset = testDomainOffset(domainIndex);
    const auto domainWidth  = testDomainWidth(domainIndex);
    service.setDomain(domainOffset, domainWidth);

    peano4::grid::AutomatonState rootState;
    rootState.level = 0;
    rootState.x     = domainOffset;
    rootState.h     = domainWidth;

    // The levels ascend so that no two batches in a row carry the same events, which the
    // service drops as already realised.
    for (int level = 1; level <= DeepestTestLevel; level++) {
      const int cellsPerAxisOnLevel = peano4::topology::cellsPerAxis(level);

      exahype2::RefinementControl                 control;
      std::vector<peano4::grid::GridControlEvent> expectedEvents;
      for (int index = 0; index < cellsPerAxisOnLevel; index++) {
        const auto latticeIndex = sweepPosition(index, cellsPerAxisOnLevel);

        peano4::grid::GridControlEvent expectedEvent;
        expectedEvent.setRefinementControl(peano4::grid::GridControlEvent::RefinementControl::Refine);
        expectedEvent.level        = level;
        expectedEvent.latticeIndex = latticeIndex;
        expectedEvents.push_back(expectedEvent);

        // Spreading the sweep over the ranks makes the exchange reassemble it.
        if (index % tarch::mpi::Rank::getInstance().getNumberOfRanks() == tarch::mpi::Rank::getInstance().getRank()) {
          const auto cellState = descendToCell(rootState, level, latticeIndex);
          control.addCommand(cellState.x + 0.5 * cellState.h, cellState.h, exahype2::RefinementCommand::Refine);
        }
      }
      commitCriterionStep(control);

      std::sort(expectedEvents.begin(), expectedEvents.end());
      const auto events = service.getGridControlEvents();
      validateEqualsWithParams3(events.size(), expectedEvents.size(), domainIndex, level, domainOffset);
      for (std::size_t eventIndex = 0; eventIndex < std::min(events.size(), expectedEvents.size()); eventIndex++) {
        validateWithParams4(
          events[eventIndex] == expectedEvents[eventIndex],
          domainIndex,
          level,
          events[eventIndex],
          expectedEvents[eventIndex]
        );
      }

      drainRefinementControlService();
    }
  }

  service.setDomain(uniformVector(0.0), uniformVector(TestDomainWidth));
}

void exahype2::tests::RefinementControlServiceTest::testCompleteEraseSiblingGroupBecomesParentEvent() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();
  service.setDomain(uniformVector(0.0), uniformVector(TestDomainWidth));

  // The refine command keeps the committed set distinct from every set realised before, so
  // the commit is not suppressed.
  exahype2::RefinementControl control;
  addEraseSiblingGroup(control, uniformVector(0.0), -1);
  addRefineCommand(control, uniformVector(50.0));
  commitCriterionStep(control);

  const auto events = service.getGridControlEvents();
  validateEquals(events.size(), 2U);
  validateTrue(isParentEraseEvent(events[0], 0) or isParentEraseEvent(events[1], 0));

  drainRefinementControlService();
}

void exahype2::tests::RefinementControlServiceTest::testEraseSiblingGroupIsAnchoredAtDomainOffset() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();
  // The parent grid starts at the domain offset, so a group anchored there is complete even
  // though none of its coordinates is a multiple of the parent size.
  const auto domainOffset = uniformVector(0.5);
  service.setDomain(domainOffset, uniformVector(TestDomainWidth));

  exahype2::RefinementControl control;
  addEraseSiblingGroup(control, domainOffset, -1);
  // The cell a command names is counted from the domain offset, so a command far from the
  // group has to be placed relative to that offset as well.
  addRefineCommand(control, domainOffset + uniformVector(53.0));
  commitCriterionStep(control);

  const auto events = service.getGridControlEvents();
  validateEquals(events.size(), 2U);
  validateTrue(isParentEraseEvent(events[0], 0) or isParentEraseEvent(events[1], 0));

  drainRefinementControlService();
  service.setDomain(uniformVector(0.0), uniformVector(TestDomainWidth));
}

// A command names its cell against its own axis's component of the domain offset, and the
// offsets here differ by more than half a cell from axis to axis.
void exahype2::tests::RefinementControlServiceTest::testEraseSiblingGroupIsAnchoredPerAxis() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();

  tarch::la::Vector<DIMENSIONS, double> domainOffset;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    domainOffset(axis) = 0.5 + 0.75 * static_cast<double>(axis);
  }
  service.setDomain(domainOffset, uniformVector(TestDomainWidth));

  exahype2::RefinementControl control;
  addEraseSiblingGroup(control, domainOffset, -1);
  // This position keeps the committed set distinct from the preceding test's, which the
  // service drops as already realised.
  addRefineCommand(control, domainOffset + uniformVector(47.0));
  commitCriterionStep(control);

  const auto events = service.getGridControlEvents();
  validateEquals(events.size(), 2U);
  validateTrue(isParentEraseEvent(events[0], 0) or isParentEraseEvent(events[1], 0));

  drainRefinementControlService();
  service.setDomain(uniformVector(0.0), uniformVector(TestDomainWidth));
}

void exahype2::tests::RefinementControlServiceTest::testIncompleteEraseSiblingGroupIsDiscarded() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();
  service.setDomain(uniformVector(0.0), uniformVector(TestDomainWidth));

  // The refine command proves the batch is processed.
  exahype2::RefinementControl control;
  addEraseSiblingGroup(control, uniformVector(0.0), ChildrenPerCell / 2);
  addRefineCommand(control, uniformVector(56.0));
  commitCriterionStep(control);

  validateEquals(service.getGridControlEvents().size(), 1U);
  validateTrue(
    service.getGridControlEvents()[0].getRefinementControl()
    == peano4::grid::GridControlEvent::RefinementControl::Refine
  );

  drainRefinementControlService();
}

void exahype2::tests::RefinementControlServiceTest::testShiftedEraseSiblingGroupIsDiscarded() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();
  service.setDomain(uniformVector(0.0), uniformVector(TestDomainWidth));

  // Neither parent receives a complete sibling group from the shifted children.
  exahype2::RefinementControl           control;
  tarch::la::Vector<DIMENSIONS, double> shiftedOrigin = uniformVector(0.0);
  shiftedOrigin(0)                                    = 1.0;
  addEraseSiblingGroup(control, shiftedOrigin, -1);
  addRefineCommand(control, uniformVector(59.0));
  commitCriterionStep(control);

  validateEquals(service.getGridControlEvents().size(), 1U);
  validateTrue(
    service.getGridControlEvents()[0].getRefinementControl()
    == peano4::grid::GridControlEvent::RefinementControl::Refine
  );

  drainRefinementControlService();
}

void exahype2::tests::RefinementControlServiceTest::testCommittedEventIsConsumedByOneTraversal() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();
  service.setDomain(uniformVector(0.0), uniformVector(TestDomainWidth));

  // The service remembers the last realised set across tests, so every test works on a cell
  // of its own and none of them is suppressed by a predecessor.
  exahype2::RefinementControl control;
  addRefineCommand(control, uniformVector(10.0));
  commitCriterionStep(control);
  validateEquals(service.getGridControlEvents().size(), 1U);

  service.finishDynamicMeshRefinementStep();
  validateTrue(service.getGridControlEvents().empty());

  // The realised set is what the suppression compares against, so the same request asks for
  // a mesh that already stands.
  exahype2::RefinementControl repeatedControl;
  addRefineCommand(repeatedControl, uniformVector(10.0));
  service.merge(repeatedControl);
  service.finishDynamicMeshRefinementStep();
  validateTrue(service.getGridControlEvents().empty());

  drainRefinementControlService();
}

void exahype2::tests::RefinementControlServiceTest::testEachBatchCarriesOnlyItsOwnRequests() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();
  service.setDomain(uniformVector(0.0), uniformVector(TestDomainWidth));

  exahype2::RefinementControl controlA;
  addRefineCommand(controlA, uniformVector(30.0));
  commitCriterionStep(controlA);
  validateEquals(service.getGridControlEvents().size(), 1U);

  // The next batch collects request B while the traversal realises A, so it carries B alone.
  exahype2::RefinementControl controlB;
  addRefineCommand(controlB, uniformVector(36.0));
  service.merge(controlB);
  service.finishDynamicMeshRefinementStep();

  const auto events = service.getGridControlEvents();
  validateEquals(events.size(), 1U);
  const auto expectedLatticeIndex = tarch::la::Vector<DIMENSIONS, int>(36);
  validateTrue(events[0].latticeIndex == expectedLatticeIndex);

  service.finishDynamicMeshRefinementStep();
  validateTrue(service.getGridControlEvents().empty());

  drainRefinementControlService();
}

/**
 * A batch carries the votes raised while it was collected, so a group assembles only from
 * children that vote in that batch.
 */
void exahype2::tests::RefinementControlServiceTest::testSiblingGroupNeedsEveryChildInOneBatch() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();
  service.setDomain(uniformVector(0.0), uniformVector(TestDomainWidth));

  // The refine command keeps the batch from being suppressed.
  exahype2::RefinementControl incompleteControl;
  addEraseSiblingGroupChildRange(incompleteControl, parentCellOffset(13), 0, ChildrenPerCell - 1, -1);
  addRefineCommand(incompleteControl, uniformVector(62.0));
  commitCriterionStep(incompleteControl);
  validateEquals(service.getGridControlEvents().size(), 1U);
  validateTrue(
    service.getGridControlEvents()[0].getRefinementControl()
    == peano4::grid::GridControlEvent::RefinementControl::Refine
  );

  // The missing child votes on its own in the next batch, which leaves that batch's group
  // incomplete as well.
  exahype2::RefinementControl completingControl;
  addEraseSiblingGroupChildRange(completingControl, parentCellOffset(13), ChildrenPerCell - 1, ChildrenPerCell, -1);
  service.merge(completingControl);
  service.finishDynamicMeshRefinementStep();
  validateTrue(service.getGridControlEvents().empty());

  exahype2::RefinementControl completeControl;
  addEraseSiblingGroup(completeControl, parentCellOffset(13), -1);
  service.merge(completeControl);
  service.finishDynamicMeshRefinementStep();

  const auto events = service.getGridControlEvents();
  validateEquals(events.size(), 1U);
  validateTrue(isParentEraseEvent(events[0], 13));

  drainRefinementControlService();
}

void exahype2::tests::RefinementControlServiceTest::testUnchangedEventSetIsSuppressed() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();
  service.setDomain(uniformVector(0.0), uniformVector(TestDomainWidth));

  {
    exahype2::RefinementControl control;
    addRefineCommand(control, uniformVector(20.0));
    commitCriterionStep(control);
    validateEquals(service.getGridControlEvents().size(), 1U);
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
    validateEquals(service.getGridControlEvents().size(), 1U);
  }

  drainRefinementControlService();
}

/**
 * A step whose merged contributions repeat the preceding step's reuses that step's
 * committed set.
 */
void exahype2::tests::RefinementControlServiceTest::testRepeatedContributionsReuseTheCommittedSet() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();
  service.setDomain(uniformVector(0.0), uniformVector(TestDomainWidth));

  constexpr int ParentLatticePosition = 10;
  const auto    parentOffset          = parentCellOffset(ParentLatticePosition);
  for (int repetition = 0; repetition < 2; repetition++) {
    exahype2::RefinementControl control;
    addEraseSiblingGroup(control, parentOffset, -1);
    service.merge(control);
    service.finishStep();
    validateEquals(service.getGridControlEvents().size(), 1U);
    validateTrue(isParentEraseEvent(service.getGridControlEvents().front(), ParentLatticePosition));
  }

  {
    exahype2::RefinementControl firstHalf;
    exahype2::RefinementControl secondHalf;
    addEraseSiblingGroupChildRange(firstHalf, parentOffset, 0, ChildrenPerCell / 2, -1);
    addEraseSiblingGroupChildRange(secondHalf, parentOffset, ChildrenPerCell / 2, ChildrenPerCell, -1);
    service.merge(firstHalf);
    service.merge(secondHalf);
    service.finishStep();
    validateEquals(service.getGridControlEvents().size(), 1U);
    validateTrue(isParentEraseEvent(service.getGridControlEvents().front(), ParentLatticePosition));
  }

  {
    exahype2::RefinementControl control;
    addEraseSiblingGroup(control, parentOffset, ChildrenPerCell / 2);
    service.merge(control);
    service.finishStep();
    validateTrue(service.getGridControlEvents().empty());
  }

  drainRefinementControlService();
}

/**
 * A committed set recorded as realised without a sweep leaves the committed
 * set empty and the topology unchanged; the same requests raised again are
 * suppressed while a different cell survives.
 */
void exahype2::tests::RefinementControlServiceTest::testARealisedSetWithoutSweepSuppressesItsRepetition() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();
  service.setDomain(uniformVector(0.0), uniformVector(TestDomainWidth));
  service.finishRefinementCriterionStep();

  {
    exahype2::RefinementControl control;
    addRefineCommand(control, uniformVector(26.0));
    commitCriterionStep(control);
    validateEquals(service.getGridControlEvents().size(), 1U);
  }
  service.realiseCommittedEventsWithoutSweep();
  validateTrue(service.getGridControlEvents().empty());
  validateFalse(service.hasRealisedTopologyChangesSinceLastTimeStep());

  {
    exahype2::RefinementControl control;
    addRefineCommand(control, uniformVector(26.0));
    commitCriterionStep(control);
    validateTrue(service.getGridControlEvents().empty());
    validateFalse(service.hasRealisedTopologyChangesSinceLastTimeStep());
  }
  {
    exahype2::RefinementControl control;
    addRefineCommand(control, uniformVector(29.0));
    commitCriterionStep(control);
    validateEquals(service.getGridControlEvents().size(), 1U);
  }

  drainRefinementControlService();
}

/**
 * A rank that raises nothing after a set realised without a sweep ships the
 * delta against the list it shipped last, so that list leaves with the delta
 * and a request the last rank raises afterwards arrives alone.
 */
void exahype2::tests::RefinementControlServiceTest::testARealisedSetWithoutSweepKeepsTheShippedLists() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();
  service.setDomain(uniformVector(0.0), uniformVector(TestDomainWidth));
  const int numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();

  // Every rank raises one cell of its own, so every rank ships a list with one entry.
  {
    exahype2::RefinementControl control;
    const auto                  cell = 40.0 + tarch::mpi::Rank::getInstance().getRank();
    control.addCommand(
      uniformVector(cell) + 0.5 * uniformVector(1.0),
      uniformVector(1.0),
      exahype2::RefinementCommand::Refine
    );
    service.merge(control);
    service.finishDynamicMeshRefinementStep();
    validateEquals(service.getGridControlEvents().size(), static_cast<std::size_t>(numberOfRanks));
  }
  service.realiseCommittedEventsWithoutSweep();
  validateTrue(service.getGridControlEvents().empty());

  {
    exahype2::RefinementControl control;
    addRefineCommandOnRank(control, uniformVector(60.0), numberOfRanks - 1);
    service.merge(control);
    service.finishDynamicMeshRefinementStep();
    validateTrue(service.committedEventsAgreeAcrossRanks());
    const auto                               events = service.getGridControlEvents();
    const tarch::la::Vector<DIMENSIONS, int> expectedLatticeIndex(60);
    validateEquals(events.size(), 1U);
    validateEquals(events.front().level, LeafLevel);
    validateTrue(events.front().latticeIndex == expectedLatticeIndex);
  }

  drainRefinementControlService();
}

void exahype2::tests::RefinementControlServiceTest::testComplementaryEraseSubsetsOfTwoSolversAreDiscarded() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();
  service.setDomain(uniformVector(0.0), uniformVector(TestDomainWidth));
  service.setNumberOfSolvers(2);

  // Two solvers split one sibling group between them, so neither solver consents to the
  // complete group.
  exahype2::RefinementControl controlOfFirstSolver(0);
  exahype2::RefinementControl controlOfSecondSolver(1);
  addEraseSiblingGroupChildRange(controlOfFirstSolver, parentCellOffset(10), 0, ChildrenPerCell / 2, -1);
  addEraseSiblingGroupChildRange(controlOfSecondSolver, parentCellOffset(10), ChildrenPerCell / 2, ChildrenPerCell, -1);
  addRefineCommand(controlOfFirstSolver, uniformVector(65.0));
  service.merge(controlOfFirstSolver);
  commitCriterionStep(controlOfSecondSolver);

  validateEquals(service.getGridControlEvents().size(), 1U);
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
  service.setDomain(uniformVector(0.0), uniformVector(TestDomainWidth));
  service.setNumberOfSolvers(2);

  // The refine command keeps the committed set distinct from every set realised before.
  {
    exahype2::RefinementControl controlOfFirstSolver(0);
    exahype2::RefinementControl controlOfSecondSolver(1);
    addEraseSiblingGroup(controlOfFirstSolver, parentCellOffset(11), -1);
    addEraseSiblingGroup(controlOfSecondSolver, parentCellOffset(11), -1);
    addRefineCommand(controlOfFirstSolver, uniformVector(68.0));
    service.merge(controlOfFirstSolver);
    commitCriterionStep(controlOfSecondSolver);

    const auto events = service.getGridControlEvents();
    validateEquals(events.size(), 2U);
    validateTrue(isParentEraseEvent(events[0], 11) or isParentEraseEvent(events[1], 11));
  }
  drainRefinementControlService();

  // A solver that submits no erase request vetoes the other solver's complete group.
  {
    exahype2::RefinementControl controlOfFirstSolver(0);
    addEraseSiblingGroup(controlOfFirstSolver, parentCellOffset(12), -1);
    addRefineCommand(controlOfFirstSolver, uniformVector(71.0));
    commitCriterionStep(controlOfFirstSolver);

    validateEquals(service.getGridControlEvents().size(), 1U);
    validateTrue(
      service.getGridControlEvents()[0].getRefinementControl()
      == peano4::grid::GridControlEvent::RefinementControl::Refine
    );
  }

  drainRefinementControlService();
  service.setNumberOfSolvers(1);
}

// The offsets belong to this test alone, so no committed set matches one realised earlier.
void exahype2::tests::RefinementControlServiceTest::testReportTracksEveryStageOfARequest() {
  drainRefinementControlService();
  auto& service = exahype2::RefinementControlService::getInstance();
  service.setDomain(uniformVector(0.0), uniformVector(TestDomainWidth));
  service.setNumberOfSolvers(1);

  // A drain leaves the realised set of whichever request it drained last, so one full cycle
  // runs first to make that set this test's own.
  {
    exahype2::RefinementControl control;
    addRefineCommand(control, uniformVector(74.0));
    commitCriterionStep(control);
    service.finishDynamicMeshRefinementStep();
  }
  validateEquals(service.toString(), describeServiceState(0, 0, 1, true, 1));

  service.finishRefinementCriterionStep();
  validateEquals(service.toString(), describeServiceState(0, 0, 1, false, 1));

  exahype2::RefinementControl control;
  addRefineCommand(control, uniformVector(77.0));
  service.merge(control);
  validateEquals(service.toString(), describeServiceState(numberOfLocalRefineRequests(), 0, 1, false, 1));

  service.finishRefinementCriterionStep();
  validateEquals(service.toString(), describeServiceState(0, 1, 1, false, 1));

  service.finishDynamicMeshRefinementStep();
  validateEquals(service.toString(), describeServiceState(0, 0, 1, true, 1));

  drainRefinementControlService();
}
