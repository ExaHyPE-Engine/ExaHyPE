// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SpacetreeSetTest.h"

#include "peano4/grid/GridControlEvent.h"
#include "peano4/grid/GridStatistics.h"
#include "peano4/grid/GridTraversalObserver.h"
#include "peano4/grid/Spacetree.h"
#include "peano4/parallel/Node.h"
#include "peano4/parallel/SpacetreeSet.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/mpi.h"

namespace {
  constexpr double DomainOffset = 10.0;
  constexpr double DomainSize   = 1.0;

  /**
   * Refining the whole box to this resolution yields a uniform level-2 grid:
   * the level-1 cells (h = 1/3) are coarser than the target, the level-2
   * cells (h = 1/9) are not.
   */
  constexpr double UniformTargetH = 0.15;

  /**
   * Refining the corner box to this resolution adds one more level there: the
   * level-2 cells are coarser than the target, the level-3 cells (h = 1/27)
   * are not.
   */
  constexpr double CornerTargetH = 0.05;
  constexpr double CornerExtent  = 0.12;

  class ScenarioObserver final : public peano4::grid::GridTraversalObserver {
  public:
    std::vector<peano4::grid::GridControlEvent> events;

    void enterNode(const peano4::grid::GridTraversalEvent&) override {}
    void leaveNode(const peano4::grid::GridTraversalEvent&) override {}

    peano4::grid::GridTraversalObserver* clone(int) override { return new ScenarioObserver(*this); }

    std::vector<peano4::grid::GridControlEvent> getGridControlEvents() const override { return events; }

    void beginTraversal(
      const tarch::la::Vector<DIMENSIONS, double>&,
      const tarch::la::Vector<DIMENSIONS, double>&
    ) override {}
    void
    endTraversal(const tarch::la::Vector<DIMENSIONS, double>&, const tarch::la::Vector<DIMENSIONS, double>&) override {}
  };

  peano4::grid::GridControlEvent createRefineEvent(const double offset, const double extent, const double targetH) {
    peano4::grid::GridControlEvent result;
    result.setRefinementControl(peano4::grid::GridControlEvent::RefinementControl::Refine);
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      result.offset[axis]  = offset;
      result.width[axis]   = extent;
      result.targetH[axis] = targetH;
    }
    return result;
  }

  peano4::grid::GridControlEvent createEraseEvent(const double offset, const double extent, const double targetH) {
    peano4::grid::GridControlEvent result;
    result.setRefinementControl(peano4::grid::GridControlEvent::RefinementControl::Erase);
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      result.offset[axis]  = offset;
      result.width[axis]   = extent;
      result.targetH[axis] = targetH;
    }
    return result;
  }

  void traverseSweeps(
    peano4::parallel::SpacetreeSet&      spacetreeSet,
    peano4::grid::GridTraversalObserver& observer,
    const int                            sweeps
  ) {
    for (int sweep = 0; sweep < sweeps; sweep++) {
      spacetreeSet.traverse(observer);
    }
  }
} // namespace

peano4::parallel::tests::SpacetreeSetTest::SpacetreeSetTest() :
  TestCase("peano4::parallel::tests::SpacetreeSetTest") {}

void peano4::parallel::tests::SpacetreeSetTest::run() {
  testMethod(testWorkerCopiesRequestedMasterState);
  testMethod(testRemoteSplitReservesIdsAsOneBatch);
  testMethod(testDistributedSplitLifecycle);
  testMethod(testCleanupRemovesConsecutiveTrees);
  testMethod(testAggregateStatisticsIncludesSettlingTrees);
  testMethod(testRegularGridLoadBalancingMatchesNonPeriodicRun);
  testMethod(testCornerRefinementMirrorsAcrossPeriodicBoundaries);
  testMethod(testMultiTreePeriodicGridMatchesSingleTree);
  testMethod(testEraseAtPeriodicBoundaryRestoresRegularGrid);
}

/** Exercise the variable-length remote reservation response without traversing a synthetic split. */
void peano4::parallel::tests::SpacetreeSetTest::testRemoteSplitReservesIdsAsOneBatch() {
#ifdef PEANO_WITH_MPI
  constexpr int WorkerRank       = 1;
  constexpr int NumberOfNewTrees = 3;
  auto&         rank             = tarch::mpi::Rank::getInstance();
  if (rank.getNumberOfRanks() <= WorkerRank) {
    return;
  }

  auto&     spacetreeSet                   = SpacetreeSet::getInstance();
  auto&     parallelNode                   = Node::getInstance();
  const int localRank                      = rank.getRank();
  const int initialNumberOfRegisteredTrees = parallelNode.getNumberOfRegisteredTrees();

  int                               sourceTreeId = -1;
  std::array<int, NumberOfNewTrees> workerTreeIds{};
  if (rank.isGlobalMaster()) {
    sourceTreeId = parallelNode.reserveId(localRank, 0);
    spacetreeSet._spacetrees.emplace_back(
      sourceTreeId,
      tarch::la::Vector<DIMENSIONS, double>(DomainOffset + 8.0),
      tarch::la::Vector<DIMENSIONS, double>(DomainSize),
      spacetreeSet._periodicBC
    );
    spacetreeSet.registerLocalSpacetree(spacetreeSet._spacetrees.back());
    auto& sourceTree                                   = spacetreeSet._spacetrees.back();
    sourceTree._spacetreeState                         = peano4::grid::SpacetreeState::Running;
    sourceTree._statistics.numberOfLocalUnrefinedCells = NumberOfNewTrees + 1;

    const std::vector<SpacetreeSet::SplitRequest> requests(
      NumberOfNewTrees,
      SpacetreeSet::SplitRequest{
        peano4::grid::SplitInstruction{1, peano4::grid::SplitInstruction::Mode::BottomUp},
        WorkerRank
      }
    );
    validate(spacetreeSet.split(sourceTreeId, requests));
    validateEquals(static_cast<int>(sourceTree._splitTriggered.size()), NumberOfNewTrees);

    int worker = 0;
    for (const auto& [workerTreeId, instruction] : sourceTree._splitTriggered) {
      validateEquals(parallelNode.getRank(workerTreeId), WorkerRank);
      validateEquals(instruction.numberOfFineGridCells, 1);
      workerTreeIds[worker++] = workerTreeId;
    }
  } else if (localRank == WorkerRank) {
    while (parallelNode.getNumberOfRegisteredTrees() < initialNumberOfRegisteredTrees + NumberOfNewTrees) {
      spacetreeSet.receiveDanglingMessages();
    }
  }

  MPI_Bcast(workerTreeIds.data(), NumberOfNewTrees, MPI_INT, 0, rank.getCommunicator());
  if (localRank == WorkerRank) {
    while (not spacetreeSet._pendingTreeIdResponses.empty()) {
      spacetreeSet.receiveDanglingMessages();
    }
    for (const int workerTreeId : workerTreeIds) {
      parallelNode.deregisterId(workerTreeId);
    }
  }

  if (rank.isGlobalMaster()) {
    spacetreeSet.unregisterLocalSpacetree(sourceTreeId);
    parallelNode.deregisterId(sourceTreeId);
    for (auto tree = spacetreeSet._spacetrees.begin(); tree != spacetreeSet._spacetrees.end(); ++tree) {
      if (tree->getId() == sourceTreeId) {
        spacetreeSet._spacetrees.erase(tree);
        break;
      }
    }
  }
  tarch::mpi::barrier(rank.getCommunicator());
#endif
}

/** Exercise one exact split through the complete cross-rank traversal protocol. */
void peano4::parallel::tests::SpacetreeSetTest::testDistributedSplitLifecycle() {
#ifdef PEANO_WITH_MPI
  constexpr int WorkerRank = 1;
  auto&         rank       = tarch::mpi::Rank::getInstance();
  if (rank.getNumberOfRanks() <= WorkerRank) {
    return;
  }

  auto&      spacetreeSet           = SpacetreeSet::getInstance();
  auto&      parallelNode           = Node::getInstance();
  const int  localRank              = rank.getRank();
  const int  initialLocalTreeCount  = static_cast<int>(spacetreeSet._spacetrees.size());
  const int  initialRegisteredTrees = parallelNode.getNumberOfRegisteredTrees();
  const auto periodicBoundaryBackup = spacetreeSet._periodicBC;
  spacetreeSet._periodicBC.reset();

  constexpr double DistributedDomainOffset = DomainOffset + 12.0;
  int              rootTreeId              = -1;
  if (rank.isGlobalMaster()) {
    rootTreeId = parallelNode.reserveId(localRank, 0);
    spacetreeSet._spacetrees.emplace_back(
      rootTreeId,
      tarch::la::Vector<DIMENSIONS, double>(DistributedDomainOffset),
      tarch::la::Vector<DIMENSIONS, double>(DomainSize),
      std::bitset<DIMENSIONS>{}
    );
    spacetreeSet.registerLocalSpacetree(spacetreeSet._spacetrees.back());
  }
  MPI_Bcast(&rootTreeId, 1, MPI_INT, 0, rank.getCommunicator());

  ScenarioObserver observer;
  observer.events = {createRefineEvent(DistributedDomainOffset, DomainSize, UniformTargetH)};
  traverseSweeps(spacetreeSet, observer, 6);

  constexpr int ExpectedNumberOfLeaves = THREE_POWER_D * THREE_POWER_D;
  if (rank.isGlobalMaster()) {
    validateEquals(spacetreeSet.getGridStatistics(rootTreeId).numberOfLocalUnrefinedCells, ExpectedNumberOfLeaves);
  }

  int workerTreeId = -1;
  if (rank.isGlobalMaster()) {
    const auto splitRegistered = spacetreeSet.split(
      rootTreeId,
      peano4::grid::SplitInstruction{ExpectedNumberOfLeaves / 2, peano4::grid::SplitInstruction::Mode::BottomUp},
      WorkerRank
    );
    validate(splitRegistered);
    validateEquals(static_cast<int>(spacetreeSet.getSpacetree(rootTreeId)._splitTriggered.size()), 1);
    workerTreeId = spacetreeSet.getSpacetree(rootTreeId)._splitTriggered.begin()->first;
  } else if (localRank == WorkerRank) {
    while (parallelNode.getNumberOfRegisteredTrees() == initialRegisteredTrees) {
      spacetreeSet.receiveDanglingMessages();
    }
  }
  MPI_Bcast(&workerTreeId, 1, MPI_INT, 0, rank.getCommunicator());
  validateEquals(parallelNode.getRank(workerTreeId), WorkerRank);

  traverseSweeps(spacetreeSet, observer, 6);

  int localFamilyTrees    = 0;
  int localFamilyLeaves   = 0;
  int localUnsettledTrees = 0;
  for (const int treeId : {rootTreeId, workerTreeId}) {
    if (not spacetreeSet.isLocalSpacetree(treeId)) {
      continue;
    }
    localFamilyTrees++;
    const auto& tree       = spacetreeSet.getSpacetree(treeId);
    const auto& statistics = tree.getGridStatistics();
    localFamilyLeaves += statistics.numberOfLocalUnrefinedCells;
    if (tree._spacetreeState != peano4::grid::SpacetreeState::Running or not tree._splitTriggered.empty()
        or not tree._splitting.empty() or not tree._hasSplit.empty() or statistics.numberOfStationarySweeps < 0) {
      localUnsettledTrees++;
    }
  }

  int globalFamilyTrees    = 0;
  int globalFamilyLeaves   = 0;
  int globalUnsettledTrees = 0;
  tarch::mpi::allreduce(&localFamilyTrees, &globalFamilyTrees, 1, MPI_INT, MPI_SUM, rank.getCommunicator());
  tarch::mpi::allreduce(&localFamilyLeaves, &globalFamilyLeaves, 1, MPI_INT, MPI_SUM, rank.getCommunicator());
  tarch::mpi::allreduce(&localUnsettledTrees, &globalUnsettledTrees, 1, MPI_INT, MPI_SUM, rank.getCommunicator());

  validateEquals(globalFamilyTrees, 2);
  validateEquals(globalFamilyLeaves, ExpectedNumberOfLeaves);
  validateEquals(globalUnsettledTrees, 0);
  validateEquals(spacetreeSet.isLocalSpacetree(rootTreeId), rank.isGlobalMaster());
  const bool ownsWorkerTree = localRank == WorkerRank;
  validateEquals(spacetreeSet.isLocalSpacetree(workerTreeId), ownsWorkerTree);
  if (localRank == WorkerRank) {
    const auto& worker = spacetreeSet.getSpacetree(workerTreeId);
    validateEquals(worker.getMasterId(), rootTreeId);
  }

  for (auto tree = spacetreeSet._spacetrees.begin(); tree != spacetreeSet._spacetrees.end();) {
    if (tree->getId() == rootTreeId or tree->getId() == workerTreeId) {
      spacetreeSet.unregisterLocalSpacetree(tree->getId());
      parallelNode.deregisterId(tree->getId());
      tree = spacetreeSet._spacetrees.erase(tree);
    } else {
      ++tree;
    }
  }
  spacetreeSet._periodicBC = periodicBoundaryBackup;
  tarch::mpi::barrier(rank.getCommunicator());
  validateEquals(static_cast<int>(spacetreeSet._spacetrees.size()), initialLocalTreeCount);
#endif
}

void peano4::parallel::tests::SpacetreeSetTest::testWorkerCopiesRequestedMasterState() {
  auto&     spacetreeSet         = SpacetreeSet::getInstance();
  auto&     parallelNode         = Node::getInstance();
  const int rank                 = tarch::mpi::Rank::getInstance().getRank();
  const int initialNumberOfTrees = static_cast<int>(spacetreeSet._spacetrees.size());

  constexpr double RequestedMasterOffset = DomainOffset + 4.0;
  constexpr double RequestedMasterSize   = 2.0;

  int requestedMasterId = -1;
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    requestedMasterId = parallelNode.reserveId(rank, 0);
    spacetreeSet._spacetrees.emplace_back(
      requestedMasterId,
      tarch::la::Vector<DIMENSIONS, double>(RequestedMasterOffset),
      tarch::la::Vector<DIMENSIONS, double>(RequestedMasterSize),
      spacetreeSet._periodicBC
    );
    spacetreeSet.registerLocalSpacetree(spacetreeSet._spacetrees.back());
  }

  int workerId = -1;
#ifdef PEANO_WITH_MPI
  constexpr int WorkerRank = 1;
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > WorkerRank) {
    const auto communicator = tarch::mpi::Rank::getInstance().getCommunicator();
    MPI_Bcast(&requestedMasterId, 1, MPI_INT, 0, communicator);
    if (rank == WorkerRank) {
      workerId = parallelNode.reserveId(WorkerRank, requestedMasterId);
    }
    MPI_Bcast(&workerId, 1, MPI_INT, WorkerRank, communicator);

    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      spacetreeSet.addSpacetrees(requestedMasterId, {workerId});
    } else if (rank == WorkerRank) {
      while (not spacetreeSet.isLocalSpacetree(workerId)) {
        spacetreeSet.receiveDanglingMessages();
      }
    }
    tarch::mpi::barrier(communicator);
  } else
#endif
  {
    if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
      workerId = parallelNode.reserveId(rank, requestedMasterId);
      spacetreeSet.addSpacetrees(requestedMasterId, {workerId});
    }
  }

  if (spacetreeSet.isLocalSpacetree(workerId)) {
    const auto& worker = spacetreeSet.getSpacetree(workerId);
    validateEquals(worker._masterId, requestedMasterId);
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      validateEquals(worker._root.x[axis], RequestedMasterOffset);
      validateEquals(worker._root.h[axis], RequestedMasterSize);
    }
  }

  const std::set<int> testTreeIds{requestedMasterId, workerId};
  for (auto treeIterator = spacetreeSet._spacetrees.begin(); treeIterator != spacetreeSet._spacetrees.end();) {
    if (testTreeIds.count(treeIterator->getId()) > 0) {
      spacetreeSet.unregisterLocalSpacetree(treeIterator->getId());
      parallelNode.deregisterId(treeIterator->getId());
      treeIterator = spacetreeSet._spacetrees.erase(treeIterator);
    } else {
      ++treeIterator;
    }
  }
#ifdef PEANO_WITH_MPI
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > WorkerRank) {
    tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
  }
#endif
  validateEquals(static_cast<int>(spacetreeSet._spacetrees.size()), initialNumberOfTrees);
}

void peano4::parallel::tests::SpacetreeSetTest::testCleanupRemovesConsecutiveTrees() {
  if (not tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    return;
  }

  auto&     spacetreeSet         = SpacetreeSet::getInstance();
  auto&     parallelNode         = Node::getInstance();
  const int rank                 = tarch::mpi::Rank::getInstance().getRank();
  const int initialNumberOfTrees = static_cast<int>(spacetreeSet._spacetrees.size());

  const int firstJoinedTreeId = parallelNode.reserveId(rank, 0);
  spacetreeSet._spacetrees.emplace_back(
    firstJoinedTreeId,
    0,
    tarch::la::Vector<DIMENSIONS, double>(DomainOffset + 6.0),
    tarch::la::Vector<DIMENSIONS, double>(DomainSize),
    false,
    spacetreeSet._periodicBC
  );
  spacetreeSet.registerLocalSpacetree(spacetreeSet._spacetrees.back());
  spacetreeSet._spacetrees.back()._spacetreeState = peano4::grid::SpacetreeState::Joined;

  const int secondJoinedTreeId = parallelNode.reserveId(rank, 0);
  spacetreeSet._spacetrees.emplace_back(
    secondJoinedTreeId,
    0,
    tarch::la::Vector<DIMENSIONS, double>(DomainOffset + 7.0),
    tarch::la::Vector<DIMENSIONS, double>(DomainSize),
    false,
    spacetreeSet._periodicBC
  );
  spacetreeSet.registerLocalSpacetree(spacetreeSet._spacetrees.back());
  spacetreeSet._spacetrees.back()._spacetreeState = peano4::grid::SpacetreeState::Joined;

  ScenarioObserver observer;
  spacetreeSet.cleanUpTrees(observer);

  validate(not spacetreeSet.isLocalSpacetree(firstJoinedTreeId));
  validate(not spacetreeSet.isLocalSpacetree(secondJoinedTreeId));
  validateEquals(static_cast<int>(spacetreeSet._spacetrees.size()), initialNumberOfTrees);

  const std::set<int> joinedTreeIds{firstJoinedTreeId, secondJoinedTreeId};
  for (auto treeIterator = spacetreeSet._spacetrees.begin(); treeIterator != spacetreeSet._spacetrees.end();) {
    if (joinedTreeIds.count(treeIterator->getId()) > 0) {
      spacetreeSet.unregisterLocalSpacetree(treeIterator->getId());
      parallelNode.deregisterId(treeIterator->getId());
      treeIterator = spacetreeSet._spacetrees.erase(treeIterator);
    } else {
      ++treeIterator;
    }
  }
}

/** A pending split keeps the aggregate grid statistics unsettled. */
void peano4::parallel::tests::SpacetreeSetTest::testAggregateStatisticsIncludesSettlingTrees() {
  if (not tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    return;
  }

  auto&         spacetreeSet          = SpacetreeSet::getInstance();
  auto&         parallelNode          = Node::getInstance();
  const auto    statisticsBeforeSplit = spacetreeSet.getGridStatistics();
  const int     rank                  = tarch::mpi::Rank::getInstance().getRank();
  const int     settlingTreeId        = parallelNode.reserveId(rank, 0);
  constexpr int SettlingSweeps        = -17;
  constexpr int SettlingTreeLeaves    = 23;

  spacetreeSet._spacetrees.emplace_back(
    settlingTreeId,
    0,
    tarch::la::Vector<DIMENSIONS, double>(DomainOffset + 9.0),
    tarch::la::Vector<DIMENSIONS, double>(DomainSize),
    false,
    spacetreeSet._periodicBC
  );
  spacetreeSet.registerLocalSpacetree(spacetreeSet._spacetrees.back());
  auto& settlingTree                                   = spacetreeSet._spacetrees.back();
  settlingTree._statistics.numberOfStationarySweeps    = SettlingSweeps;
  settlingTree._statistics.numberOfLocalUnrefinedCells = SettlingTreeLeaves;
  settlingTree._spacetreeState                         = peano4::grid::SpacetreeState::Running;
  validate(spacetreeSet.maySplit(settlingTreeId));
  settlingTree._spacetreeState = peano4::grid::SpacetreeState::NewFromSplit;
  validate(not spacetreeSet.maySplit(settlingTreeId));

  const auto statisticsDuringSplit = spacetreeSet.getGridStatistics();
  validateEquals(
    statisticsDuringSplit.numberOfStationarySweeps,
    std::min(statisticsBeforeSplit.numberOfStationarySweeps, SettlingSweeps)
  );
  validateEquals(
    statisticsDuringSplit.numberOfLocalUnrefinedCells,
    statisticsBeforeSplit.numberOfLocalUnrefinedCells + SettlingTreeLeaves
  );

  spacetreeSet.unregisterLocalSpacetree(settlingTreeId);
  parallelNode.deregisterId(settlingTreeId);
  spacetreeSet._spacetrees.pop_back();
}

/**
 * Create a root tree over [10,11]^DIMENSIONS, build a level-2 grid, split it,
 * and optionally refine or erase a corner. Splits precede adaptive refinement
 * because hanging vertices can make an exact cut impossible.
 *
 * All ranks execute the same number of collective traversals; tree
 * creation, splits, and the returned statistics are confined to the global
 * master, so the test runs identically in serial and MPI configurations.
 */
peano4::parallel::tests::SpacetreeSetTest::ScenarioResult peano4::parallel::tests::SpacetreeSetTest::runScenario(
  const bool periodic,
  const int  targetTrees,
  const bool refineCorner,
  const bool eraseCornerAgain
) const {
  auto&      spacetreeSet   = SpacetreeSet::getInstance();
  auto&      parallelNode   = Node::getInstance();
  const int  rank           = tarch::mpi::Rank::getInstance().getRank();
  const bool isGlobalMaster = tarch::mpi::Rank::getInstance().isGlobalMaster();

  std::bitset<DIMENSIONS> periodicAxes;
  if (periodic) {
    periodicAxes.set();
  }

  // All ranks share routing state, so the scenario applies and restores the
  // periodic configuration collectively.
  const auto backupPeriodicBC = spacetreeSet._periodicBC;
  spacetreeSet._periodicBC    = periodicAxes;

  int rootId = -1;
  if (isGlobalMaster) {
    rootId = parallelNode.reserveId(rank, 0);
    spacetreeSet._spacetrees.emplace_back(
      rootId,
      tarch::la::Vector<DIMENSIONS, double>(DomainOffset),
      tarch::la::Vector<DIMENSIONS, double>(DomainSize),
      periodicAxes
    );
    spacetreeSet.registerLocalSpacetree(spacetreeSet._spacetrees.back());
  }

  ScenarioResult result;

  ScenarioObserver observer;
  observer.events = {createRefineEvent(DomainOffset, DomainSize, UniformTargetH)};
  traverseSweeps(spacetreeSet, observer, 8);

  if (targetTrees > 1 and isGlobalMaster) {
    const int numberOfUnrefinedCells = spacetreeSet.getGridStatistics(rootId).numberOfLocalUnrefinedCells;
    std::vector<peano4::parallel::SpacetreeSet::SplitRequest> requests;
    requests.reserve(targetTrees - 1);
    for (int splitIndex = 1; splitIndex < targetTrees; splitIndex++) {
      requests.push_back(
        {peano4::grid::SplitInstruction{
           numberOfUnrefinedCells / targetTrees,
           peano4::grid::SplitInstruction::Mode::BottomUp
         },
         rank}
      );
    }
    result.splitsSucceeded = spacetreeSet.split(rootId, requests);
  }
  traverseSweeps(spacetreeSet, observer, 6);

  if (refineCorner) {
    observer.events.push_back(createRefineEvent(DomainOffset, CornerExtent, CornerTargetH));
    traverseSweeps(spacetreeSet, observer, 8);
  }

  if (eraseCornerAgain) {
    observer.events = {
      createRefineEvent(DomainOffset, DomainSize, UniformTargetH),
      createEraseEvent(DomainOffset, DomainSize, UniformTargetH)
    };
    traverseSweeps(spacetreeSet, observer, 8);
  }

  std::set<int> treeFamily;
  if (isGlobalMaster) {
    treeFamily.insert(rootId);
    bool treeFamilyGrew = true;
    while (treeFamilyGrew) {
      treeFamilyGrew = false;
      for (const auto& tree : spacetreeSet._spacetrees) {
        if (treeFamily.count(tree.getId()) == 0 and treeFamily.count(tree.getMasterId()) > 0) {
          treeFamily.insert(tree.getId());
          treeFamilyGrew = true;
        }
      }
    }

    for (const int treeId : treeFamily) {
      if (not spacetreeSet.isLocalSpacetree(treeId)) {
        continue;
      }
      const auto& statistics = spacetreeSet.getGridStatistics(treeId);
      result.trees++;
      result.unrefinedCells += statistics.numberOfLocalUnrefinedCells;
      result.refinedCells += statistics.numberOfLocalRefinedCells;
      result.maxDepth = std::max(result.maxDepth, statistics.maxDepth);
      result.unrefinedCellsPerTree.push_back(statistics.numberOfLocalUnrefinedCells);
    }
    std::sort(result.unrefinedCellsPerTree.begin(), result.unrefinedCellsPerTree.end());

    result.routingResolved   = not spacetreeSet._periodicRouting.empty();
    result.routingIsSelfOnly = result.routingResolved;
    for (const auto& routingEntry : spacetreeSet._periodicRouting) {
      for (const auto& routedTreeIds : routingEntry.second) {
        for (const int treeId : routedTreeIds) {
          result.routingIsSelfOnly = result.routingIsSelfOnly and treeId == rootId;
        }
      }
    }

    // Later tests require the harness state without trees from this scenario.
    for (auto treeIterator = spacetreeSet._spacetrees.begin(); treeIterator != spacetreeSet._spacetrees.end();) {
      if (treeFamily.count(treeIterator->getId()) > 0) {
        spacetreeSet.unregisterLocalSpacetree(treeIterator->getId());
        parallelNode.deregisterId(treeIterator->getId());
        treeIterator = spacetreeSet._spacetrees.erase(treeIterator);
      } else {
        ++treeIterator;
      }
    }
  }

  spacetreeSet._periodicRouting.clear();
  spacetreeSet._crossRankPeriodicChannels.clear();
  spacetreeSet._lastPeriodicProducerDescriptors.clear();
  spacetreeSet._lastPeriodicConsumerDescriptors.clear();
  spacetreeSet._crossRankPeriodicGridVertexStaging.clear();
  spacetreeSet._periodicBC = backupPeriodicBC;

  return result;
}

void peano4::parallel::tests::SpacetreeSetTest::testRegularGridLoadBalancingMatchesNonPeriodicRun() {
  const auto withPeriodic    = runScenario(true, 4, false, false);
  const auto withoutPeriodic = runScenario(false, 4, false, false);

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    // The load balancing input is periodic-agnostic, so the decomposition has
    // to coincide tree by tree.
    validateEquals(withPeriodic.trees, withoutPeriodic.trees);
    validateEquals(withPeriodic.unrefinedCells, withoutPeriodic.unrefinedCells);
    validateEquals(withPeriodic.refinedCells, withoutPeriodic.refinedCells);
    validateEquals(withPeriodic.maxDepth, withoutPeriodic.maxDepth);
    validateEquals(
      static_cast<int>(withPeriodic.unrefinedCellsPerTree.size()),
      static_cast<int>(withoutPeriodic.unrefinedCellsPerTree.size())
    );
    for (std::size_t treeIndex = 0; treeIndex < withPeriodic.unrefinedCellsPerTree.size(); treeIndex++) {
      validateEqualsWithParams1(
        withPeriodic.unrefinedCellsPerTree[treeIndex],
        withoutPeriodic.unrefinedCellsPerTree[treeIndex],
        static_cast<int>(treeIndex)
      );
    }

    validateEquals(withPeriodic.trees, 4);
    validate(withPeriodic.splitsSucceeded);
    validate(withoutPeriodic.splitsSucceeded);
    validate(withPeriodic.routingResolved);
    validate(not withoutPeriodic.routingResolved);
  }
}

void peano4::parallel::tests::SpacetreeSetTest::testCornerRefinementMirrorsAcrossPeriodicBoundaries() {
  const auto withPeriodic    = runScenario(true, 1, true, false);
  const auto withoutPeriodic = runScenario(false, 1, true, false);

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    // The corner region crosses every periodic axis, so the wrap has to
    // mirror its refinement onto the far sides: the periodic grid holds
    // strictly more cells than the non-periodic one, whose refinement stays
    // confined to the corner.
    validateEquals(withPeriodic.maxDepth, withoutPeriodic.maxDepth);
    validateWithParams2(
      withPeriodic.unrefinedCells > withoutPeriodic.unrefinedCells,
      withPeriodic.unrefinedCells,
      withoutPeriodic.unrefinedCells
    );

    // A single tree owns both sides of every wrap, so the routing pairs the
    // tree with itself.
    validate(withPeriodic.routingResolved);
    validate(withPeriodic.routingIsSelfOnly);
  }
}

void peano4::parallel::tests::SpacetreeSetTest::testMultiTreePeriodicGridMatchesSingleTree() {
  const auto oneTree   = runScenario(true, 1, true, false);
  const auto fourTrees = runScenario(true, 4, true, false);

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    // The grid, including the refinement mirrored across the wrap, must not
    // depend on the domain decomposition.
    validateEquals(fourTrees.trees, 4);
    validate(fourTrees.splitsSucceeded);
    validateEquals(fourTrees.unrefinedCells, oneTree.unrefinedCells);
    validateEquals(fourTrees.refinedCells, oneTree.refinedCells);
    validateEquals(fourTrees.maxDepth, oneTree.maxDepth);
    validate(fourTrees.routingResolved);
    validate(not fourTrees.routingIsSelfOnly);
  }
}

void peano4::parallel::tests::SpacetreeSetTest::testEraseAtPeriodicBoundaryRestoresRegularGrid() {
  // Erasing runs on one tree: Peano vetoes coarsening above forked subtrees,
  // so a multi-tree grid deliberately keeps refinement deployed to workers,
  // with or without periodic boundaries.
  const auto regularGrid = runScenario(true, 1, false, false);
  const auto erasedAgain = runScenario(true, 1, true, true);

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    // The erase has to travel across the wrap exactly like the refinement
    // did, so the mirrored bands disappear too and the regular grid returns.
    validateEquals(erasedAgain.trees, regularGrid.trees);
    validateEquals(erasedAgain.unrefinedCells, regularGrid.unrefinedCells);
    validateEquals(erasedAgain.maxDepth, regularGrid.maxDepth);
  }
}
