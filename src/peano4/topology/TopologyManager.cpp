// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TopologyManager.h"

#include "peano4/communication/CommunicationManager.h"
#include "peano4/data/ContainerRegistry.h"
#include "peano4/grid/GridStatistics.h"
#include "peano4/grid/GridTraversalObserver.h"
#include "tarch/Assertions.h"
#include "tarch/Termination.h"
#include "tarch/mpi/AbstractCommunicationManager.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/tasking/FunctorTask.h"
#include "tarch/tasking/Runtime.h"
#include "tarch/tasking/TaskGraph.h"

namespace {
  /**
   * Compares two refinement plans flag by flag.
   *
   * Every plan is canonical, its flags sorted by lattice key on each level, so equal
   * levels compare equal element by element. A plan may carry trailing levels the
   * other one has not reached, and those hold no flags.
   */
  bool refinementPlansMatch(
    const peano4::topology::Tree::RefinementPlan& left,
    const peano4::topology::Tree::RefinementPlan& right
  ) {
    const std::size_t numberOfLevels = std::max(left.size(), right.size());
    for (std::size_t planLevel = 0; planLevel < numberOfLevels; planLevel++) {
      const auto* const leftLevel  = planLevel < left.size() ? &left[planLevel] : nullptr;
      const auto* const rightLevel = planLevel < right.size() ? &right[planLevel] : nullptr;
      const std::size_t leftSize   = leftLevel != nullptr ? leftLevel->size() : 0;
      const std::size_t rightSize  = rightLevel != nullptr ? rightLevel->size() : 0;
      if (leftSize != rightSize) {
        return false;
      }
      if (leftSize > 0 and *leftLevel != *rightLevel) {
        return false;
      }
    }
    return true;
  }
} // namespace

peano4::topology::TopologyManager peano4::topology::TopologyManager::_singleton;

peano4::topology::TopologyManager& peano4::topology::TopologyManager::getInstance() { return _singleton; }

void peano4::topology::TopologyManager::init(
  const tarch::la::Vector<DIMENSIONS, double>& offset,
  const tarch::la::Vector<DIMENSIONS, double>& width,
  const std::bitset<DIMENSIONS>&               periodicBC
) {
  auto& communicationManager = tarch::mpi::AbstractCommunicationManager::getInstance();
  communicationManager.registerPoller([]() {
    return peano4::communication::CommunicationManager::getInstance().pollInFlightTransfers();
  });
  communicationManager.start();

  // The rank is known here, so every registered container anchors its serial
  // slot now.
  peano4::data::ContainerRegistry::getInstance().initialiseSerialSlots();

  _domainOffset = offset;
  _domainWidth  = width;
  _periodicBC   = periodicBC;
}

/**
 * Schedules the direct construction of the regular base grid on the local
 * tree. Every rank calls this with the same target level, so the partition
 * follows from the rank count alone and needs no communication.
 */
void peano4::topology::TopologyManager::scheduleRegularGridConstruction(int targetLevel) { _targetLevel = targetLevel; }

void peano4::topology::TopologyManager::adoptRefinementPlan(peano4::topology::Tree::RefinementPlan refinementPlan) {
  assertion(_tree == nullptr);
  _refinementPlan = std::move(refinementPlan);
}

const peano4::topology::Tree::RefinementPlan& peano4::topology::TopologyManager::getRefinementPlan() const {
  return _refinementPlan;
}

const peano4::topology::Tree& peano4::topology::TopologyManager::getTree() const {
  assertion(_tree != nullptr);
  return *_tree;
}

/**
 * Records that the solvers' data is initialised. A regrid then migrates the
 * pools so solver data survives.
 */
void peano4::topology::TopologyManager::markPersistentDataLive() { _persistentDataLive = true; }

/**
 * Allows or forbids a regrid to move a block to another rank.
 *
 * The grid construction derives the decomposition either way, so a run that
 * forbids the move keeps that first decomposition for the whole run.
 */
void peano4::topology::TopologyManager::enableDynamicLoadBalancing(const bool enable) {
  _dynamicLoadBalancing = enable;
}

void peano4::topology::TopologyManager::shutdown() {
  _rankObserverOfType.clear();
  // A second stop is a no-op, so this covers a caller that has not stopped the
  // manager itself and whose containers live until this point.
  auto& communicationManager = tarch::mpi::AbstractCommunicationManager::getInstance();
  communicationManager.finishPendingWork();
  communicationManager.stop();
}

/**
 * Returns the kept rank-level clone of the given observer. One clone per
 * observer type constructs on the first call and is destroyed at shutdown.
 */
peano4::grid::GridTraversalObserver& peano4::topology::TopologyManager::rankObserverClone(
  peano4::grid::GridTraversalObserver& observer
) {
  auto& keptClone = _rankObserverOfType[std::type_index(typeid(observer))];
  if (keptClone == nullptr) {
    keptClone.reset(observer.clone(tarch::mpi::Rank::getInstance().getRank()));
    if (keptClone == nullptr) {
      tarch::terminateWithError("A grid traversal observer returned a null rank clone.");
    }
  }
  return *keptClone;
}

/** Returns the coarsest level providing sufficient distributed ownership granularity. */
int peano4::topology::TopologyManager::deriveSpawnLevel(const int targetLevel, const int numberOfRanks) {
  return peano4::topology::Tree::deriveSpawnLevel(targetLevel, numberOfRanks);
}

int peano4::topology::TopologyManager::deriveSpawnLevel() const {
  return deriveSpawnLevel(_targetLevel, tarch::mpi::Rank::getInstance().getNumberOfRanks());
}

/**
 * Chooses how a regrid realises its delta.
 *
 * A realisation in place needs the partition to stay put and, for an erase,
 * room for the gaps a prune leaves where the cells stand. A rank whose gaps
 * have grown or whose partition moves builds the new structure and moves the
 * entities onto it after the transition sweep.
 */
peano4::topology::TopologyManager::RegridRealisation peano4::topology::TopologyManager::selectRegridRealisation(
  const bool pureErase,
  const bool pureRefine,
  const bool partitionMoved,
  const bool gapsStayBounded
) {
  assertion(not(pureErase and pureRefine));
  if (pureErase) {
    return not partitionMoved and gapsStayBounded ? RegridRealisation::EraseInPlace
                                                  : RegridRealisation::EraseOnRebuiltTree;
  }
  if (partitionMoved) {
    return RegridRealisation::MixedWithAppendedUnion;
  }
  if (pureRefine) {
    return RegridRealisation::RefineInPlace;
  }
  return gapsStayBounded ? RegridRealisation::MixedInPlace : RegridRealisation::MixedWithAppendedUnion;
}

/** Returns a spare tree to rebuild into, or a new one when no spare is left. */
std::unique_ptr<peano4::topology::Tree> peano4::topology::TopologyManager::acquireTree() {
  if (not _spareTrees.empty()) {
    auto spareTree = std::move(_spareTrees.back());
    _spareTrees.pop_back();
    return spareTree;
  }
  const auto rank = tarch::mpi::Rank::getInstance().getRank();
  return std::make_unique<peano4::topology::Tree>(rank, _domainOffset, _domainWidth, _periodicBC);
}

/** Returns a spare traversal to rebuild into, or a new one when no spare is left. */
std::unique_ptr<peano4::topology::TreeTraversal> peano4::topology::TopologyManager::acquireTraversal() {
  if (not _spareTraversals.empty()) {
    auto spareTraversal = std::move(_spareTraversals.back());
    _spareTraversals.pop_back();
    return spareTraversal;
  }
  const auto rank = tarch::mpi::Rank::getInstance().getRank();
  return std::make_unique<peano4::topology::TreeTraversal>(rank, _domainOffset, _domainWidth);
}

void peano4::topology::TopologyManager::traverse(peano4::grid::GridTraversalObserver& observer) { runSweep(observer); }

/**
 * Returns whether the given control events change the refinement plan, so a
 * caller skips a regrid sweep over a set the plan rejects. Before the tree
 * stands every non-empty set counts as a change.
 */
bool peano4::topology::TopologyManager::controlEventsChangePlan(
  const std::vector<peano4::grid::GridControlEvent>& gridControlEvents
) {
  if (_tree == nullptr) {
    return not gridControlEvents.empty();
  }
  peano4::topology::Tree::RefinementPlan updatedPlan;
  return derivePlanUpdate(gridControlEvents, updatedPlan);
}

/**
 * Evaluates the control events against the standing plan and returns whether
 * they change it; updatedPlan then holds the changed plan. The last evaluated
 * set and its result are cached, so a repeated set answers from the cache.
 */
bool peano4::topology::TopologyManager::derivePlanUpdate(
  const std::vector<peano4::grid::GridControlEvent>& gridControlEvents,
  peano4::topology::Tree::RefinementPlan&            updatedPlan
) {
  assertion(_tree != nullptr);
  if (gridControlEvents.empty()) {
    return false;
  }
  if (gridControlEvents == _lastEvaluatedControlEvents) {
    updatedPlan = _lastEvaluatedPlanUpdate;
    return _lastEvaluationChangedPlan;
  }
  updatedPlan            = _refinementPlan;
  const bool planChanged = _tree->updateRefinementPlan(gridControlEvents, updatedPlan)
    and not refinementPlansMatch(updatedPlan, _refinementPlan);
  _lastEvaluatedControlEvents = gridControlEvents;
  _lastEvaluatedPlanUpdate    = updatedPlan;
  _lastEvaluationChangedPlan  = planChanged;
  return planChanged;
}

/**
 * Builds the tree from the target level and the current refinement plan,
 * derives the rank and interface topologies, sizes every registered pool to
 * the entity counts plus the cross-rank frozen and task shadow slots, and
 * rebuilds the exchange topology. A rebuild drops all pool data and event
 * recordings, so a creational sweep has to follow.
 */
void peano4::topology::TopologyManager::buildTree(
  const bool                                    resizePools,
  const peano4::topology::Tree::RefinementPlan& refinementPlan
) {
  const auto numberOfRanks     = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  auto&      containerRegistry = peano4::data::ContainerRegistry::getInstance();

  assertion(_targetLevel >= 0);
  const int spawnLevel = deriveSpawnLevel();
  _tree                = acquireTree();

  // Creational construction can adopt the plan's balanced partition because no
  // data has to migrate. An unrefined grid keeps the default partition when its
  // spawn blocks cover every rank; shallower grids distribute child ownership.
  std::int64_t numberOfBlocks = 1;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    numberOfBlocks *= peano4::topology::cellsPerAxis(spawnLevel);
  }
  const bool balancesCreationalGrid = numberOfRanks > 1 and spawnLevel < _targetLevel and not _persistentDataLive
    and (not refinementPlan.empty() or numberOfBlocks < numberOfRanks);
  if (balancesCreationalGrid) {
    _blockOwners = _tree->deriveBalancedBlockOwners(_targetLevel, numberOfRanks, refinementPlan);
  }
  _tree->buildRegularGrid(
    _targetLevel,
    spawnLevel,
    numberOfRanks,
    refinementPlan,
    _blockOwners.empty() ? nullptr : &_blockOwners
  );
  if (_blockOwners.empty()) {
    _blockOwners = _tree->getOwnersOfBlocks();
  }
  _traversal = acquireTraversal();
  _traversal->prepareRankTopology(*_tree);

  // Deriving the interface topology before the first sweep sizes the pools
  // once, shadow slots included, so no later replication reallocates them.
  if (tarch::tasking::Runtime::getInstance().getNumberOfThreads() > 1 and _tree->getDepth() > 0
      and containerRegistry.supportsSubtreeTaskReplication()) {
    _traversal->prepareInterfaceTopology(*_tree, spawnLevel);
  }
  if (resizePools) {
    containerRegistry.resizePoolsForTree(
      _traversal->getCellSlotSpaceSize(),
      _traversal->getFaceSlotSpaceSize() + _traversal->getNumberOfFaceRankFrozenSlots()
        + _traversal->getNumberOfFaceShadowSlots(),
      _traversal->getVertexSlotSpaceSize() + _traversal->getNumberOfVertexRankFrozenSlots()
        + _traversal->getNumberOfVertexShadowSlots()
    );
  }
  peano4::communication::CommunicationManager::getInstance().rebuildTopology(_traversal->getNeighbourRanks());
}

/**
 * Runs one sweep and applies a changed refinement plan before the observer
 * traverses the resulting transition. Every transition emits its create and
 * destroy events in one union sweep. With neighbour ranks each sweep posts one
 * aggregated boundary message per rank pair and container, and the following
 * sweep completes these receives before its boundary tasks read the frozen
 * slots.
 */
void peano4::topology::TopologyManager::runSweep(peano4::grid::GridTraversalObserver& observer) {
  auto& containerRegistry = peano4::data::ContainerRegistry::getInstance();

  // A caller raises the live marker on the global master alone, and the regrid
  // branch below must be rank-uniform. This vote hands the master's flag to
  // every rank at the same sweep and stops once agreed.
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1 and not _persistentDataLiveUniform) {
    int localDataLive  = _persistentDataLive ? 1 : 0;
    int globalDataLive = 0;
    tarch::mpi::allreduce(
      &localDataLive,
      &globalDataLive,
      1,
      MPI_INT,
      MPI_MAX,
      tarch::mpi::Rank::getInstance().getCommunicator()
    );
    _persistentDataLive = globalDataLive != 0;
    if (_persistentDataLive) {
      _persistentDataLiveUniform = true;
    }
  }

  const auto finishBoundaryReceives = [this, &containerRegistry]() {
    if (_traversal->getNeighbourRanks().empty()) {
      return;
    }
    containerRegistry.finishBoundaryExchange(
      _traversal->getFacePackLists(),
      _traversal->getVertexPackLists(),
      _traversal->getFaceFrozenSlotBases(),
      _traversal->getVertexFrozenSlotBases()
    );
  };
  const auto drainBoundaryTransfers = [this]() {
    if (_traversal->getNeighbourRanks().empty()) {
      return;
    }
    auto& boundaryExchange = peano4::communication::CommunicationManager::getInstance();
    while (not boundaryExchange.haveTransfersCompleted()) {
      tarch::mpi::AbstractCommunicationManager::getInstance().progress();
    }
  };
  // Nothing may target a pool or channel after a rebuild replaces it.
  const auto drainBoundaryExchange = [&finishBoundaryReceives, &drainBoundaryTransfers]() {
    finishBoundaryReceives();
    drainBoundaryTransfers();
  };

  const auto migrationHop = [this, &containerRegistry](
                              std::unique_ptr<peano4::topology::Tree>&&          targetTree,
                              std::unique_ptr<peano4::topology::TreeTraversal>&& targetTraversal,
                              const std::vector<std::pair<int, int>>&            cellSlotPairs,
                              const std::vector<std::pair<int, int>>&            faceSlotPairs,
                              const std::vector<std::pair<int, int>>&            vertexSlotPairs,
                              const bool                                         transfersMovedData
                            ) {
    assertion(targetTree != nullptr and targetTraversal != nullptr);
    if (transfersMovedData) {
      containerRegistry.transferPoolsForTree(
        _regridTransition->sendCellSlots,
        _regridTransition->receiveCellSlots,
        _regridTransition->sendFaceSlots,
        _regridTransition->receiveFaceSlots,
        _regridTransition->sendVertexSlots,
        _regridTransition->receiveVertexSlots
      );
    }
    const auto sweepStatistics = _traversal->getGridStatistics();
    _spareTrees.push_back(std::move(_tree));
    _spareTraversals.push_back(std::move(_traversal));
    _tree      = std::move(targetTree);
    _traversal = std::move(targetTraversal);
    peano4::communication::CommunicationManager::getInstance().rebuildTopology(_traversal->getNeighbourRanks());
    _traversal->adoptGridStatistics(sweepStatistics);
    containerRegistry.migratePoolsForTree(
      cellSlotPairs,
      faceSlotPairs,
      vertexSlotPairs,
      _traversal->getCellSlotSpaceSize(),
      _traversal->getFaceSlotSpaceSize() + _traversal->getNumberOfFaceRankFrozenSlots()
        + _traversal->getNumberOfFaceShadowSlots(),
      _traversal->getVertexSlotSpaceSize() + _traversal->getNumberOfVertexRankFrozenSlots()
        + _traversal->getNumberOfVertexShadowSlots()
    );
    if (tarch::tasking::Runtime::getInstance().getNumberOfThreads() > 1 and _tree->getDepth() > 0
        and containerRegistry.supportsSubtreeTaskReplication()) {
      containerRegistry.replicateInterfaceSlots(
        _traversal->getInterfaceSeedFacePairs(),
        _traversal->getInterfaceSeedVertexPairs()
      );
      containerRegistry.replicateInterfaceSlots(
        _traversal->getInterfaceCopyToFrozenFacePairs(),
        _traversal->getInterfaceCopyToFrozenVertexPairs()
      );
    }
  };

  auto        sweepKind       = peano4::topology::TreeTraversal::PoolSweepKind::Standing;
  const auto* regridOverrides = static_cast<const peano4::topology::TreeTraversal::RegridOverrides*>(nullptr);
  char        finalInterfaceTopologyDerived{};
  bool        finalInterfaceTopologyPending = false;
  bool        taskInterfaceDataMerged       = false;
  const auto  waitForFinalInterfaceTopology = [&finalInterfaceTopologyDerived, &finalInterfaceTopologyPending]() {
    if (finalInterfaceTopologyPending) {
      tarch::tasking::waitForDependency(&finalInterfaceTopologyDerived);
      finalInterfaceTopologyPending = false;
    }
  };
  if (_tree == nullptr) {
    buildTree(true, _refinementPlan);
    sweepKind = peano4::topology::TreeTraversal::PoolSweepKind::Creational;
  } else {
    const auto                             gridControlEvents = observer.getGridControlEvents();
    peano4::topology::Tree::RefinementPlan updatedPlan;
    if (derivePlanUpdate(gridControlEvents, updatedPlan)) {
      _lastEvaluatedControlEvents.clear();
      _lastEvaluatedPlanUpdate.clear();
      _lastEvaluationChangedPlan = false;
      if (not _persistentDataLive) {
        // During grid construction no data has to survive, so the tree
        // rebuilds and the sweep re-creates everything.
        _refinementPlan = std::move(updatedPlan);
        drainBoundaryExchange();
        buildTree(true, _refinementPlan);
        sweepKind = peano4::topology::TreeTraversal::PoolSweepKind::Creational;
      } else {
        // The transition derives before the sweep so the observer traverses
        // the union structure once with the old and new states available.
        peano4::topology::Tree::RefinementPlan unionPlan;
        peano4::topology::Tree::RefinementPlan changedFlags;
        peano4::topology::Tree::RefinementPlan addedFlags;
        peano4::topology::Tree::RefinementPlan removedFlags;
        bool                                   anyAddedFlag   = false;
        bool                                   anyRemovedFlag = false;
        const auto numberOfPlanLevels                         = std::max(_refinementPlan.size(), updatedPlan.size());
        // Every plan level is sorted by lattice key, so one linear merge per
        // level yields the union and the symmetric difference, which is the
        // regrid's delta.
        const std::vector<tarch::la::Vector<DIMENSIONS, int>> emptyPlanLevel;
        for (std::size_t planLevel = 0; planLevel < numberOfPlanLevels; planLevel++) {
          const auto& oldLevel     = planLevel < _refinementPlan.size() ? _refinementPlan[planLevel] : emptyPlanLevel;
          const auto& newLevel     = planLevel < updatedPlan.size() ? updatedPlan[planLevel] : emptyPlanLevel;
          auto&       unionLevel   = unionPlan.emplace_back();
          auto&       changedLevel = changedFlags.emplace_back();
          auto&       addedLevel   = addedFlags.emplace_back();
          auto&       removedLevel = removedFlags.emplace_back();
          unionLevel.reserve(oldLevel.size() + newLevel.size());
          std::size_t oldNumber = 0;
          std::size_t newNumber = 0;
          while (oldNumber < oldLevel.size() and newNumber < newLevel.size()) {
            if (peano4::topology::Tree::latticeKeyPrecedes(oldLevel[oldNumber], newLevel[newNumber])) {
              unionLevel.push_back(oldLevel[oldNumber]);
              changedLevel.push_back(oldLevel[oldNumber]);
              removedLevel.push_back(oldLevel[oldNumber]);
              anyRemovedFlag = true;
              oldNumber++;
            } else if (peano4::topology::Tree::latticeKeyPrecedes(newLevel[newNumber], oldLevel[oldNumber])) {
              unionLevel.push_back(newLevel[newNumber]);
              changedLevel.push_back(newLevel[newNumber]);
              addedLevel.push_back(newLevel[newNumber]);
              anyAddedFlag = true;
              newNumber++;
            } else {
              unionLevel.push_back(newLevel[newNumber]);
              oldNumber++;
              newNumber++;
            }
          }
          for (; oldNumber < oldLevel.size(); oldNumber++) {
            unionLevel.push_back(oldLevel[oldNumber]);
            changedLevel.push_back(oldLevel[oldNumber]);
            removedLevel.push_back(oldLevel[oldNumber]);
            anyRemovedFlag = true;
          }
          for (; newNumber < newLevel.size(); newNumber++) {
            unionLevel.push_back(newLevel[newNumber]);
            changedLevel.push_back(newLevel[newNumber]);
            addedLevel.push_back(newLevel[newNumber]);
            anyAddedFlag = true;
          }
        }
        const bool        pureErase     = not anyAddedFlag;
        const bool        pureRefine    = not anyRemovedFlag;
        const auto        numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
        const int         spawnLevel    = deriveSpawnLevel();
        const auto* const currentOwners = _blockOwners.empty() ? nullptr : &_blockOwners;
        auto              newTree       = acquireTree();
        auto              newTraversal  = acquireTraversal();
        std::vector<int>  balancedOwners;
        const bool        derivesInterfaceTopologies = tarch::tasking::Runtime::getInstance().getNumberOfThreads() > 1
          and _targetLevel > 0 and containerRegistry.supportsSubtreeTaskReplication();

        // A standing partition that splits a cell with a leaf child under the
        // new plan separates a coarsening family or the fine cells that
        // restrict onto one coarse face, so the partition derives anew.
        const bool standingPartitionSplitsLeafFamily = numberOfRanks > 1 and not _blockOwners.empty()
          and newTree->ownershipPlanSplitsCellWithLeafChild(_targetLevel, spawnLevel, updatedPlan, _blockOwners);
        // The plan implies the leaf weights, so the balanced partition derives
        // before the build and every rank computes the identical table without
        // communication. A false _dynamicLoadBalancing value skips the
        // derivation, and the buildNewTree() lambda then reads currentOwners.
        if (numberOfRanks > 1 and not _blockOwners.empty()
            and (_dynamicLoadBalancing or standingPartitionSplitsLeafFamily)) {
          balancedOwners = newTree->deriveBalancedBlockOwners(_targetLevel, numberOfRanks, updatedPlan);
          if (balancedOwners == _blockOwners) {
            balancedOwners.clear();
          }
        }
        // The standing partition stays while its largest rank load is within
        // the tolerance of the balanced one, so the regrid realises in place.
        if (not balancedOwners.empty() and not standingPartitionSplitsLeafFamily) {
          const auto largestLoadOf = [](const std::vector<std::int64_t>& loads) {
            return *std::max_element(loads.begin(), loads.end());
          };
          const auto standingLoad =
            largestLoadOf(newTree->deriveRankLoads(_targetLevel, numberOfRanks, updatedPlan, _blockOwners));
          const auto balancedLoad =
            largestLoadOf(newTree->deriveRankLoads(_targetLevel, numberOfRanks, updatedPlan, balancedOwners));
          if (standingLoad * 100 <= balancedLoad * (100 + RepartitionTolerancePercent)) {
            balancedOwners.clear();
          }
        }
        const bool partitionMoved = not balancedOwners.empty();
        const auto buildNewTree =
          [this, &newTree, &updatedPlan, &balancedOwners, spawnLevel, numberOfRanks, currentOwners]() {
            const auto* const newOwners = balancedOwners.empty() ? currentOwners : &balancedOwners;
            newTree->buildRegularGrid(_targetLevel, spawnLevel, numberOfRanks, updatedPlan, newOwners);
          };

        // Gaps of dead cells stay while a structure prunes in place, so
        // once they rival the live cells the regrid rebuilds and compacts.
        const auto numberOfDeadCells = static_cast<std::size_t>(_tree->getNumberOfDeadCells());
        const bool gapsStayBounded   = numberOfDeadCells < _tree->getNumberOfCells() - numberOfDeadCells;
        const auto realisation       = selectRegridRealisation(pureErase, pureRefine, partitionMoved, gapsStayBounded);

        // In place on a partitioned rank: The receives of the last sweep still
        // target the standing pack lists, so they complete and the tasks'
        // interface data merges with them before the tree changes. The
        // boundary then derives anew behind the grown slot ranges, and the
        // frozen slots that hold partner data move along.
        const bool inPlaceAcrossRanks            = _tree->hasDistributedOwnership();
        const bool keepsStandingDecomposition    = derivesInterfaceTopologies and not inPlaceAcrossRanks;
        const auto settleStandingExchangeInPlace = [this,
                                                    inPlaceAcrossRanks,
                                                    derivesInterfaceTopologies,
                                                    &observer,
                                                    &containerRegistry,
                                                    &drainBoundaryExchange,
                                                    &taskInterfaceDataMerged]() {
          if (not inPlaceAcrossRanks) {
            return;
          }
          if (not _skipStandingBoundaryFinish) {
            drainBoundaryExchange();
          }
          // The appended entities and the moved frozen slots take the places
          // of the shadow slots, so the tasks' live copies flush into the
          // entity slots before the tree changes.
          if (derivesInterfaceTopologies) {
            if (_traversal->getNumberOfReplayTasks() > 0) {
              _traversal->mergeTaskInterfaceData(rankObserverClone(observer), {});
            }
            containerRegistry.replicateInterfaceSlots(
              _traversal->getInterfaceFlushFacePairs(),
              _traversal->getInterfaceFlushVertexPairs()
            );
            taskInterfaceDataMerged = true;
          }
          _skipStandingBoundaryFinish = true;
        };
        // The re-derived boundary can add or drop a neighbour rank, and the
        // pack lists index the manager's rank set, so the set follows.
        const auto relocateFrozenSlots =
          [this, &containerRegistry](const peano4::topology::TreeTraversal::RankBoundaryRelocation& relocation) {
            containerRegistry.migratePoolsForTree(
              {},
              relocation.facePairs,
              relocation.vertexPairs,
              _traversal->getCellSlotSpaceSize(),
              _traversal->getFaceSlotSpaceSize() + _traversal->getNumberOfFaceRankFrozenSlots(),
              _traversal->getVertexSlotSpaceSize() + _traversal->getNumberOfVertexRankFrozenSlots()
            );
            peano4::communication::CommunicationManager::getInstance().rebuildTopology(_traversal->getNeighbourRanks());
          };
        // Only a frozen slot that moved along holds partner data, so the
        // transition sweep merges those and no other.
        const auto markMovedFrozenSlotsValid =
          [this](const peano4::topology::TreeTraversal::RankBoundaryRelocation& relocation) {
            const auto markValid = [](
                                     const std::vector<std::pair<int, int>>& pairs,
                                     const int                               firstFrozenSlot,
                                     std::vector<char>&                      frozenSlotValid
                                   ) {
              std::fill(frozenSlotValid.begin() + firstFrozenSlot, frozenSlotValid.end(), 0);
              for (const auto& [oldSlot, newSlot] : pairs) {
                frozenSlotValid[newSlot] = 1;
              }
            };
            markValid(
              relocation.vertexPairs,
              _traversal->getVertexSlotSpaceSize(),
              _regridTransition->overrides.vertexFrozenSlotValid
            );
            markValid(
              relocation.facePairs,
              _traversal->getFaceSlotSpaceSize(),
              _regridTransition->overrides.faceFrozenSlotValid
            );
          };
        if (realisation == RegridRealisation::EraseInPlace) {
          // A pure erase realises fully in place: The standing structure is
          // the union, the transition destroys the removed region's data on
          // it, and the prune afterwards marks the region's entities unused.
          _spareTrees.push_back(std::move(newTree));
          _spareTraversals.push_back(std::move(newTraversal));
          auto newPlanView = acquireTree();
          newPlanView->buildPlanQueryTablesFrom(_targetLevel, *_tree, updatedPlan, addedFlags, removedFlags);
          _regridTransition = _traversal->deriveRegridTransition(
            *_tree,
            *_tree,
            *_traversal,
            *newPlanView,
            *_traversal,
            changedFlags,
            false,
            true
          );
          _transitionPlanView = std::move(newPlanView);
          if (keepsStandingDecomposition) {
            _traversal->retainRecordingsForPartialRerecord({});
            _traversal->snapshotStandingDecomposition();
          }
          _transitionPruneFlags                    = std::move(removedFlags);
          _transitionPrunesStructure               = true;
          _refinementPlan                          = std::move(updatedPlan);
          _transitionSweepPending                  = true;
          _transitionFlushPending                  = true;
          _transitionOnNewStructure                = true;
          _transitionRestoresDecompositionSnapshot = keepsStandingDecomposition;
        } else if (realisation == RegridRealisation::EraseOnRebuiltTree) {
          // A pure erase's union is the standing structure, so the
          // transition runs on it in place: No union build, no first
          // migration hop, and every frozen slot keeps its remote data.
          buildNewTree();
          newTraversal->prepareRankTopology(*newTree, _tree.get(), _traversal.get());
          _regridTransition = _traversal->deriveRegridTransition(
            *_tree,
            *_tree,
            *_traversal,
            *newTree,
            *newTraversal,
            changedFlags,
            partitionMoved,
            true
          );
          if (derivesInterfaceTopologies) {
            auto* const       finalTraversal = newTraversal.get();
            const auto* const finalTree      = newTree.get();
            finalInterfaceTopologyPending    = true;
            tarch::tasking::spawnTask(
              new tarch::tasking::FunctorTask([finalTraversal, finalTree, spawnLevel]() {
                finalTraversal->prepareInterfaceTopology(*finalTree, spawnLevel);
              }),
              {},
              {&finalInterfaceTopologyDerived}
            );
          }
          _preparedNewTree          = std::move(newTree);
          _preparedNewTraversal     = std::move(newTraversal);
          _refinementPlan           = std::move(updatedPlan);
          _pendingBlockOwners       = std::move(balancedOwners);
          _transitionSweepPending   = true;
          _transitionFlushPending   = true;
          _transitionOnNewStructure = false;
        } else if (realisation == RegridRealisation::RefineInPlace) {
          // A pure refinement realises in place: The standing tree appends
          // the added region and the transition runs on it as the final
          // structure.
          _spareTrees.push_back(std::move(newTree));
          _spareTraversals.push_back(std::move(newTraversal));
          auto planView = acquireTree();
          planView->buildPlanQueryTablesFrom(_targetLevel, *_tree, updatedPlan, addedFlags, removedFlags);
          const auto appendedDelta = _tree->appendRefinementDelta(*planView, updatedPlan, addedFlags);
          settleStandingExchangeInPlace();
          const auto relocation = _traversal->extendRankTopologyForAppendedEntities(*_tree, appendedDelta);
          relocateFrozenSlots(relocation);
          _regridTransition =
            _traversal
              ->deriveRegridTransition(*_tree, *_tree, *_traversal, *planView, *_traversal, changedFlags, false, true);
          if (inPlaceAcrossRanks) {
            markMovedFrozenSlotsValid(relocation);
          }
          if (keepsStandingDecomposition) {
            const auto staleUnits = _traversal->extendInterfaceTopologyForAppendedEntities(*_tree, appendedDelta);
            _traversal->retainRecordingsForPartialRerecord(staleUnits);
            _traversal->snapshotStandingDecomposition();
          }
          _tree->adoptRefinementPlanTablesFrom(*planView);
          _spareTrees.push_back(std::move(planView));
          _refinementPlan                          = std::move(updatedPlan);
          _transitionSweepPending                  = true;
          _transitionFlushPending                  = true;
          _transitionOnNewStructure                = true;
          _transitionRestoresDecompositionSnapshot = keepsStandingDecomposition;
        } else if (realisation == RegridRealisation::MixedInPlace) {
          // A mixed delta realises fully in place: The standing tree
          // appends the added region and becomes the union, the transition
          // runs on it, and the prune afterwards marks the removed region
          // unused.
          _spareTrees.push_back(std::move(newTree));
          _spareTraversals.push_back(std::move(newTraversal));
          const peano4::topology::Tree::RefinementPlan noFlags;
          auto                                         unionPlanView = acquireTree();
          unionPlanView->buildPlanQueryTablesFrom(_targetLevel, *_tree, unionPlan, addedFlags, noFlags);
          const auto appendedDelta = _tree->appendRefinementDelta(*unionPlanView, unionPlan, addedFlags);
          settleStandingExchangeInPlace();
          const auto relocation = _traversal->extendRankTopologyForAppendedEntities(*_tree, appendedDelta);
          relocateFrozenSlots(relocation);
          auto oldPlanView = acquireTree();
          oldPlanView->buildPlanQueryTablesFrom(_targetLevel, *_tree, _refinementPlan, noFlags, noFlags);
          auto newPlanView = acquireTree();
          newPlanView->buildPlanQueryTablesFrom(_targetLevel, *_tree, updatedPlan, addedFlags, removedFlags);
          // The transition derivation probes the union's plan on the
          // patched tree and the other plans on query views, so the
          // adoption precedes the derivation.
          _tree->adoptRefinementPlanTablesFrom(*unionPlanView);
          _spareTrees.push_back(std::move(unionPlanView));
          _regridTransition = _traversal->deriveRegridTransition(
            *oldPlanView,
            *_tree,
            *_traversal,
            *newPlanView,
            *_traversal,
            changedFlags,
            false,
            true
          );
          _spareTrees.push_back(std::move(oldPlanView));
          _transitionPlanView = std::move(newPlanView);
          if (inPlaceAcrossRanks) {
            markMovedFrozenSlotsValid(relocation);
          }
          if (keepsStandingDecomposition) {
            const auto staleUnits = _traversal->extendInterfaceTopologyForAppendedEntities(*_tree, appendedDelta);
            _traversal->retainRecordingsForPartialRerecord(staleUnits);
            _traversal->snapshotStandingDecomposition();
          }
          _transitionPruneFlags                    = std::move(removedFlags);
          _transitionPrunesStructure               = true;
          _refinementPlan                          = std::move(updatedPlan);
          _transitionSweepPending                  = true;
          _transitionFlushPending                  = true;
          _transitionOnNewStructure                = true;
          _transitionRestoresDecompositionSnapshot = keepsStandingDecomposition;
        } else {
          // The union realises in place: The standing tree appends the added
          // region. The new structure builds concurrently with the append,
          // because the erased entities leave with the hop onto it.
          char newTreeBuilt{};
          tarch::tasking::spawnTask(new tarch::tasking::FunctorTask(buildNewTree), {}, {&newTreeBuilt});
          const peano4::topology::Tree::RefinementPlan noFlags;
          auto                                         unionPlanView = acquireTree();
          unionPlanView->buildPlanQueryTablesFrom(_targetLevel, *_tree, unionPlan, addedFlags, noFlags);
          const auto appendedDelta = _tree->appendRefinementDelta(*unionPlanView, unionPlan, addedFlags);
          settleStandingExchangeInPlace();
          const auto relocation = _traversal->extendRankTopologyForAppendedEntities(*_tree, appendedDelta);
          relocateFrozenSlots(relocation);
          tarch::tasking::waitForDependency(&newTreeBuilt);
          newTraversal->prepareRankTopology(*newTree, _tree.get(), _traversal.get());
          // The transition derivation probes the union's plan on the patched
          // tree and the pre-append plan on a query view, so the adoption
          // precedes the derivation.
          auto oldPlanView = acquireTree();
          oldPlanView->buildPlanQueryTablesFrom(_targetLevel, *_tree, _refinementPlan, noFlags, noFlags);
          _tree->adoptRefinementPlanTablesFrom(*unionPlanView);
          _spareTrees.push_back(std::move(unionPlanView));
          _regridTransition = _traversal->deriveRegridTransition(
            *oldPlanView,
            *_tree,
            *_traversal,
            *newTree,
            *newTraversal,
            changedFlags,
            partitionMoved,
            true
          );
          if (inPlaceAcrossRanks) {
            markMovedFrozenSlotsValid(relocation);
          }
          _spareTrees.push_back(std::move(oldPlanView));
          if (derivesInterfaceTopologies) {
            auto* const       finalTraversal = newTraversal.get();
            const auto* const finalTree      = newTree.get();
            finalInterfaceTopologyPending    = true;
            tarch::tasking::spawnTask(
              new tarch::tasking::FunctorTask([finalTraversal, finalTree, spawnLevel]() {
                finalTraversal->prepareInterfaceTopology(*finalTree, spawnLevel);
              }),
              {},
              {&finalInterfaceTopologyDerived}
            );
          }
          _preparedNewTree          = std::move(newTree);
          _preparedNewTraversal     = std::move(newTraversal);
          _refinementPlan           = std::move(updatedPlan);
          _pendingBlockOwners       = std::move(balancedOwners);
          _transitionSweepPending   = true;
          _transitionFlushPending   = true;
          _transitionOnNewStructure = false;
        }
      }
    }
  }

  const bool transitionPrepared = _transitionFlushPending;
  const bool sourceHasTaskSlots = tarch::tasking::Runtime::getInstance().getNumberOfThreads() > 1
    and _tree->getDepth() > 0 and containerRegistry.supportsSubtreeTaskReplication();
  const bool sourceBoundaryExchangeConsumed = _skipStandingBoundaryFinish;
  if (transitionPrepared and sourceHasTaskSlots and _traversal->getNumberOfReplayTasks() > 0
      and not taskInterfaceDataMerged) {
    std::function<void()> waitForRemoteData;
    if (not _traversal->getNeighbourRanks().empty() and not sourceBoundaryExchangeConsumed) {
      waitForRemoteData = finishBoundaryReceives;
    }
    _traversal->mergeTaskInterfaceData(rankObserverClone(observer), waitForRemoteData);
    if (waitForRemoteData) {
      drainBoundaryTransfers();
      _skipStandingBoundaryFinish = true;
    }
  }
  if (_transitionFlushPending) {
    if (sourceHasTaskSlots and not taskInterfaceDataMerged) {
      containerRegistry.replicateInterfaceSlots(
        _traversal->getInterfaceFlushFacePairs(),
        _traversal->getInterfaceFlushVertexPairs()
      );
    }
    _transitionFlushPending = false;
  }
  // The transition sweep traverses the union tree: Vanishing entities destroy
  // at their last touches, new ones create at their first touches, and the
  // refinement masks carry the states of both structures.
  if (_transitionSweepPending) {
    regridOverrides         = &_regridTransition->overrides;
    _transitionSweepPending = false;
  }
  const bool ranTransitionSweep = regridOverrides != nullptr;
  peano4::communication::CommunicationManager::getInstance().beginSweep();

  // The subtree tasks of a rank are exactly its spawn-level blocks, so the task
  // level comes from the tree.
  const int taskLevel = _tree->getSpawnLevel();
  // A container whose copies must not diverge keeps the rank on one subtree
  // task altogether.
  const bool decomposesIntoTasks = tarch::tasking::Runtime::getInstance().getNumberOfThreads() > 1
    and _tree->getDepth() > 0 and containerRegistry.supportsSubtreeTaskReplication();
  const bool runsParallelSweeps = decomposesIntoTasks and not ranTransitionSweep;
  // The transition writes the entity slots directly under block ordering
  // dependencies, so it parallelises without the persistent-copy protocol and
  // skips that protocol's copies.
  const bool transitionRunsParallel      = decomposesIntoTasks and ranTransitionSweep;
  const bool exchangesWithNeighbourRanks = not _traversal->getNeighbourRanks().empty();

  // The creational sweep has nothing in flight, so only a standing sweep
  // completes the neighbour ranks' receives. A sweep following a migration
  // hop skips the completion too: The hop's drain already consumed the
  // pending messages under the pre-hop topology's layout.
  const bool skipStandingBoundaryFinish = _skipStandingBoundaryFinish;
  _skipStandingBoundaryFinish           = false;
  std::function<void()> waitForRemoteData;
  if (exchangesWithNeighbourRanks and sweepKind == peano4::topology::TreeTraversal::PoolSweepKind::Standing
      and not skipStandingBoundaryFinish) {
    waitForRemoteData = finishBoundaryReceives;
  }

  auto& observerClone = rankObserverClone(observer);
  if (runsParallelSweeps or transitionRunsParallel) {
    if (ranTransitionSweep) {
      if (_transitionOnNewStructure) {
        waitForFinalInterfaceTopology();
      }
      _traversal->prepareTransitionTopology(*_tree, taskLevel, *regridOverrides);
    }
    _traversal->prepareParallelSweep(*_tree, taskLevel, not ranTransitionSweep);
    containerRegistry.prepareLocalSlots(1 + _traversal->getNumberOfSubtreeTasks());
    _traversal->traverseWithPoolTasks(*_tree, observerClone, sweepKind, taskLevel, waitForRemoteData, regridOverrides);
    if (runsParallelSweeps) {
      // A rank-boundary entity several tasks touch consolidates into the
      // entity slot the pack sends: The first toucher's copy rebuilds it and
      // the other touchers' copies merge into it.
      _traversal->runConsolidations(observerClone);
      containerRegistry.replicateInterfaceSlots(
        _traversal->getInterfaceCopyToFrozenFacePairs(),
        _traversal->getInterfaceCopyToFrozenVertexPairs()
      );
      // The creational sweep copies the first toucher's created copy into the
      // entity slot the pack sends.
      if (sweepKind == peano4::topology::TreeTraversal::PoolSweepKind::Creational) {
        containerRegistry.replicateInterfaceSlots(
          _traversal->getCreationalPublishFacePairs(),
          _traversal->getCreationalPublishVertexPairs()
        );
      }
    }
  } else {
    if (waitForRemoteData) {
      waitForRemoteData();
    }
    _traversal->traverseWithPools(*_tree, observerClone, sweepKind, regridOverrides);
  }
  // The _accumulatedTaskSeconds value adds the task time before the hop
  // replaces its source traversal.
  _accumulatedTaskSeconds += _traversal->getLastSweepTaskSeconds();
  if (exchangesWithNeighbourRanks) {
    containerRegistry.packBoundarySlots(_traversal->getFacePackLists(), _traversal->getVertexPackLists());
  }

  if (ranTransitionSweep and _transitionOnNewStructure) {
    waitForFinalInterfaceTopology();
    const bool prunedStructure = _transitionPrunesStructure;
    const bool acrossRanks     = _tree->hasDistributedOwnership();
    if (prunedStructure) {
      // The view built for the new plan before the sweep carries its tables.
      assertion(_transitionPlanView != nullptr);
      _tree->adoptRefinementPlanTablesFrom(*_transitionPlanView);
      _spareTrees.push_back(std::move(_transitionPlanView));
      const auto pruneExtent = _tree->pruneRefinementDelta(_transitionPruneFlags);
      _transitionPruneFlags.clear();
      _transitionPrunesStructure = false;
      if (acrossRanks) {
        // The transition's pack used the standing lists, so its messages
        // complete before the boundary derives anew without the pruned
        // entities, and the frozen slots that hold partner data move along.
        drainBoundaryExchange();
        const auto relocation = _traversal->rederiveRankBoundary(*_tree);
        containerRegistry.migratePoolsForTree(
          {},
          relocation.facePairs,
          relocation.vertexPairs,
          _traversal->getCellSlotSpaceSize(),
          _traversal->getFaceSlotSpaceSize() + _traversal->getNumberOfFaceRankFrozenSlots(),
          _traversal->getVertexSlotSpaceSize() + _traversal->getNumberOfVertexRankFrozenSlots()
        );
        peano4::communication::CommunicationManager::getInstance().rebuildTopology(_traversal->getNeighbourRanks());
        _skipStandingBoundaryFinish = true;
      }
      if (decomposesIntoTasks and not acrossRanks) {
        // The transition ran on its own decomposition, so the standing one
        // returns before the prune resolves which of its tasks went stale.
        if (_transitionRestoresDecompositionSnapshot) {
          _traversal->restoreSnapshottedDecomposition();
          _transitionRestoresDecompositionSnapshot = false;
        }
        if (_traversal->standingCutsMatchFreshSelection(*_tree, taskLevel)) {
          _traversal->retainRecordingsForPartialRerecord(
            _traversal->shrinkInterfaceTopologyForPrunedEntities(*_tree, pruneExtent)
          );
        } else {
          // A moved cut selection invalidates the recordings' task set, so the
          // decomposition re-derives and the next sweep re-records.
          _traversal->discardRecordingsForFullRerecord();
          _traversal->rederiveInterfaceTopology(*_tree, taskLevel);
        }
      }
    }
    const bool restoresDecompositionSnapshot = _transitionRestoresDecompositionSnapshot;
    if (restoresDecompositionSnapshot) {
      _traversal->restoreSnapshottedDecomposition();
      _transitionRestoresDecompositionSnapshot = false;
    }
    if (decomposesIntoTasks) {
      // A rank boundary that grew or shrank changes the interface entities
      // and their partner slots, so the interface topology derives anew and
      // the next sweep re-records every task.
      if (acrossRanks) {
        _traversal->discardRecordingsForFullRerecord();
        _traversal->rederiveInterfaceTopology(*_tree, taskLevel);
      }
      containerRegistry.resizePoolsForTree(
        _traversal->getCellSlotSpaceSize(),
        _traversal->getFaceSlotSpaceSize() + _traversal->getNumberOfFaceRankFrozenSlots()
          + _traversal->getNumberOfFaceShadowSlots(),
        _traversal->getVertexSlotSpaceSize() + _traversal->getNumberOfVertexRankFrozenSlots()
          + _traversal->getNumberOfVertexShadowSlots()
      );
      // A prune and a partitioned rank re-derive the whole decomposition, so
      // only the remaining in-place transitions restore the standing one.
      if (not restoresDecompositionSnapshot and not prunedStructure and not acrossRanks) {
        _traversal->restoreStandingTaskDecomposition(*_tree, taskLevel);
      }
      containerRegistry.replicateInterfaceSlots(
        _traversal->getInterfaceSeedFacePairs(),
        _traversal->getInterfaceSeedVertexPairs()
      );
      containerRegistry.replicateInterfaceSlots(
        _traversal->getInterfaceCopyToFrozenFacePairs(),
        _traversal->getInterfaceCopyToFrozenVertexPairs()
      );
      _traversal->commitPendingInterfaceFlushPairs();
    }
    _transitionOnNewStructure = false;
    _regridTransition.reset();
  } else if (ranTransitionSweep) {
    drainBoundaryExchange();
    // The transition's rank-boundary writes reach the partner ranks only with
    // this drain, and the hop below can delete a boundary's exchange channel,
    // so the partners' copies merge into the live entities before anything
    // migrates.
    _traversal->mergeTransitionRankBoundaryData(observerClone);
    waitForFinalInterfaceTopology();
    const bool repartitioned = not _pendingBlockOwners.empty();
    if (repartitioned) {
      _blockOwners = std::move(_pendingBlockOwners);
      _pendingBlockOwners.clear();
    }
    migrationHop(
      std::move(_preparedNewTree),
      std::move(_preparedNewTraversal),
      _regridTransition->unionToNewCellPairs,
      _regridTransition->unionToNewFacePairs,
      _regridTransition->unionToNewVertexPairs,
      repartitioned
    );
    // A repartition creates rank boundaries whose frozen slots hold no partner
    // data yet, so one synchronous exchange over the new topology refreshes
    // every frozen slot before the next sweep reads them.
    if (repartitioned and not _traversal->getNeighbourRanks().empty()) {
      // The refresh runs as its own exchange generation, so its records never
      // alias the transition sweep's completed ones.
      peano4::communication::CommunicationManager::getInstance().beginSweep();
      containerRegistry.packBoundarySlots(_traversal->getFacePackLists(), _traversal->getVertexPackLists());
      drainBoundaryExchange();
    }
    _regridTransition.reset();
    _skipStandingBoundaryFinish = true;
  }
}

const peano4::grid::GridStatistics& peano4::topology::TopologyManager::getGridStatistics() const {
  if (_traversal != nullptr) {
    return _traversal->getGridStatistics();
  }
  static const peano4::grid::GridStatistics emptyStatistics{};
  return emptyStatistics;
}

/**
 * Returns the cell load of the subtree tasks the standing sweeps decompose
 * into, empty while no tree stands or while the rank traverses serially.
 */
peano4::topology::TreeTraversal::SubtreeTaskLoad peano4::topology::TopologyManager::getSubtreeTaskLoad() const {
  return _traversal != nullptr ? _traversal->getSubtreeTaskLoad() : peano4::topology::TreeTraversal::SubtreeTaskLoad{};
}

/** Returns the task time all sweeps of this rank have spent since the run began. */
double peano4::topology::TopologyManager::getAccumulatedTaskSeconds() const { return _accumulatedTaskSeconds; }

const tarch::la::Vector<DIMENSIONS, double>& peano4::topology::TopologyManager::getDomainOffset() const {
  return _domainOffset;
}

const tarch::la::Vector<DIMENSIONS, double>& peano4::topology::TopologyManager::getDomainWidth() const {
  return _domainWidth;
}

const std::bitset<DIMENSIONS>& peano4::topology::TopologyManager::getPeriodicBC() const { return _periodicBC; }
