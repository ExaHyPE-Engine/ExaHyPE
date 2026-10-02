// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Spacetree.h"

#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "peano4/Regrid.h"
#include "peano4/sfc/ActiveCurve.h"
#include "peano4/topology/ChunkedTasks.h"
#include "tarch/Assertions.h"
#include "tarch/MemoryUsage.h"
#include "tarch/Termination.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/tasking/Runtime.h"

#include <mpi.h>

namespace {
  /**
   * Compares two refinement plans flag by flag. Every plan is canonical, its flags sorted by
   * lattice key on each level, so equal levels compare equal element by element. A plan may carry
   * trailing levels the other one has not reached, and those hold no flags.
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

  tarch::la::Vector<DIMENSIONS, int> getParentLatticeOf(const tarch::la::Vector<DIMENSIONS, int>& lattice) {
    tarch::la::Vector<DIMENSIONS, int> result(0);
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      result(axis) = lattice(axis) / peano4::SubdivisionFactor;
    }
    return result;
  }

  tarch::la::Vector<DIMENSIONS, int> getChildLatticeOf(
    const tarch::la::Vector<DIMENSIONS, int>& parentLattice,
    const tarch::la::Vector<DIMENSIONS, int>& position
  ) {
    tarch::la::Vector<DIMENSIONS, int> result(0);
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      result(axis) = parentLattice(axis) * peano4::SubdivisionFactor + position(axis);
    }
    return result;
  }

  int getChildPositionOf(const tarch::la::Vector<DIMENSIONS, int>& lattice) {
    tarch::la::Vector<DIMENSIONS, int> position(0);
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      position(axis) = lattice(axis) % peano4::SubdivisionFactor;
    }
    return peano4::dLinearised(position, peano4::SubdivisionFactor);
  }

  /**
   * Returns the index of a child face among the subfaces of its parent face, ordered along the tangential axes.
   */
  int getSubfaceIndexOf(const tarch::la::Vector<DIMENSIONS, int>& childPosition, const int normal) {
    return peano4::dLinearisedExclusive(childPosition, peano4::SubdivisionFactor, normal);
  }

  int getSideOfCellOnFace(const int faceNumber) { return faceNumber < DIMENSIONS ? 1 : 0; }

  /**
   * Returns whether the cell is refined in the structure the plan describes. The plan tables
   * start at the tree's base depth, and every cell above the base depth is refined.
   */
  bool isRefinedIn(
    const peano4::topology::Tree&             tree,
    const int                                 level,
    const tarch::la::Vector<DIMENSIONS, int>& lattice
  ) {
    return level < tree.getDepth() or tree.isCellRefinedInPlan(level, lattice);
  }

  bool isLeafIn(
    const peano4::topology::Tree&             tree,
    const int                                 level,
    const tarch::la::Vector<DIMENSIONS, int>& lattice
  ) {
    return tree.cellExistsInPlan(level, lattice) and not isRefinedIn(tree, level, lattice);
  }

  int getOppositeFaceNumber(const int faceNumber) {
    return faceNumber < DIMENSIONS ? faceNumber + DIMENSIONS : faceNumber - DIMENSIONS;
  }

  struct TransferKey final {
    int          level{0};
    std::int64_t lattice{0};

    bool operator<(const TransferKey& other) const noexcept {
      return level != other.level ? level < other.level : lattice < other.lattice;
    }
  };

  /**
   * Turns the counts per key into the offsets of the ranges, with one end entry after the last key.
   */
  void turnCountsIntoOffsets(std::vector<int>& ranges) {
    int offset = 0;
    for (int& entry : ranges) {
      const int count = entry;
      entry           = offset;
      offset += count;
    }
  }
} // namespace

static_assert(std::is_standard_layout_v<peano4::Spacetree::Statistics>);
static_assert(std::is_trivially_copyable_v<peano4::Spacetree::Statistics>);
static_assert(std::is_trivially_copyable_v<peano4::Partition>);

peano4::Spacetree peano4::Spacetree::_singleton;

peano4::Spacetree&
peano4::Spacetree::getInstance() {
  return _singleton;
}

void
peano4::Spacetree::init(
  const tarch::la::Vector<DIMENSIONS, double>& offset,
  const tarch::la::Vector<DIMENSIONS, double>& width,
  const std::bitset<DIMENSIONS>&               periodicBoundaries
) {
  _domainOffset       = offset;
  _domainWidth        = width;
  _periodicBoundaries = periodicBoundaries;
}

/**
 * Releases the tree and the partition and forgets the plan, so the next init() starts a new grid.
 */
void
peano4::Spacetree::shutdown() {
  _targetLevel = -1;
  _tree.reset();
  _spareTrees.clear();
  _refinementPlan.clear();
  _blockOwners.clear();
  _lastEvaluatedRequests.clear();
  _lastEvaluatedPlanUpdate.clear();
  _lastEvaluationChangedPlan = false;
  _persistentDataLive        = false;
  _persistentDataLiveUniform = false;
  _dynamicLoadBalancing      = true;
  _regridPending             = false;
  _arrays                    = PartitionArrays{};
  _deviceArrays              = DeviceArrays{};
  _partition                 = Partition{};
  _partitionOnDevice         = Partition{};
  _statistics                = Statistics{};
}

/**
 * Schedules the construction of the regular base grid. Every rank calls this with the same target
 * level, so the partition follows from the rank count alone and needs no communication.
 */
void
peano4::Spacetree::scheduleRegularGridConstruction(const int targetLevel) {
  _targetLevel = targetLevel;
}

/**
 * Records that the pools of the caller hold live data. A regrid then keeps the slots of the
 * surviving cells and returns the slot moves a caller realises on its pools.
 */
void
peano4::Spacetree::markPersistentDataLive() {
  _persistentDataLive = true;
}

/**
 * Allows or forbids a regrid to move a block to another rank. The grid construction derives the
 * decomposition either way, so a run that forbids the move keeps that first decomposition.
 */
void
peano4::Spacetree::enableDynamicLoadBalancing(const bool enable) {
  _dynamicLoadBalancing = enable;
}

/**
 * Returns the coarsest level whose blocks give every rank at least one block.
 */
int
peano4::Spacetree::deriveSpawnLevel(const int targetLevel, const int numberOfRanks) {
  return peano4::topology::Tree::deriveSpawnLevel(targetLevel, numberOfRanks);
}

int
peano4::Spacetree::deriveSpawnLevel() const {
  return deriveSpawnLevel(_targetLevel, tarch::mpi::Rank::getInstance().getNumberOfRanks());
}

/**
 * Returns a spare tree to rebuild into, or a new one when no spare is left.
 */
std::unique_ptr<peano4::topology::Tree>
peano4::Spacetree::acquireTree() {
  if (not _spareTrees.empty()) {
    auto spareTree = std::move(_spareTrees.back());
    _spareTrees.pop_back();
    return spareTree;
  }
  const auto rank = tarch::mpi::Rank::getInstance().getRank();
  return std::make_unique<peano4::topology::Tree>(rank, _domainOffset, _domainWidth, _periodicBoundaries);
}

const peano4::Partition&
peano4::Spacetree::getPartition() const noexcept {
  return _partition;
}

const peano4::Spacetree::Statistics&
peano4::Spacetree::getStatistics() const noexcept {
  return _statistics;
}

const std::bitset<DIMENSIONS>&
peano4::Spacetree::getPeriodicBoundaries() const noexcept {
  return _periodicBoundaries;
}

/**
 * Returns whether the requests change the refinement plan, so a caller skips a regrid over a set
 * the plan rejects. Before the tree stands every non-empty set counts as a change.
 */
bool
peano4::Spacetree::requestsChangePlan(const std::vector<RefinementRequest>& requests) {
  if (_tree == nullptr) {
    return not requests.empty();
  }
  peano4::topology::Tree::RefinementPlan updatedPlan;
  return derivePlanUpdate(requests, updatedPlan);
}

/**
 * Evaluates the requests against the standing plan and returns whether they change it;
 * updatedPlan then holds the changed plan. The last evaluated set and its result are cached, so a
 * repeated set answers from the cache.
 */
bool
peano4::Spacetree::derivePlanUpdate(
  const std::vector<RefinementRequest>&   requests,
  peano4::topology::Tree::RefinementPlan& updatedPlan
) {
  assertion(_tree != nullptr);
  if (requests.empty()) {
    return false;
  }
  if (requests == _lastEvaluatedRequests) {
    updatedPlan = _lastEvaluatedPlanUpdate;
    return _lastEvaluationChangedPlan;
  }
  updatedPlan = _refinementPlan;
  const bool planChanged =
    _tree->updateRefinementPlan(requests, updatedPlan) and not refinementPlansMatch(updatedPlan, _refinementPlan);
  _lastEvaluatedRequests     = requests;
  _lastEvaluatedPlanUpdate   = updatedPlan;
  _lastEvaluationChangedPlan = planChanged;
  return planChanged;
}

/**
 * Builds the tree from the target level and the plan. Without a standing decomposition the build
 * derives the balanced one from the plan, so every rank computes the identical table without
 * communication.
 */
void
peano4::Spacetree::buildTree(const peano4::topology::Tree::RefinementPlan& refinementPlan) {
  const auto numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  assertion(_targetLevel >= 0);
  const int spawnLevel = deriveSpawnLevel();
  auto      tree       = acquireTree();

  std::int64_t numberOfBlocks = 1;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    numberOfBlocks *= peano4::topology::getCellsPerAxis(spawnLevel);
  }
  const bool balancesCreationalGrid =
    numberOfRanks > 1 and spawnLevel < _targetLevel and (not refinementPlan.empty() or numberOfBlocks < numberOfRanks);
  if (balancesCreationalGrid) {
    _blockOwners = tree->deriveBalancedBlockOwners(_targetLevel, numberOfRanks, refinementPlan);
  }
  tree->buildRegularGrid(
    _targetLevel,
    spawnLevel,
    numberOfRanks,
    refinementPlan,
    _blockOwners.empty() ? nullptr : &_blockOwners
  );
  if (_blockOwners.empty()) {
    _blockOwners = tree->getOwnersOfBlocks();
  }
  if (_tree != nullptr) {
    _spareTrees.push_back(std::move(_tree));
  }
  _tree = std::move(tree);
}

/**
 * Returns the block owners the updated plan gets: The standing ones while they split no sibling
 * cells and their largest rank load stays within the tolerance of the balanced ones, else the
 * balanced ones. An empty result says the standing owners stay.
 */
std::vector<int>
peano4::Spacetree::deriveOwnersForPlan(const peano4::topology::Tree::RefinementPlan& updatedPlan) const {
  const auto numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  if (numberOfRanks <= 1 or _blockOwners.empty()) {
    return {};
  }
  const int spawnLevel = deriveSpawnLevel();
  // Standing owners that split a cell with a leaf child under the new plan separate sibling cells
  // that coarsen together or the fine cells that restrict onto one coarse face, so the owners
  // derive anew.
  const bool standingOwnersSplitSiblingCells =
    _tree->ownershipPlanSplitsCellWithLeafChild(_targetLevel, spawnLevel, updatedPlan, _blockOwners);
  if (not _dynamicLoadBalancing and not standingOwnersSplitSiblingCells) {
    return {};
  }
  auto balancedOwners = _tree->deriveBalancedBlockOwners(_targetLevel, numberOfRanks, updatedPlan);
  if (balancedOwners == _blockOwners) {
    return {};
  }
  if (standingOwnersSplitSiblingCells) {
    return balancedOwners;
  }
  const auto largestLoadOf = [](const std::vector<std::int64_t>& loads) {
    return *std::max_element(loads.begin(), loads.end());
  };
  const auto standingLoad =
    largestLoadOf(_tree->deriveRankLoads(_targetLevel, numberOfRanks, updatedPlan, _blockOwners));
  const auto balancedLoad =
    largestLoadOf(_tree->deriveRankLoads(_targetLevel, numberOfRanks, updatedPlan, balancedOwners));
  if (standingLoad * 100 <= balancedLoad * (100 + RepartitionTolerancePercent)) {
    return {};
  }
  return balancedOwners;
}

/**
 * Sets the live marker on every rank once one rank has set it, so every rank takes the same regrid
 * branch. A caller may raise the marker on the global master alone.
 */
void
peano4::Spacetree::votePersistentDataLive() {
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() <= 1 or _persistentDataLiveUniform) {
    return;
  }
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

/**
 * Applies the requests to the grid and returns whether the partition changed; the first call
 * builds the scheduled base grid. Before the pools hold live data a changed plan numbers the
 * partition afresh; afterwards the surviving cells keep their slots and regrid names the moves a
 * caller realises on its pools before finishRegrid().
 */
bool
peano4::Spacetree::regrid(const std::vector<RefinementRequest>& requests, Regrid& regrid) {
  assertion(not _regridPending, "finishRegrid() has to follow every regrid that returned the slot moves.");
  votePersistentDataLive();

  if (_tree == nullptr) {
    buildTree(_refinementPlan);
    buildPartition();
    _statistics.numberOfStationaryRegrids = 0;
    deriveStatistics();
    return true;
  }

  peano4::topology::Tree::RefinementPlan updatedPlan;
  if (not derivePlanUpdate(requests, updatedPlan)) {
    _statistics.numberOfStationaryRegrids++;
    return false;
  }
  _lastEvaluatedRequests.clear();
  _lastEvaluatedPlanUpdate.clear();
  _lastEvaluationChangedPlan            = false;
  _statistics.numberOfStationaryRegrids = 0;

  if (not _persistentDataLive) {
    _refinementPlan = std::move(updatedPlan);
    buildTree(_refinementPlan);
    buildPartition();
    deriveStatistics();
    return true;
  }

  const auto        numberOfRanks  = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  const int         spawnLevel     = deriveSpawnLevel();
  auto              balancedOwners = deriveOwnersForPlan(updatedPlan);
  const auto* const owners         = balancedOwners.empty() ? &_blockOwners : &balancedOwners;
  auto              newTree        = acquireTree();
  newTree->buildRegularGrid(_targetLevel, spawnLevel, numberOfRanks, updatedPlan, owners);

  std::swap(_tree, newTree);
  rebuildPartition(*newTree, regrid);
  _spareTrees.push_back(std::move(newTree));
  _refinementPlan = std::move(updatedPlan);
  if (not balancedOwners.empty()) {
    _blockOwners = std::move(balancedOwners);
  }
  _regridPending = true;
  deriveStatistics();
  return true;
}

/**
 * Closes a regrid once the caller has moved its data: The freed slots join the free list.
 */
void
peano4::Spacetree::finishRegrid() {
  assertion(_regridPending);
  _arrays.freeSlots
    .insert(_arrays.freeSlots.end(), _arrays.slotsFreedByRegrid.begin(), _arrays.slotsFreedByRegrid.end());
  _arrays.slotsFreedByRegrid.clear();
  _regridPending = false;
}

/**
 * Derives the rank's statistics from the partition and the tree. A refined cell counts for the
 * rank that owns it, so the ranks together count it once. A rank without cells reports the
 * identities of the reductions over the ranks as its minimum level and minimum cell width.
 */
void
peano4::Spacetree::deriveStatistics() {
  const auto rank                  = tarch::mpi::Rank::getInstance().getRank();
  _statistics.maximumLevel         = _arrays.maximumLevel;
  _statistics.minimumLevel         = _arrays.cells.empty() ? std::numeric_limits<int>::max() : _arrays.minimumLevel;
  _statistics.numberOfCells        = static_cast<int>(_arrays.cells.size());
  _statistics.numberOfRefinedCells = 0;
  for (std::size_t cellIndex = 0; cellIndex < _tree->getNumberOfCells(); cellIndex++) {
    const auto& cell = _tree->getCell(static_cast<int>(cellIndex));
    if (cell.childrenBlock >= 0 and cell.ownerTreeId == rank) {
      _statistics.numberOfRefinedCells++;
    }
  }
  _statistics.numberOfFaces        = _arrays.numberOfFaces;
  _statistics.numberOfHangingFaces = _partition.getFaces(Dependence::Local, FaceKind::Hanging).size
    + _partition.getFaces(Dependence::Remote, FaceKind::Hanging).size;
  _statistics.numberOfExchangedFaces = static_cast<int>(_arrays.exchangedFaces.size());
  _statistics.partitionBytes         = getPartitionBytes();
  _statistics.treeBytes              = _tree->getAllocatedBytes();
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    _statistics.minimumCellWidth(axis) = _arrays.cells.empty() ? std::numeric_limits<double>::max()
                                                               : _partition.getWidthOfLevel(_arrays.maximumLevel, axis);
  }
}

std::int64_t
peano4::Spacetree::getLatticeKey(
  [[maybe_unused]] const int                level,
  const tarch::la::Vector<DIMENSIONS, int>& lattice
) const {
  assertion(level >= 0, level, lattice);
  return peano4::topology::Tree::serialiseLatticeKey(lattice);
}

/**
 * Returns the slot of the cell at the level and lattice position, or -1 when this rank owns no such cell.
 */
int
peano4::Spacetree::getCellAt(const int level, const tarch::la::Vector<DIMENSIONS, int>& lattice) const {
  if (level < 0 or level >= static_cast<int>(_arrays.cellSlotOfLattice.size())) {
    return -1;
  }
  const int slot = _arrays.cellSlotOfLattice[static_cast<std::size_t>(level)].find(getLatticeKey(level, lattice));
  return slot == peano4::grid::LatticeIndexMap::NoEntry ? -1 : slot;
}

/**
 * Numbers the partition of the tree afresh; the slots of an earlier partition are forgotten.
 */
void
peano4::Spacetree::buildPartition() {
  assertion(_arrays.slotsFreedByRegrid.empty());
  _arrays.numberOfSlots = 0;
  _arrays.freeSlots.clear();
  _arrays.cellSlotOfLattice.clear();

  std::vector<CellRecord> records;
  collectCells(records);
  assignSlots(records, false);
  deriveFaces(records);
  orderLists(records);
  numberCellsAlongCurve(records);
  assemblePartitions();
}

/**
 * Rebuilds the partition over the changed tree and derives the slot moves. The old lookup tables
 * stay alive until the moves are derived, so a new cell finds the old slot of its parent or its
 * children; finishRegrid() releases the freed slots afterwards.
 */
void
peano4::Spacetree::rebuildPartition(const peano4::topology::Tree& previousTree, Regrid& regrid) {
  assertion(_arrays.slotsFreedByRegrid.empty());
  assertion(_periodicBoundaries == _tree->getPeriodicBoundaries());
  const int rank = tarch::mpi::Rank::getInstance().getRank();

  const auto oldCells         = _arrays.cells;
  const auto oldCellLevel     = _arrays.cellLevel;
  const auto oldCellLattice   = _arrays.cellLattice;
  auto       oldLocator       = std::move(_arrays.cellSlotOfLattice);
  const int  oldNumberOfSlots = _arrays.numberOfSlots;
  _arrays.cellSlotOfLattice.clear();

  std::vector<CellRecord> records;
  collectCells(records);
  // The old lookup answers the survivors; the new one takes over once every slot is assigned.
  std::swap(_arrays.cellSlotOfLattice, oldLocator);
  assignSlots(records, true);
  std::swap(_arrays.cellSlotOfLattice, oldLocator);

  {
    std::vector<char> survives(static_cast<std::size_t>(oldNumberOfSlots), 0);
    for (const auto& record : records) {
      if (record.slot < oldNumberOfSlots) {
        survives[static_cast<std::size_t>(record.slot)] = 1;
      }
    }
    for (const int oldSlot : oldCells) {
      if (survives[static_cast<std::size_t>(oldSlot)] == 0) {
        _arrays.slotsFreedByRegrid.push_back(oldSlot);
      }
    }
  }

  // The partition still points at the arrays of the old grid, so the new lattices come from the arrays.
  const auto newLatticeOf = [this](const int slot) {
    tarch::la::Vector<DIMENSIONS, int> result(0);
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      result(axis) = _arrays.cellLattice[static_cast<std::size_t>(slot) * DIMENSIONS + axis];
    }
    return result;
  };
  std::vector<peano4::grid::LatticeIndexMap> newLocator(static_cast<std::size_t>(_arrays.maximumLevel) + 1);
  {
    std::vector<std::size_t> cellsPerLevel(static_cast<std::size_t>(_arrays.maximumLevel) + 1, 0);
    for (const auto& record : records) {
      cellsPerLevel[static_cast<std::size_t>(_arrays.cellLevel[static_cast<std::size_t>(record.slot)])]++;
    }
    for (std::size_t level = 0; level < newLocator.size(); level++) {
      newLocator[level].reserve(cellsPerLevel[level]);
    }
    for (const auto& record : records) {
      const int level = _arrays.cellLevel[static_cast<std::size_t>(record.slot)];
      newLocator[static_cast<std::size_t>(level)].insert(getLatticeKey(level, newLatticeOf(record.slot)), record.slot);
    }
  }
  _arrays.cellSlotOfLattice = std::move(newLocator);

  deriveFaces(records);
  orderLists(records);
  numberCellsAlongCurve(records);
  assemblePartitions();

  regrid                  = Regrid{};
  regrid.oldNumberOfSlots = oldNumberOfSlots;
  regrid.newNumberOfSlots = _arrays.numberOfSlots;
  regrid.freedSlots       = _arrays.slotsFreedByRegrid;

  const auto lookupOld = [&oldLocator, this](const int level, const tarch::la::Vector<DIMENSIONS, int>& lattice) {
    if (level < 0 or level >= static_cast<int>(oldLocator.size())) {
      return -1;
    }
    const int slot = oldLocator[static_cast<std::size_t>(level)].find(getLatticeKey(level, lattice));
    return slot == peano4::grid::LatticeIndexMap::NoEntry ? -1 : slot;
  };
  const auto oldLatticeOf = [&oldCellLattice](const int oldSlot) {
    tarch::la::Vector<DIMENSIONS, int> result(0);
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      result(axis) = oldCellLattice[static_cast<std::size_t>(oldSlot) * DIMENSIONS + axis];
    }
    return result;
  };
  const auto beyondOneLevel = []() {
    tarch::terminateWithError(
      "A regrid changed the level of a cell by more than one, which the slot moves do not carry."
    );
  };

  std::map<int, std::vector<std::pair<TransferKey, TransferredCell>>> sent;
  std::map<int, std::vector<std::pair<TransferKey, int>>>             received;

  for (const auto& record : records) {
    const int  slot    = record.slot;
    const int  level   = _arrays.cellLevel[static_cast<std::size_t>(slot)];
    const auto lattice = newLatticeOf(slot);
    if (lookupOld(level, lattice) >= 0) {
      continue;
    }
    const auto parentLattice = getParentLatticeOf(lattice);
    const int  parentLevel   = level - 1;
    const bool parentWasLeaf = parentLevel >= 0 and isLeafIn(previousTree, parentLevel, parentLattice);
    if (parentWasLeaf) {
      regrid.createdSlots.push_back(slot);
      const int owner = previousTree.getOwnerOfCellAt(parentLevel, parentLattice);
      if (owner == rank) {
        const int parentSlot = lookupOld(parentLevel, parentLattice);
        assertion(parentSlot >= 0, level, lattice);
        regrid.prolongations.push_back({parentSlot, slot, getChildPositionOf(lattice)});
      } else {
        received[owner].emplace_back(TransferKey{level, getLatticeKey(level, lattice)}, slot);
      }
      continue;
    }
    if (previousTree.cellExistsInPlan(level, lattice)) {
      if (isRefinedIn(previousTree, level, lattice)) {
        const auto firstChild = getChildLatticeOf(lattice, peano4::dDelinearised(0, peano4::SubdivisionFactor));
        if (isRefinedIn(previousTree, level + 1, firstChild)) {
          beyondOneLevel();
        }
        regrid.createdSlots.push_back(slot);
        // The ownership plans never split a cell with a leaf child, so every child has the owner of the first one.
        const int owner = previousTree.getOwnerOfCellAt(level + 1, firstChild);
        for (int child = 1; child < peano4::ChildrenPerCell; child++) {
          [[maybe_unused]] const auto childLattice =
            getChildLatticeOf(lattice, peano4::dDelinearised(child, peano4::SubdivisionFactor));
          assertion(previousTree.getOwnerOfCellAt(level + 1, childLattice) == owner, level, lattice, child, owner);
        }
        if (owner == rank) {
          Restriction restriction;
          restriction.parentSlot = slot;
          for (int child = 0; child < peano4::ChildrenPerCell; child++) {
            const int childSlot =
              lookupOld(level + 1, getChildLatticeOf(lattice, peano4::dDelinearised(child, peano4::SubdivisionFactor)));
            assertion(childSlot >= 0, level, lattice, child);
            restriction.childSlots[child] = childSlot;
          }
          regrid.restrictions.push_back(restriction);
        } else {
          received[owner].emplace_back(TransferKey{level, getLatticeKey(level, lattice)}, slot);
        }
      } else {
        const int owner = previousTree.getOwnerOfCellAt(level, lattice);
        assertion(owner != rank, level, lattice);
        received[owner].emplace_back(TransferKey{level, getLatticeKey(level, lattice)}, slot);
      }
      continue;
    }
    beyondOneLevel();
  }

  for (const int oldSlot : oldCells) {
    const int  level   = oldCellLevel[static_cast<std::size_t>(oldSlot)];
    const auto lattice = oldLatticeOf(oldSlot);
    if (getCellAt(level, lattice) >= 0) {
      continue;
    }
    if (_tree->cellExistsInPlan(level, lattice)) {
      if (isRefinedIn(*_tree, level, lattice)) {
        for (int child = 0; child < peano4::ChildrenPerCell; child++) {
          const auto childLattice = getChildLatticeOf(lattice, peano4::dDelinearised(child, peano4::SubdivisionFactor));
          if (isRefinedIn(*_tree, level + 1, childLattice)) {
            beyondOneLevel();
          }
          const int owner = _tree->getOwnerOfCellAt(level + 1, childLattice);
          if (owner != rank) {
            TransferredCell cell;
            cell.parentSlot    = oldSlot;
            cell.childPosition = child;
            sent[owner].emplace_back(TransferKey{level + 1, getLatticeKey(level + 1, childLattice)}, cell);
          }
        }
      } else {
        const int owner = _tree->getOwnerOfCellAt(level, lattice);
        assertion(owner != rank, level, lattice);
        TransferredCell cell;
        cell.keptSlot = oldSlot;
        sent[owner].emplace_back(TransferKey{level, getLatticeKey(level, lattice)}, cell);
      }
      continue;
    }
    const auto parentLattice = getParentLatticeOf(lattice);
    if (level == 0 or not _tree->cellExistsInPlan(level - 1, parentLattice)) {
      beyondOneLevel();
    }
    const int owner = _tree->getOwnerOfCellAt(level - 1, parentLattice);
    if (owner != rank and getChildPositionOf(lattice) == 0) {
      TransferredCell cell;
      for (int child = 0; child < peano4::ChildrenPerCell; child++) {
        const int childSlot =
          lookupOld(level, getChildLatticeOf(parentLattice, peano4::dDelinearised(child, peano4::SubdivisionFactor)));
        assertion(childSlot >= 0, level, lattice, child);
        cell.childSlots[child] = childSlot;
      }
      sent[owner].emplace_back(TransferKey{level - 1, getLatticeKey(level - 1, parentLattice)}, cell);
    }
  }

  std::set<int> partnerRanks;
  for (const auto& [partner, cells] : sent) {
    partnerRanks.insert(partner);
  }
  for (const auto& [partner, slots] : received) {
    partnerRanks.insert(partner);
  }
  for (const int partner : partnerRanks) {
    RankTransfer transfer;
    transfer.rank = partner;
    if (auto entry = sent.find(partner); entry != sent.end()) {
      auto& cells = entry->second;
      std::sort(cells.begin(), cells.end(), [](const auto& left, const auto& right) {
        return left.first < right.first;
      });
      transfer.sentCells.reserve(cells.size());
      for (const auto& [key, cell] : cells) {
        transfer.sentCells.push_back(cell);
      }
    }
    if (auto entry = received.find(partner); entry != received.end()) {
      auto& slots = entry->second;
      std::sort(slots.begin(), slots.end(), [](const auto& left, const auto& right) {
        return left.first < right.first;
      });
      transfer.receivedSlots.reserve(slots.size());
      for (const auto& [key, slot] : slots) {
        transfer.receivedSlots.push_back(slot);
      }
    }
    regrid.rankTransfers.push_back(std::move(transfer));
  }
}

/**
 * Returns the bytes of the host arrays and lattice maps as host bytes and the bytes of every
 * device array in its location.
 */
tarch::AllocatedBytes
peano4::Spacetree::getPartitionBytes() const noexcept {
  std::size_t hostBytes = tarch::getAllocatedBytesOf(_arrays.widthOfLevel) + tarch::getAllocatedBytesOf(_arrays.cells)
    + tarch::getAllocatedBytesOf(_arrays.cellRanges) + tarch::getAllocatedBytesOf(_arrays.cellLevel)
    + tarch::getAllocatedBytesOf(_arrays.cellLattice) + tarch::getAllocatedBytesOf(_arrays.facesOfCell)
    + tarch::getAllocatedBytesOf(_arrays.cellCurveIndex) + tarch::getAllocatedBytesOf(_arrays.faces)
    + tarch::getAllocatedBytesOf(_arrays.faceRanges) + tarch::getAllocatedBytesOf(_arrays.faceLevel)
    + tarch::getAllocatedBytesOf(_arrays.faceNormal) + tarch::getAllocatedBytesOf(_arrays.faceLattice)
    + tarch::getAllocatedBytesOf(_arrays.cellsOfFace) + tarch::getAllocatedBytesOf(_arrays.faceKind)
    + tarch::getAllocatedBytesOf(_arrays.faceDependsOnRemoteData) + tarch::getAllocatedBytesOf(_arrays.levelJumps)
    + tarch::getAllocatedBytesOf(_arrays.levelJumpRanges) + tarch::getAllocatedBytesOf(_arrays.neighbours)
    + tarch::getAllocatedBytesOf(_arrays.exchangedFaces) + tarch::getAllocatedBytesOf(_arrays.faceOfTreeFace)
    + tarch::getAllocatedBytesOf(_arrays.levelJumpOfCoarseFace) + tarch::getAllocatedBytesOf(_arrays.cellSlotOfLattice)
    + tarch::getAllocatedBytesOf(_arrays.freeSlots) + tarch::getAllocatedBytesOf(_arrays.slotsFreedByRegrid);
  for (const auto& cellSlotOfLattice : _arrays.cellSlotOfLattice) {
    hostBytes += cellSlotOfLattice.getAllocatedBytes();
  }
  return tarch::AllocatedBytes::onHost(hostBytes) + _deviceArrays.cells.getAllocatedBytes()
    + _deviceArrays.cellLevel.getAllocatedBytes() + _deviceArrays.cellLattice.getAllocatedBytes()
    + _deviceArrays.facesOfCell.getAllocatedBytes() + _deviceArrays.faces.getAllocatedBytes()
    + _deviceArrays.faceLevel.getAllocatedBytes() + _deviceArrays.faceNormal.getAllocatedBytes()
    + _deviceArrays.faceLattice.getAllocatedBytes() + _deviceArrays.cellsOfFace.getAllocatedBytes()
    + _deviceArrays.faceKind.getAllocatedBytes() + _deviceArrays.levelJumps.getAllocatedBytes()
    + _deviceArrays.widthOfLevel.getAllocatedBytes();
}

/**
 * Walks the tree along the curve and records every cell this rank owns. The walk descends
 * serially until it holds enough subtree roots for the threads, then one task per root walks
 * its subtree, and the per-root lists concatenate in curve order.
 */
void
peano4::Spacetree::collectCells(std::vector<CellRecord>& records) const {
  struct Root final {
    int          cell{-1};
    std::uint8_t curveState{0};
  };

  const auto& tree            = *_tree;
  const int   rank            = tarch::mpi::Rank::getInstance().getRank();
  const int   numberOfThreads = std::max(1, tarch::tasking::Runtime::getInstance().getNumberOfThreads());
  const int   rootsWanted     = 4 * numberOfThreads;

  std::vector<Root> roots{{peano4::topology::Tree::RootCell, peano4::sfc::ActiveCurve::RootState}};
  bool              anyRefinedRoot = tree.isRefined(peano4::topology::Tree::RootCell);
  while (anyRefinedRoot and static_cast<int>(roots.size()) < rootsWanted) {
    std::vector<Root> expanded;
    expanded.reserve(roots.size() * peano4::ChildrenPerCell);
    anyRefinedRoot = false;
    for (const auto& root : roots) {
      if (not tree.isRefined(root.cell) or not tree.subtreeTouchesRank(root.cell, rank)) {
        expanded.push_back(root);
        continue;
      }
      for (const auto& child : peano4::sfc::ActiveCurve::getChildren(root.curveState)) {
        const int childCell = tree.getChild(root.cell, child.linearisedPosition);
        expanded.push_back({childCell, child.state});
        anyRefinedRoot = anyRefinedRoot or tree.isRefined(childCell);
      }
    }
    roots = std::move(expanded);
  }

  std::vector<std::vector<int>> cellsOfRoot(roots.size());
  const auto                    descend =
    [&tree, rank](const auto& recurse, const int cell, const std::uint8_t curveState, std::vector<int>& cells) -> void {
    if (not tree.subtreeTouchesRank(cell, rank)) {
      return;
    }
    if (tree.isRefined(cell)) {
      for (const auto& child : peano4::sfc::ActiveCurve::getChildren(curveState)) {
        recurse(recurse, tree.getChild(cell, child.linearisedPosition), child.state, cells);
      }
    } else if (tree.getCell(cell).ownerTreeId == rank) {
      cells.push_back(cell);
    }
  };
  peano4::topology::runChunkedTasks(
    static_cast<int>(roots.size()),
    [&roots, &cellsOfRoot, &descend](const int chunkNumber, const int begin, const int end) {
      static_cast<void>(chunkNumber);
      for (int rootNumber = begin; rootNumber < end; rootNumber++) {
        descend(
          descend,
          roots[static_cast<std::size_t>(rootNumber)].cell,
          roots[static_cast<std::size_t>(rootNumber)].curveState,
          cellsOfRoot[static_cast<std::size_t>(rootNumber)]
        );
      }
    },
    1
  );

  std::size_t numberOfCells = 0;
  for (const auto& cells : cellsOfRoot) {
    numberOfCells += cells.size();
  }
  records.clear();
  records.reserve(numberOfCells);
  for (const auto& cells : cellsOfRoot) {
    for (const int cell : cells) {
      records.push_back({cell, -1});
    }
  }
}

/**
 * Gives every cell its slot. With preserveSlots a cell the lookup tables know keeps its slot,
 * every other cell takes a free slot and then the next slot beyond the range. The per-slot
 * arrays and the widths follow from the assignment.
 */
void
peano4::Spacetree::assignSlots(std::vector<CellRecord>& records, const bool preserveSlots) {
  const auto& tree = *_tree;
  for (auto& record : records) {
    const auto& cell = tree.getCell(record.treeCell);
    record.slot      = preserveSlots ? getCellAt(cell.level, cell.latticeIndex) : -1;
  }
  for (auto& record : records) {
    if (record.slot >= 0) {
      continue;
    }
    if (_arrays.freeSlots.empty()) {
      record.slot = _arrays.numberOfSlots++;
    } else {
      record.slot = _arrays.freeSlots.back();
      _arrays.freeSlots.pop_back();
    }
  }

  _arrays.cellLevel.assign(static_cast<std::size_t>(_arrays.numberOfSlots), -1);
  _arrays.cellLattice.assign(static_cast<std::size_t>(_arrays.numberOfSlots) * DIMENSIONS, 0);
  _arrays.facesOfCell.assign(static_cast<std::size_t>(_arrays.numberOfSlots) * peano4::FacesPerCell, -1);
  _arrays.minimumLevel = std::numeric_limits<int>::max();
  _arrays.maximumLevel = 0;
  for (const auto& record : records) {
    const auto& cell                                         = tree.getCell(record.treeCell);
    _arrays.cellLevel[static_cast<std::size_t>(record.slot)] = cell.level;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      _arrays.cellLattice[static_cast<std::size_t>(record.slot) * DIMENSIONS + axis] = cell.latticeIndex(axis);
    }
    _arrays.minimumLevel = std::min(_arrays.minimumLevel, cell.level);
    _arrays.maximumLevel = std::max(_arrays.maximumLevel, cell.level);
  }
  if (records.empty()) {
    _arrays.minimumLevel = 0;
  }

  _arrays.widthOfLevel.assign((static_cast<std::size_t>(_arrays.maximumLevel) + 1) * DIMENSIONS, 0.0);
  for (int level = 0; level <= _arrays.maximumLevel; level++) {
    const auto cellsPerAxis = static_cast<double>(peano4::topology::getCellsPerAxis(level));
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      _arrays.widthOfLevel[static_cast<std::size_t>(level) * DIMENSIONS + axis] = _domainWidth(axis) / cellsPerAxis;
    }
  }

  if (not preserveSlots) {
    std::vector<std::size_t> cellsPerLevel(static_cast<std::size_t>(_arrays.maximumLevel) + 1, 0);
    for (const auto& record : records) {
      cellsPerLevel[static_cast<std::size_t>(_arrays.cellLevel[static_cast<std::size_t>(record.slot)])]++;
    }
    _arrays.cellSlotOfLattice.assign(static_cast<std::size_t>(_arrays.maximumLevel) + 1, {});
    for (std::size_t level = 0; level < _arrays.cellSlotOfLattice.size(); level++) {
      _arrays.cellSlotOfLattice[level].reserve(cellsPerLevel[level]);
    }
    for (const auto& record : records) {
      const auto& cell = tree.getCell(record.treeCell);
      _arrays.cellSlotOfLattice[static_cast<std::size_t>(cell.level)].insert(
        getLatticeKey(cell.level, cell.latticeIndex),
        record.slot
      );
    }
  }
}

/**
 * Registers the face faceNumber of a tree cell once and returns its index. A face registered
 * twice carries the same kind from both sides.
 */
int
peano4::Spacetree::registerFace(const int treeCell, const int faceNumber, const FaceKind kind) {
  const auto& cell     = _tree->getCell(treeCell);
  const int   treeFace = cell.faceIndices[static_cast<std::size_t>(faceNumber)];
  assertion(treeFace >= 0 and treeFace < static_cast<int>(_arrays.faceOfTreeFace.size()), treeFace);
  int face = _arrays.faceOfTreeFace[static_cast<std::size_t>(treeFace)];
  if (face >= 0) {
    assertion(_arrays.faceKind[static_cast<std::size_t>(face)] == kind, face, static_cast<int>(kind));
    return face;
  }
  face                                                       = _arrays.numberOfFaces++;
  _arrays.faceOfTreeFace[static_cast<std::size_t>(treeFace)] = face;
  const int normal                                           = faceNumber % DIMENSIONS;
  auto      lattice                                          = cell.latticeIndex;
  if (faceNumber >= DIMENSIONS) {
    lattice(normal) += 1;
    if (_periodicBoundaries[normal] and lattice(normal) == _tree->getCellsPerAxisOfLevel(cell.level)) {
      lattice(normal) = 0;
    }
  }
  _arrays.faceLevel.push_back(cell.level);
  _arrays.faceNormal.push_back(normal);
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    _arrays.faceLattice.push_back(lattice(axis));
  }
  _arrays.cellsOfFace.push_back(-1);
  _arrays.cellsOfFace.push_back(-1);
  _arrays.faceKind.push_back(kind);
  _arrays.levelJumpOfCoarseFace.push_back(-1);
  return face;
}

/**
 * Registers the level jump below the face faceNumber of a refined cell: The face itself is the
 * coarse face and the faces of the cell's children on that side are the fine faces, ordered
 * along the tangential axes.
 */
int
peano4::Spacetree::registerLevelJump(const int refinedTreeCell, const int faceNumber, const int coarseFace) {
  if (_arrays.levelJumpOfCoarseFace[static_cast<std::size_t>(coarseFace)] >= 0) {
    return _arrays.levelJumpOfCoarseFace[static_cast<std::size_t>(coarseFace)];
  }
  const auto& tree = *_tree;
  assertion(tree.isRefined(refinedTreeCell), refinedTreeCell);
  const int normal         = faceNumber % DIMENSIONS;
  const int positionOnAxis = faceNumber < DIMENSIONS ? 0 : peano4::SubdivisionFactor - 1;
  LevelJump levelJump;
  levelJump.coarseFace = coarseFace;
  levelJump.coarseSide = 1 - getSideOfCellOnFace(faceNumber);
  levelJump.level      = tree.getCell(refinedTreeCell).level;
  for (int child = 0; child < peano4::ChildrenPerCell; child++) {
    const auto position = peano4::dDelinearised(child, peano4::SubdivisionFactor);
    if (position(normal) != positionOnAxis) {
      continue;
    }
    const int      childCell = tree.getChild(refinedTreeCell, child);
    const FaceKind kind      = tree.isRefined(childCell) ? FaceKind::Intermediate : FaceKind::Hanging;
    levelJump.fineFaces[getSubfaceIndexOf(position, normal)] = registerFace(childCell, faceNumber, kind);
  }
  const int levelJumpIndex = static_cast<int>(_arrays.levelJumps.size());
  _arrays.levelJumps.push_back(levelJump);
  _arrays.levelJumpOfCoarseFace[static_cast<std::size_t>(coarseFace)] = levelJumpIndex;
  return levelJumpIndex;
}

void
peano4::Spacetree::addExchangedFace(const int rank, const int face, const int side) {
  for (auto& neighbour : _arrays.neighbourFaces) {
    if (neighbour.rank == rank) {
      neighbour.faces.push_back({face, side});
      return;
    }
  }
  _arrays.neighbourFaces.push_back({rank, {{face, side}}});
}

/**
 * Derives every face of the local cells with its kind and registers one level jump per level of
 * the chain above a hanging face. Every face this rank and a neighbour rank hold is recorded with
 * the side this rank owns.
 */
void
peano4::Spacetree::deriveFaces(const std::vector<CellRecord>& records) {
  const auto& tree = *_tree;
  const int   rank = tarch::mpi::Rank::getInstance().getRank();
  _arrays.faceOfTreeFace.assign(tree.getNumberOfFaces(), -1);
  _arrays.levelJumpOfCoarseFace.clear();
  _arrays.numberOfFaces = 0;
  _arrays.faceLevel.clear();
  _arrays.faceNormal.clear();
  _arrays.faceLattice.clear();
  _arrays.cellsOfFace.clear();
  _arrays.faceKind.clear();
  _arrays.levelJumps.clear();
  _arrays.neighbourFaces.clear();

  for (const auto& record : records) {
    const auto& cell = tree.getCell(record.treeCell);
    const int   slot = record.slot;
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      const int  normal       = faceNumber % DIMENSIONS;
      const bool upper        = faceNumber >= DIMENSIONS;
      const int  ownSide      = getSideOfCellOnFace(faceNumber);
      const int  cellsPerAxis = tree.getCellsPerAxisOfLevel(cell.level);
      auto       neighbour    = cell.latticeIndex;
      neighbour(normal) += upper ? 1 : -1;
      const bool outside = neighbour(normal) < 0 or neighbour(normal) >= cellsPerAxis;
      if (outside and not _periodicBoundaries[normal]) {
        const int face = registerFace(record.treeCell, faceNumber, FaceKind::Boundary);
        _arrays.facesOfCell[static_cast<std::size_t>(slot) * peano4::FacesPerCell + faceNumber] = face;
        _arrays.cellsOfFace[static_cast<std::size_t>(face) * 2 + ownSide]                       = slot;
        continue;
      }
      if (outside) {
        neighbour(normal) = (neighbour(normal) + cellsPerAxis) % cellsPerAxis;
      }
      if (tree.cellExistsInPlan(cell.level, neighbour)) {
        if (not isRefinedIn(tree, cell.level, neighbour)) {
          const int face = registerFace(record.treeCell, faceNumber, FaceKind::Interior);
          _arrays.facesOfCell[static_cast<std::size_t>(slot) * peano4::FacesPerCell + faceNumber] = face;
          _arrays.cellsOfFace[static_cast<std::size_t>(face) * 2 + ownSide]                       = slot;
          const int owner = tree.getOwnerOfCellAt(cell.level, neighbour);
          if (owner != rank) {
            addExchangedFace(owner, face, ownSide);
          }
        } else {
          const int face = registerFace(record.treeCell, faceNumber, FaceKind::Coarse);
          _arrays.facesOfCell[static_cast<std::size_t>(slot) * peano4::FacesPerCell + faceNumber] = face;
          _arrays.cellsOfFace[static_cast<std::size_t>(face) * 2 + ownSide]                       = slot;
          auto childPosition    = peano4::dDelinearised(0, peano4::SubdivisionFactor);
          childPosition(normal) = upper ? 0 : peano4::SubdivisionFactor - 1;
          const int childOwner  = tree.getOwnerOfCellAt(cell.level + 1, getChildLatticeOf(neighbour, childPosition));
          if (childOwner != rank) {
            addExchangedFace(childOwner, face, ownSide);
          } else {
            const int neighbourCell = tree.getCellAt(cell.level, neighbour);
            assertion(neighbourCell >= 0, cell.level, neighbour);
            registerLevelJump(neighbourCell, getOppositeFaceNumber(faceNumber), face);
          }
        }
        continue;
      }

      const int face = registerFace(record.treeCell, faceNumber, FaceKind::Hanging);
      _arrays.facesOfCell[static_cast<std::size_t>(slot) * peano4::FacesPerCell + faceNumber] = face;
      _arrays.cellsOfFace[static_cast<std::size_t>(face) * 2 + ownSide]                       = slot;

      int  currentCell    = record.treeCell;
      auto currentLattice = cell.latticeIndex;
      int  currentLevel   = cell.level;
      while (true) {
        const int parentCell = tree.getCell(currentCell).parent;
        assertion(parentCell >= 0, currentCell, currentLevel);
        const auto parentLattice   = getParentLatticeOf(currentLattice);
        const int  parentLevel     = currentLevel - 1;
        const int  parentTreeFace  = tree.getCell(parentCell).faceIndices[static_cast<std::size_t>(faceNumber)];
        const bool parentFaceKnown = _arrays.faceOfTreeFace[static_cast<std::size_t>(parentTreeFace)] >= 0;
        auto       parentNeighbour = parentLattice;
        parentNeighbour(normal) += upper ? 1 : -1;
        const int parentCellsPerAxis = tree.getCellsPerAxisOfLevel(parentLevel);
        if (_periodicBoundaries[normal]) {
          parentNeighbour(normal) = (parentNeighbour(normal) + parentCellsPerAxis) % parentCellsPerAxis;
        }
        assertion(
          parentNeighbour(normal) >= 0 and parentNeighbour(normal) < parentCellsPerAxis,
          parentLevel,
          parentNeighbour
        );
        if (tree.cellExistsInPlan(parentLevel, parentNeighbour)) {
          const int coarseFace = registerFace(parentCell, faceNumber, FaceKind::Coarse);
          if (not parentFaceKnown) {
            const int owner = tree.getOwnerOfCellAt(parentLevel, parentNeighbour);
            if (owner != rank) {
              addExchangedFace(owner, coarseFace, ownSide);
            }
          }
          registerLevelJump(parentCell, faceNumber, coarseFace);
          break;
        }
        const int intermediate = registerFace(parentCell, faceNumber, FaceKind::Intermediate);
        registerLevelJump(parentCell, faceNumber, intermediate);
        if (parentFaceKnown) {
          break;
        }
        currentCell    = parentCell;
        currentLattice = parentLattice;
        currentLevel   = parentLevel;
      }
    }
  }
}

/**
 * Orders the lists so that every group a caller loops over is one span: The exchanged faces of
 * every neighbour in the canonical order (level, normal, lattice) and the neighbours by rank; the
 * cells by dependence and level, the faces by dependence and kind and the level jumps by
 * dependence and level, each group in the order of the curve. A fine face of a level jump whose
 * coarse face is remote is remote too, and so is a cell with a remote face.
 */
void
peano4::Spacetree::orderLists(const std::vector<CellRecord>& records) {
  const auto latticeKeyOfFace = [this](const int face) {
    tarch::la::Vector<DIMENSIONS, int> lattice(0);
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      lattice(axis) = _arrays.faceLattice[static_cast<std::size_t>(face) * DIMENSIONS + axis];
    }
    return getLatticeKey(_arrays.faceLevel[static_cast<std::size_t>(face)], lattice);
  };
  const auto facePrecedes = [this, &latticeKeyOfFace](const ExchangedFace& left, const ExchangedFace& right) {
    const int leftLevel  = _arrays.faceLevel[static_cast<std::size_t>(left.face)];
    const int rightLevel = _arrays.faceLevel[static_cast<std::size_t>(right.face)];
    if (leftLevel != rightLevel) {
      return leftLevel < rightLevel;
    }
    const int leftNormal  = _arrays.faceNormal[static_cast<std::size_t>(left.face)];
    const int rightNormal = _arrays.faceNormal[static_cast<std::size_t>(right.face)];
    if (leftNormal != rightNormal) {
      return leftNormal < rightNormal;
    }
    return latticeKeyOfFace(left.face) < latticeKeyOfFace(right.face);
  };
  std::sort(
    _arrays.neighbourFaces.begin(),
    _arrays.neighbourFaces.end(),
    [](const NeighbourFaces& left, const NeighbourFaces& right) { return left.rank < right.rank; }
  );
  _arrays.neighbours.clear();
  _arrays.exchangedFaces.clear();
  _arrays.faceDependsOnRemoteData.assign(static_cast<std::size_t>(_arrays.numberOfFaces), 0);
  for (auto& neighbour : _arrays.neighbourFaces) {
    std::sort(neighbour.faces.begin(), neighbour.faces.end(), facePrecedes);
    _arrays.neighbours.push_back(
      {neighbour.rank, static_cast<int>(_arrays.exchangedFaces.size()), static_cast<int>(neighbour.faces.size())}
    );
    for (const auto& entry : neighbour.faces) {
      _arrays.exchangedFaces.push_back(entry);
      _arrays.faceDependsOnRemoteData[static_cast<std::size_t>(entry.face)] = 1;
    }
  }

  std::stable_sort(
    _arrays.levelJumps.begin(),
    _arrays.levelJumps.end(),
    [](const LevelJump& left, const LevelJump& right) { return left.level < right.level; }
  );
  for (const auto& levelJump : _arrays.levelJumps) {
    if (_arrays.faceDependsOnRemoteData[static_cast<std::size_t>(levelJump.coarseFace)] != 0) {
      for (const int fineFace : levelJump.fineFaces) {
        _arrays.faceDependsOnRemoteData[static_cast<std::size_t>(fineFace)] = 1;
      }
    }
  }

  const auto dependenceOfFace = [this](const int face) {
    return _arrays.faceDependsOnRemoteData[static_cast<std::size_t>(face)] != 0 ? Dependence::Remote
                                                                                : Dependence::Local;
  };
  const auto levelsPerDependence = static_cast<std::size_t>(_arrays.maximumLevel) + 2;

  _arrays.faceRanges.assign(2 * (peano4::NumberOfFaceKinds + 1), 0);
  for (int face = 0; face < _arrays.numberOfFaces; face++) {
    _arrays.faceRanges[static_cast<std::size_t>(
      _partition.getRangeIndexOfKind(dependenceOfFace(face), _arrays.faceKind[static_cast<std::size_t>(face)])
    )]++;
  }
  turnCountsIntoOffsets(_arrays.faceRanges);
  _arrays.faces.assign(static_cast<std::size_t>(_arrays.numberOfFaces), -1);
  {
    std::vector<int> next = _arrays.faceRanges;
    for (int face = 0; face < _arrays.numberOfFaces; face++) {
      const auto range = static_cast<std::size_t>(
        _partition.getRangeIndexOfKind(dependenceOfFace(face), _arrays.faceKind[static_cast<std::size_t>(face)])
      );
      _arrays.faces[static_cast<std::size_t>(next[range]++)] = face;
    }
  }

  _arrays.levelJumpRanges.assign(2 * levelsPerDependence, 0);
  for (const auto& levelJump : _arrays.levelJumps) {
    _arrays.levelJumpRanges
      [static_cast<std::size_t>(dependenceOfFace(levelJump.coarseFace)) * levelsPerDependence
       + static_cast<std::size_t>(levelJump.level)]++;
  }
  turnCountsIntoOffsets(_arrays.levelJumpRanges);
  {
    std::vector<LevelJump> ordered(_arrays.levelJumps.size());
    std::vector<int>       next = _arrays.levelJumpRanges;
    for (const auto& levelJump : _arrays.levelJumps) {
      const auto range = static_cast<std::size_t>(dependenceOfFace(levelJump.coarseFace)) * levelsPerDependence
        + static_cast<std::size_t>(levelJump.level);
      ordered[static_cast<std::size_t>(next[range]++)] = levelJump;
    }
    _arrays.levelJumps = std::move(ordered);
  }

  const auto dependenceOfCell = [this](const int slot) {
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      const int face = _arrays.facesOfCell[static_cast<std::size_t>(slot) * peano4::FacesPerCell + faceNumber];
      if (_arrays.faceDependsOnRemoteData[static_cast<std::size_t>(face)] != 0) {
        return Dependence::Remote;
      }
    }
    return Dependence::Local;
  };
  const auto cellRangeOf = [this, levelsPerDependence, &dependenceOfCell](const int slot) {
    return static_cast<std::size_t>(dependenceOfCell(slot)) * levelsPerDependence
      + static_cast<std::size_t>(_arrays.cellLevel[static_cast<std::size_t>(slot)]);
  };
  _arrays.cellRanges.assign(2 * levelsPerDependence, 0);
  for (const auto& record : records) {
    _arrays.cellRanges[cellRangeOf(record.slot)]++;
  }
  turnCountsIntoOffsets(_arrays.cellRanges);
  _arrays.cells.assign(records.size(), -1);
  {
    std::vector<int> next = _arrays.cellRanges;
    for (const auto& record : records) {
      _arrays.cells[static_cast<std::size_t>(next[cellRangeOf(record.slot)]++)] = record.slot;
    }
  }
}

/**
 * Numbers the cells along the space-filling curve over all ranks; every rank calls this. The walk
 * records the cells of this rank in curve order, and the ranks own consecutive sections of the
 * curve in rank order, so the cells of the lower ranks precede them.
 */
void
peano4::Spacetree::numberCellsAlongCurve(const std::vector<CellRecord>& records) {
  const int numberOfLocalCells        = static_cast<int>(records.size());
  int       numberOfCellsUpToThisRank = numberOfLocalCells;
  tarch::mpi::scan(
    &numberOfLocalCells,
    &numberOfCellsUpToThisRank,
    1,
    MPI_INT,
    MPI_SUM,
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
  const int firstCurveIndex = numberOfCellsUpToThisRank - numberOfLocalCells;

  _arrays.cellCurveIndex.assign(static_cast<std::size_t>(_arrays.numberOfSlots), -1);
  for (std::size_t position = 0; position < records.size(); position++) {
    _arrays.cellCurveIndex[static_cast<std::size_t>(records[position].slot)] =
      firstCurveIndex + static_cast<int>(position);
  }
}

/**
 * Points the partition at the host arrays and, with a GPU backend, uploads every array a kernel
 * reads and points the device twin at the copies. The range arrays, the neighbours and the curve
 * indices stay in host memory in both partitions, because only the host reads them.
 */
void
peano4::Spacetree::assemblePartitions() {
  const auto fill = [this](Partition& partition) {
    partition._numberOfSlots      = _arrays.numberOfSlots;
    partition._numberOfFaces      = _arrays.numberOfFaces;
    partition._numberOfCells      = static_cast<int>(_arrays.cells.size());
    partition._numberOfLevelJumps = static_cast<int>(_arrays.levelJumps.size());
    partition._numberOfNeighbours = static_cast<int>(_arrays.neighbours.size());
    partition._minimumLevel       = _arrays.minimumLevel;
    partition._maximumLevel       = _arrays.maximumLevel;
    partition._cellRanges         = _arrays.cellRanges.data();
    partition._faceRanges         = _arrays.faceRanges.data();
    partition._levelJumpRanges    = _arrays.levelJumpRanges.data();
    partition._neighbours         = _arrays.neighbours.data();
    partition._exchangedFaces     = _arrays.exchangedFaces.data();
    partition._cellCurveIndex     = _arrays.cellCurveIndex.data();
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      partition._domainOffset[axis] = _domainOffset(axis);
    }
    partition._deviceTwin = nullptr;
  };

  fill(_partition);
  _partition._cells        = _arrays.cells.data();
  _partition._cellLevel    = _arrays.cellLevel.data();
  _partition._cellLattice  = _arrays.cellLattice.data();
  _partition._facesOfCell  = _arrays.facesOfCell.data();
  _partition._faces        = _arrays.faces.data();
  _partition._faceLevel    = _arrays.faceLevel.data();
  _partition._faceNormal   = _arrays.faceNormal.data();
  _partition._faceLattice  = _arrays.faceLattice.data();
  _partition._cellsOfFace  = _arrays.cellsOfFace.data();
  _partition._faceKind     = _arrays.faceKind.data();
  _partition._levelJumps   = _arrays.levelJumps.data();
  _partition._widthOfLevel = _arrays.widthOfLevel.data();

#if defined(PEANO_WITH_GPU_BACKEND)
  _deviceArrays.cells.setValues(_arrays.cells);
  _deviceArrays.cellLevel.setValues(_arrays.cellLevel);
  _deviceArrays.cellLattice.setValues(_arrays.cellLattice);
  _deviceArrays.facesOfCell.setValues(_arrays.facesOfCell);
  _deviceArrays.faces.setValues(_arrays.faces);
  _deviceArrays.faceLevel.setValues(_arrays.faceLevel);
  _deviceArrays.faceNormal.setValues(_arrays.faceNormal);
  _deviceArrays.faceLattice.setValues(_arrays.faceLattice);
  _deviceArrays.cellsOfFace.setValues(_arrays.cellsOfFace);
  _deviceArrays.faceKind.setValues(_arrays.faceKind);
  _deviceArrays.levelJumps.setValues(_arrays.levelJumps);
  _deviceArrays.widthOfLevel.setValues(_arrays.widthOfLevel);

  fill(_partitionOnDevice);
  _partitionOnDevice._cells        = _deviceArrays.cells.getData();
  _partitionOnDevice._cellLevel    = _deviceArrays.cellLevel.getData();
  _partitionOnDevice._cellLattice  = _deviceArrays.cellLattice.getData();
  _partitionOnDevice._facesOfCell  = _deviceArrays.facesOfCell.getData();
  _partitionOnDevice._faces        = _deviceArrays.faces.getData();
  _partitionOnDevice._faceLevel    = _deviceArrays.faceLevel.getData();
  _partitionOnDevice._faceNormal   = _deviceArrays.faceNormal.getData();
  _partitionOnDevice._faceLattice  = _deviceArrays.faceLattice.getData();
  _partitionOnDevice._cellsOfFace  = _deviceArrays.cellsOfFace.getData();
  _partitionOnDevice._faceKind     = _deviceArrays.faceKind.getData();
  _partitionOnDevice._levelJumps   = _deviceArrays.levelJumps.getData();
  _partitionOnDevice._widthOfLevel = _deviceArrays.widthOfLevel.getData();
  _partition._deviceTwin           = &_partitionOnDevice;
#endif
}
