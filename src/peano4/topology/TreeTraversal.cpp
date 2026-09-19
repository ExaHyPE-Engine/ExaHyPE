// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TreeTraversal.h"

#include "peano4/grid/GridTraversalEvent.h"
#include "peano4/grid/GridTraversalObserver.h"
#include "peano4/grid/SendReceiveContext.h"
#include "peano4/grid/StackTopology.h"
#include "peano4/sfc/ActiveCurve.h"
#include "peano4/topology/ChunkedTasks.h"
#include "peano4/topology/Tree.h"
#include "tarch/Termination.h"
#include "tarch/la/ScalarOperations.h"
#include "tarch/mpi/Rank.h"
#include "tarch/tasking/FunctorTask.h"
#include "tarch/tasking/Runtime.h"
#include "tarch/tasking/TaskGraph.h"

namespace {
  constexpr int SubdivisionFactor = peano4::sfc::ActiveCurve::SubdivisionFactor;
  constexpr int ChildrenPerCell   = peano4::sfc::ActiveCurve::ChildrenPerCell;

  /** Sizes one axis field so three coordinates fit into one order key. */
  constexpr int OrderKeyBitsPerAxis = 21;

  /** Serialises lattice coordinates into one order key, one field per axis. */
  std::int64_t latticeOrderKey(const tarch::la::Vector<DIMENSIONS, int>& lattice) {
    std::int64_t orderKey = 0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      orderKey |= static_cast<std::int64_t>(lattice(axis)) << (OrderKeyBitsPerAxis * axis);
    }
    return orderKey;
  }

  /** Orders cross-rank entities by level, normal, and lattice key. */
  using EntityOrderKey = std::tuple<int, int, std::int64_t>;

  /**
   * Entries of the first-unit table of the interface membership derivation:
   * No unit, a first unit, the mark a second unit leaves, and the number of
   * the candidate the mark becomes.
   */
  constexpr int NoUnitEntry        = 0;
  constexpr int CandidateMarkEntry = -1;
  constexpr int entryOfUnit(const int unit) { return unit + 1; }
  constexpr int entryOfCandidate(const int candidate) { return -2 - candidate; }
  constexpr int candidateOfEntry(const int entry) { return -2 - entry; }

  /** Tasks per chunk of the recording's per-task cell counts. */
  constexpr int TasksPerCountChunk = 8;

  /** One cell adjacent to an entity with the cell's subtree task unit. */
  struct AdjacentUnit {
    int unit{-1};
    int cellIndex{-1};
  };

  /** One subtree task's touch of an entity with the number of its cells adjacent to the entity. */
  struct EntityTouch {
    int          unit{-1};
    std::uint8_t numberOfAdjacentCells{0};
  };

  /**
   * Lists the touches of one entity from its adjacency records: The records
   * with a unit, ordered by cell index, merged per unit. Returns the number
   * of touches.
   */
  int gatherTouchesFromRecords(
    const AdjacentUnit* const                        records,
    const int                                        numberOfRecords,
    std::array<EntityTouch, peano4::CellsPerVertex>& touches
  ) {
    assertion(numberOfRecords >= 1 and numberOfRecords <= peano4::CellsPerVertex, numberOfRecords);
    std::array<AdjacentUnit, peano4::CellsPerVertex> orderedRecords{};
    int                                              numberOfAdjacentCells = 0;
    for (int record = 0; record < numberOfRecords; record++) {
      if (records[record].unit >= 0) {
        orderedRecords[numberOfAdjacentCells] = records[record];
        numberOfAdjacentCells++;
      }
    }
    std::sort(
      orderedRecords.begin(),
      orderedRecords.begin() + numberOfAdjacentCells,
      [](const AdjacentUnit& left, const AdjacentUnit& right) { return left.cellIndex < right.cellIndex; }
    );
    int numberOfTouches = 0;
    for (int adjacentCell = 0; adjacentCell < numberOfAdjacentCells; adjacentCell++) {
      const int unit     = orderedRecords[adjacentCell].unit;
      int       position = 0;
      while (position < numberOfTouches and touches[position].unit != unit) {
        position++;
      }
      if (position == numberOfTouches) {
        touches[position] = {unit, 0};
        numberOfTouches++;
      }
      touches[position].numberOfAdjacentCells++;
    }
    return numberOfTouches;
  }

  tarch::la::Vector<DIMENSIONS, int> vertexLatticeOfCorner(
    const peano4::topology::Tree&     tree,
    const peano4::topology::TreeCell& cell,
    const int                         vertexNumber
  ) {
    const int                          cellsPerAxisOnLevel = peano4::topology::cellsPerAxis(cell.level);
    tarch::la::Vector<DIMENSIONS, int> vertexLattice(0);
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      int coordinate = cell.latticeIndex(axis) + ((vertexNumber >> axis) & 1);
      if (tree.getPeriodicBoundaries()[axis] and coordinate == cellsPerAxisOnLevel) {
        coordinate = 0;
      }
      vertexLattice(axis) = coordinate;
    }
    return vertexLattice;
  }

  tarch::la::Vector<DIMENSIONS, int> faceLatticeOfSide(
    const peano4::topology::Tree&     tree,
    const peano4::topology::TreeCell& cell,
    const int                         faceNumber
  ) {
    const int cellsPerAxisOnLevel = peano4::topology::cellsPerAxis(cell.level);
    const int normal              = faceNumber % DIMENSIONS;
    auto      faceLattice         = cell.latticeIndex;
    if (faceNumber >= DIMENSIONS) {
      faceLattice(normal) += 1;
    }
    if (tree.getPeriodicBoundaries()[normal] and faceLattice(normal) == cellsPerAxisOnLevel) {
      faceLattice(normal) = 0;
    }
    return faceLattice;
  }

  /**
   * Maps a cell's vertex and one of that vertex's adjacency entries to the
   * cell that entry names within the 3^d patch centred on the cell.
   */
  struct AdjacentCellTable {
    int cellInPatch[peano4::VerticesPerCell][peano4::CellsPerVertex]{};
  };

  constexpr AdjacentCellTable AdjacentCells = [] {
    AdjacentCellTable table{};
    for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
      for (int adjacencyEntry = 0; adjacencyEntry < peano4::CellsPerVertex; adjacencyEntry++) {
        int linearised = 0;
        int stride     = 1;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          const int vertexBit    = (vertexNumber >> axis) & 1;
          const int adjacencyBit = (adjacencyEntry >> axis) & 1;
          linearised += (vertexBit + adjacencyBit) * stride;
          stride *= peano4::NeighbourhoodWidth;
        }
        table.cellInPatch[vertexNumber][adjacencyEntry] = linearised;
      }
    }
    return table;
  }();

  /**
   * Maps each of the 3^d cells around a cell to the corners of the cell it
   * touches, as a bit mask over the cell's 2^d vertices. A persistent corner
   * has a cell at every adjacency position inside the domain, so a neighbour
   * that touches a persistent corner exists without a lattice lookup.
   */
  constexpr std::array<std::uint8_t, peano4::CellsPerNeighbourhood> CornersTouchingNeighbour = [] {
    std::array<std::uint8_t, peano4::CellsPerNeighbourhood> corners{};
    for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
      for (int adjacencyEntry = 0; adjacencyEntry < peano4::CellsPerVertex; adjacencyEntry++) {
        corners[AdjacentCells.cellInPatch[vertexNumber][adjacencyEntry]] |=
          static_cast<std::uint8_t>(1U << vertexNumber);
      }
    }
    return corners;
  }();

  /** Returns the bit mask of the corners whose vertex is persistent. */
  std::uint8_t persistentCornerMask(const peano4::grid::GridVertex vertices[peano4::VerticesPerCell]) {
    std::uint8_t mask = 0;
    for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
      if (vertices[vertexNumber].getState() != peano4::grid::GridVertex::State::HangingVertex) {
        mask |= static_cast<std::uint8_t>(1U << vertexNumber);
      }
    }
    return mask;
  }

  /**
   * Returns whether the domain holds a cell at one of the 3^d positions
   * around a cell and writes that cell's lattice index to neighbourLattice.
   * persistentCorners masks the cell's persistent corners; a neighbour that
   * touches one exists without the lattice lookup, and a zero mask looks
   * every neighbour up.
   */
  bool neighbourhoodLatticeAt(
    const peano4::topology::Tree&       tree,
    const peano4::topology::TreeCell&   cell,
    const int                           position,
    const std::uint8_t                  persistentCorners,
    tarch::la::Vector<DIMENSIONS, int>& neighbourLattice
  ) {
    const int cellsPerAxisOnLevel = tree.getCellsPerAxisOfLevel(cell.level);
    int       remainingPosition   = position;
    bool      isInsideDomain      = true;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      int coordinate = cell.latticeIndex(axis) + remainingPosition % peano4::NeighbourhoodWidth - 1;
      remainingPosition /= peano4::NeighbourhoodWidth;
      if (tree.getPeriodicBoundaries()[axis]) {
        coordinate = (coordinate + cellsPerAxisOnLevel) % cellsPerAxisOnLevel;
      }
      isInsideDomain         = isInsideDomain and coordinate >= 0 and coordinate < cellsPerAxisOnLevel;
      neighbourLattice(axis) = coordinate;
    }
    if (not isInsideDomain) {
      return false;
    }
    if ((CornersTouchingNeighbour[position] & persistentCorners) != 0) {
      assertion(tree.cellExistsInPlan(cell.level, neighbourLattice), cell.level, position, persistentCorners);
      return true;
    }
    return tree.cellExistsInPlan(cell.level, neighbourLattice);
  }

  /**
   * Owners of the 3^d cells around one cell, -1 where no cell exists. One
   * gather serves the cell's vertex and face localities, its adjacency mask
   * and its touch counts. persistentCorners masks the cell's persistent
   * corners, whose neighbours need no lattice lookup.
   */
  using NeighbourhoodOwners = std::array<int, peano4::CellsPerNeighbourhood>;

  NeighbourhoodOwners gatherNeighbourhoodOwners(
    const peano4::topology::Tree&     tree,
    const peano4::topology::TreeCell& cell,
    const std::uint8_t                persistentCorners
  ) {
    NeighbourhoodOwners result;
    for (int position = 0; position < peano4::CellsPerNeighbourhood; position++) {
      tarch::la::Vector<DIMENSIONS, int> neighbourLattice(0);
      result[position] = neighbourhoodLatticeAt(tree, cell, position, persistentCorners, neighbourLattice)
        ? tree.getOwnerOfCellAt(cell.level, neighbourLattice)
        : -1;
    }
    return result;
  }

  using NeighbourhoodTraversal = std::array<bool, peano4::CellsPerNeighbourhood>;

  /**
   * Returns which of the 3^d cells around one cell touch the vertices the
   * rank holds: Its own cells, every cell above the spawn level, and every
   * spawn-level block with a local leaf below it. Below the spawn level the
   * ownership is exact per cell, so a remote cell there moves nothing.
   */
  NeighbourhoodTraversal gatherNeighbourhoodTraversal(
    const peano4::topology::Tree&     tree,
    const peano4::topology::TreeCell& cell,
    const NeighbourhoodOwners&        neighbourhoodOwners,
    const int                         treeId
  ) {
    NeighbourhoodTraversal result;
    for (int position = 0; position < peano4::CellsPerNeighbourhood; position++) {
      tarch::la::Vector<DIMENSIONS, int> neighbourLattice(0);
      result[position] = neighbourhoodOwners[position] == treeId
        or (neighbourhoodOwners[position] >= 0 and cell.level <= tree.getSpawnLevel()
            and (cell.level < tree.getSpawnLevel()
                 or (neighbourhoodLatticeAt(tree, cell, position, 0, neighbourLattice)
                     and tree.subtreeTouchesRankAt(cell.level, neighbourLattice, treeId))));
    }
    return result;
  }

  using NeighbourhoodUnits = std::array<int, peano4::CellsPerNeighbourhood>;

  /**
   * Returns the subtree-task units of the 3^d cells around one cell, -1 where
   * no cell exists or the cell belongs to no unit. A subtree task narrows the
   * particle locality of its events to its own unit with this gather.
   */
  NeighbourhoodUnits gatherNeighbourhoodUnits(
    const peano4::topology::Tree&     tree,
    const peano4::topology::TreeCell& cell,
    const std::vector<int>&           unitOfCell
  ) {
    NeighbourhoodUnits result;
    for (int position = 0; position < peano4::CellsPerNeighbourhood; position++) {
      tarch::la::Vector<DIMENSIONS, int> neighbourLattice(0);
      result[position] = -1;
      if (neighbourhoodLatticeAt(tree, cell, position, 0, neighbourLattice)) {
        const auto neighbourCellIndex = tree.getCellAt(cell.level, neighbourLattice);
        result[position]              = neighbourCellIndex >= 0 ? unitOfCell[neighbourCellIndex] : -1;
      }
    }
    return result;
  }

  /**
   * Returns whether the cell counts the vertex in the grid statistics. A
   * vertex counts once across all ranks and tasks: On the cell at the lowest
   * adjacency entry among the cells of its lowest owner. The caller passes
   * the owners of the 3^d cells around a cell that the given tree owns.
   */
  bool cellCountsVertex(const NeighbourhoodOwners& neighbourhoodOwners, const int vertexNumber, const int treeId) {
    assertion(vertexNumber >= 0 and vertexNumber < peano4::VerticesPerCell, vertexNumber);
    const int ownAdjacencyEntry = peano4::CellsPerVertex - 1 - vertexNumber;
    assertion(neighbourhoodOwners[AdjacentCells.cellInPatch[vertexNumber][ownAdjacencyEntry]] == treeId, treeId);
    for (int adjacencyEntry = 0; adjacencyEntry < peano4::CellsPerVertex; adjacencyEntry++) {
      const int owner = neighbourhoodOwners[AdjacentCells.cellInPatch[vertexNumber][adjacencyEntry]];
      if (owner >= 0 and (owner < treeId or (owner == treeId and adjacencyEntry < ownAdjacencyEntry))) {
        return false;
      }
    }
    return true;
  }

  /**
   * Returns which of a cell's vertices the given tree holds. A vertex is
   * local while the tree owns any cell around it; a hanging vertex answers
   * through its governing coarse vertex.
   */
  std::bitset<peano4::VerticesPerCell> areVerticesLocal(
    const peano4::topology::Tree&     tree,
    const peano4::topology::TreeCell& cell,
    const peano4::grid::GridVertex    vertices[peano4::VerticesPerCell],
    const NeighbourhoodOwners&        neighbourhoodOwners,
    const int                         treeId
  ) {
    std::bitset<peano4::VerticesPerCell> result;
    for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
      bool isLocal = false;
      if (vertices[vertexNumber].getState() == peano4::grid::GridVertex::State::HangingVertex) {
        const auto vertexLattice = vertexLatticeOfCorner(tree, cell, vertexNumber);
        for (int adjacencyEntry = 0; adjacencyEntry < peano4::CellsPerVertex and not isLocal; adjacencyEntry++) {
          isLocal = tree.getAdjacentCellOwnerAt(cell.level, vertexLattice, adjacencyEntry) == treeId;
        }
      } else {
        for (int adjacencyEntry = 0; adjacencyEntry < peano4::CellsPerVertex and not isLocal; adjacencyEntry++) {
          isLocal = neighbourhoodOwners[AdjacentCells.cellInPatch[vertexNumber][adjacencyEntry]] == treeId;
        }
      }
      result.set(vertexNumber, isLocal);
    }
    return result;
  }

  /**
   * Returns which of a cell's faces the given tree holds: The cell's own
   * side or an existing same-level neighbour across the face belongs to it.
   */
  std::bitset<peano4::FacesPerCell> areFacesLocal(
    const peano4::topology::TreeCell& cell,
    const NeighbourhoodOwners&        neighbourhoodOwners,
    const int                         treeId
  ) {
    std::bitset<peano4::FacesPerCell> result;
    constexpr int                     CentrePosition = (peano4::CellsPerNeighbourhood - 1) / 2;
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      const int normal = faceNumber % DIMENSIONS;
      int       stride = 1;
      for (int axis = 0; axis < normal; axis++) {
        stride *= peano4::NeighbourhoodWidth;
      }
      const int neighbourPosition = faceNumber < DIMENSIONS ? CentrePosition - stride : CentrePosition + stride;
      result.set(faceNumber, cell.ownerTreeId == treeId or neighbourhoodOwners[neighbourPosition] == treeId);
    }
    return result;
  }

  /** Seconds since an arbitrary fixed origin, so differences measure wall clock. */
  double now() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }

  /**
   * Adds one task's wall clock to the accumulator every task of the sweep
   * updates. Concurrent tasks reach it through a relaxed exchange loop.
   */
  void accumulateSeconds(std::atomic<double>& accumulator, const double seconds) {
    auto current = accumulator.load(std::memory_order_relaxed);
    while (not accumulator.compare_exchange_weak(current, current + seconds, std::memory_order_relaxed)) {
    }
  }

  /** The automaton state of the domain's root cell, which is level zero over the whole domain. */
  peano4::grid::AutomatonState rootStateOf(
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
    const tarch::la::Vector<DIMENSIONS, double>& domainWidth
  ) {
    peano4::grid::AutomatonState state;
    state.x = domainOffset;
    state.h = domainWidth;
    return state;
  }

  std::unique_ptr<peano4::grid::GridTraversalObserver> cloneObserverForTask(
    peano4::grid::GridTraversalObserver& observer,
    const int                            treeId
  ) {
    auto clone = std::unique_ptr<peano4::grid::GridTraversalObserver>(observer.clone(treeId));
    if (clone == nullptr) {
      tarch::terminateWithError("A grid traversal observer returned a null task clone.");
    }
    return clone;
  }
} // namespace

/**
 * Returns the kept clone of the given observer for one subtree task. A clone
 * constructs on the first call per observer type and task position and is
 * destroyed with the traversal, so a standing sweep constructs and destroys no
 * observer. The clone's tree id is the rank plus a multiple of the rank
 * count, so it is unique across all ranks and their tasks and can name a
 * particle's origin.
 */
peano4::grid::GridTraversalObserver& peano4::topology::TreeTraversal::observerOfTask(
  peano4::grid::GridTraversalObserver& observer,
  const int                            taskNumber
) const {
  auto& observersOfType = _taskObserversOfType[std::type_index(typeid(observer))];
  if (observersOfType.size() <= static_cast<std::size_t>(taskNumber)) {
    observersOfType.resize(taskNumber + 1);
  }
  if (observersOfType[taskNumber] == nullptr) {
    const int numberOfRanks     = tarch::mpi::Rank::getInstance().getNumberOfRanks();
    observersOfType[taskNumber] = cloneObserverForTask(observer, _id + numberOfRanks * (1 + taskNumber));
  }
  return *observersOfType[taskNumber];
}

peano4::topology::TreeTraversal::TreeTraversal(
  const int                                    treeId,
  const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
  const tarch::la::Vector<DIMENSIONS, double>& domainWidth
) :
  _id(treeId),
  _root(rootStateOf(domainOffset, domainWidth)) {}

const peano4::grid::GridStatistics& peano4::topology::TreeTraversal::getGridStatistics() const { return _statistics; }

/**
 * Adopts the given statistics as this traversal's current ones. A rebuild
 * replaces the traversal mid-regrid, and the sweep's statistics have to
 * survive into the replacement so the reported counts stay truthful.
 */
void peano4::topology::TreeTraversal::adoptGridStatistics(const peano4::grid::GridStatistics& statistics) {
  _statistics = statistics;
}

/**
 * Replays one recorded entry buffer against the observer. An interface merge
 * runs immediately before its recorded entry because an observer may retain
 * references to entity data across calls.
 */
void peano4::topology::TreeTraversal::replayEntries(
  const std::vector<ReplayEntry>&              entries,
  const std::vector<InterfaceMergeDescriptor>& merges,
  peano4::grid::GridTraversalObserver&         observer
) {
  std::size_t nextMerge = 0;
  for (const auto& entry : entries) {
    if (entry.isEnter) {
      assertion(entry.numberOfMergesBefore >= 0, entry.numberOfMergesBefore);
      const auto mergeEnd = nextMerge + static_cast<std::size_t>(entry.numberOfMergesBefore);
      assertion(mergeEnd <= merges.size(), nextMerge, entry.numberOfMergesBefore, merges.size());
      while (nextMerge < mergeEnd) {
        runMerge(merges[nextMerge], observer);
        nextMerge++;
      }
      observer.enterNode(entry.event);
    } else {
      assertion(entry.numberOfMergesBefore == 0, entry.numberOfMergesBefore);
      observer.leaveNode(entry.event);
    }
  }
  assertion(nextMerge == merges.size(), nextMerge, merges.size());
}

void peano4::topology::TreeTraversal::runMerge(
  const InterfaceMergeDescriptor&      merge,
  peano4::grid::GridTraversalObserver& observer
) {
  for (std::size_t sourceNumber = 0; sourceNumber < merge.sourceSlots.size(); sourceNumber++) {
    // A consolidation's first source overwrites the target slot. The observer's
    // merge routine performs that overwrite, so it can delete what the slot held.
    const bool overwritesTargetSlot   = merge.rebuildsTargetSlot and sourceNumber == 0;
    const bool sourceSlotIsOnThisRank = sourceNumber < static_cast<std::size_t>(merge.numberOfPartnerSlots);
    const auto contextOfSourceSlot    = sourceSlotIsOnThisRank ? peano4::grid::SendReceiveContext::IntraRank
                                                               : peano4::grid::SendReceiveContext::InterRank;
    const auto context = overwritesTargetSlot ? peano4::grid::SendReceiveContext::Consolidation : contextOfSourceSlot;
    observer.receiveNodeData(
      merge.isVertex ? peano4::grid::GridTraversalObserver::NodeData::Vertex
                     : peano4::grid::GridTraversalObserver::NodeData::Face,
      merge.position,
      merge.sourceSlots[sourceNumber],
      context,
      merge.event
    );
  }
}

/**
 * Runs the given merges through the observer's generated merge routines.
 * Each descriptor's event names the slot the merge accumulates into and
 * carries its marker; the source slots stay untouched.
 */
void peano4::topology::TreeTraversal::runMerges(
  const std::vector<InterfaceMergeDescriptor>& merges,
  peano4::grid::GridTraversalObserver&         observer
) {
  for (const auto& merge : merges) {
    runMerge(merge, observer);
  }
}

/**
 * Returns the remote frozen slots of one entity, or nullptr when no
 * neighbour rank holds it. The partner table is sorted by entity slot, so
 * the lookup is a binary search.
 */
const std::vector<int>* peano4::topology::TreeTraversal::findRemoteFrozenSlots(
  const std::vector<std::pair<int, std::vector<int>>>& remotePartners,
  const int                                            entitySlot
) {
  const auto entry =
    std::lower_bound(remotePartners.begin(), remotePartners.end(), entitySlot, [](const auto& partner, const int slot) {
      return partner.first < slot;
    });
  return entry != remotePartners.end() and entry->first == entitySlot ? &entry->second : nullptr;
}

void peano4::topology::TreeTraversal::assertRankTopologyMatchesTree(const peano4::topology::Tree& tree) const {
  assertion(_rankTopology.derived);
  assertion(
    _rankTopology.cellSlotOfTreeCell.size() == tree.getNumberOfCells(),
    _rankTopology.cellSlotOfTreeCell.size(),
    tree.getNumberOfCells()
  );
  assertion(
    _rankTopology.vertexSlotOfTreeVertex.size() == tree.getNumberOfVertices(),
    _rankTopology.vertexSlotOfTreeVertex.size(),
    tree.getNumberOfVertices()
  );
  assertion(
    _rankTopology.faceSlotOfTreeFace.size() == tree.getNumberOfFaces(),
    _rankTopology.faceSlotOfTreeFace.size(),
    tree.getNumberOfFaces()
  );
  assertion(_rankTopology.numberOfLocalCellSlots >= 0, _rankTopology.numberOfLocalCellSlots);
  assertion(_rankTopology.numberOfLocalVertexSlots >= 0, _rankTopology.numberOfLocalVertexSlots);
  assertion(_rankTopology.numberOfLocalFaceSlots >= 0, _rankTopology.numberOfLocalFaceSlots);
  assertion(
    _rankTopology.cellSlotSpaceSize >= _rankTopology.numberOfLocalCellSlots,
    _rankTopology.cellSlotSpaceSize,
    _rankTopology.numberOfLocalCellSlots
  );
  assertion(
    _rankTopology.vertexSlotSpaceSize >= _rankTopology.numberOfLocalVertexSlots,
    _rankTopology.vertexSlotSpaceSize,
    _rankTopology.numberOfLocalVertexSlots
  );
  assertion(
    _rankTopology.faceSlotSpaceSize >= _rankTopology.numberOfLocalFaceSlots,
    _rankTopology.faceSlotSpaceSize,
    _rankTopology.numberOfLocalFaceSlots
  );
  assertion(_rankTopology.numberOfVertexFrozenSlots >= 0, _rankTopology.numberOfVertexFrozenSlots);
  assertion(_rankTopology.numberOfFaceFrozenSlots >= 0, _rankTopology.numberOfFaceFrozenSlots);
  assertion(
    _rankTopology.vertexPackLists.size() == _rankTopology.neighbourRanks.size(),
    _rankTopology.vertexPackLists.size(),
    _rankTopology.neighbourRanks.size()
  );
  assertion(
    _rankTopology.facePackLists.size() == _rankTopology.neighbourRanks.size(),
    _rankTopology.facePackLists.size(),
    _rankTopology.neighbourRanks.size()
  );
  assertion(
    _rankTopology.vertexFrozenSlotBases.size() == _rankTopology.neighbourRanks.size(),
    _rankTopology.vertexFrozenSlotBases.size(),
    _rankTopology.neighbourRanks.size()
  );
  assertion(
    _rankTopology.faceFrozenSlotBases.size() == _rankTopology.neighbourRanks.size(),
    _rankTopology.faceFrozenSlotBases.size(),
    _rankTopology.neighbourRanks.size()
  );
  static_cast<void>(tree);
}

void peano4::topology::TreeTraversal::assertRegridOverridesMatchTree(
  const peano4::topology::Tree& tree,
  const RegridOverrides&        overrides
) const {
  assertRankTopologyMatchesTree(tree);
  assertion(
    overrides.cellCreate.size() == tree.getNumberOfCells(),
    overrides.cellCreate.size(),
    tree.getNumberOfCells()
  );
  assertion(
    overrides.cellDestroy.size() == tree.getNumberOfCells(),
    overrides.cellDestroy.size(),
    tree.getNumberOfCells()
  );
  assertion(
    overrides.faceCreate.size() == tree.getNumberOfFaces(),
    overrides.faceCreate.size(),
    tree.getNumberOfFaces()
  );
  assertion(
    overrides.faceDestroy.size() == tree.getNumberOfFaces(),
    overrides.faceDestroy.size(),
    tree.getNumberOfFaces()
  );
  assertion(
    overrides.vertexCreate.size() == tree.getNumberOfVertices(),
    overrides.vertexCreate.size(),
    tree.getNumberOfVertices()
  );
  assertion(
    overrides.vertexDestroy.size() == tree.getNumberOfVertices(),
    overrides.vertexDestroy.size(),
    tree.getNumberOfVertices()
  );
  assertion(
    overrides.vertexRefinedBefore.size() == tree.getNumberOfVertices(),
    overrides.vertexRefinedBefore.size(),
    tree.getNumberOfVertices()
  );
  assertion(
    overrides.vertexRefinedAfter.size() == tree.getNumberOfVertices(),
    overrides.vertexRefinedAfter.size(),
    tree.getNumberOfVertices()
  );
  assertion(
    overrides.vertexFrozenSlotValid.size()
      == static_cast<std::size_t>(_rankTopology.vertexSlotSpaceSize)
        + static_cast<std::size_t>(_rankTopology.numberOfVertexFrozenSlots),
    overrides.vertexFrozenSlotValid.size(),
    _rankTopology.vertexSlotSpaceSize + _rankTopology.numberOfVertexFrozenSlots
  );
  assertion(
    overrides.faceFrozenSlotValid.size()
      == static_cast<std::size_t>(_rankTopology.faceSlotSpaceSize)
        + static_cast<std::size_t>(_rankTopology.numberOfFaceFrozenSlots),
    overrides.faceFrozenSlotValid.size(),
    _rankTopology.faceSlotSpaceSize + _rankTopology.numberOfFaceFrozenSlots
  );
  static_cast<void>(overrides);
}

/**
 * Derives the rank boundary from the refinement and ownership plans. Each rank
 * maps entities from its local tree view into its own pool slots, while the
 * entities two ranks hold sort by level, normal, and lattice key. Without a
 * partition the topology is empty.
 */
void peano4::topology::TreeTraversal::prepareRankTopology(
  const peano4::topology::Tree&          tree,
  const peano4::topology::Tree*          predecessorTree,
  const peano4::topology::TreeTraversal* predecessorTraversal
) {
  const bool inheritsSlots = predecessorTree != nullptr and predecessorTraversal != nullptr;
  assertion((predecessorTree == nullptr) == (predecessorTraversal == nullptr));
  if (inheritsSlots) {
    assertion(predecessorTraversal != this);
    predecessorTraversal->assertRankTopologyMatchesTree(*predecessorTree);
  }

  _rankTopology      = RankTopology{};
  _interfaceTopology = InterfaceTopology{};
  _cellIsBlockRoot.assign(tree.getNumberOfCells(), 0);
  _cellIsDirectTask.assign(tree.getNumberOfCells(), 0);
  _subtreeDecomposition     = SubtreeDecomposition{};
  _eventRecording           = EventRecording{};
  _parallelRecording        = ParallelRecording{};
  _numberOfStationarySweeps = 0;
  _staleRerecordUnits.clear();
  _partialRerecordPending = false;
  _pendingFlushVertexPairs.clear();
  _pendingFlushFacePairs.clear();
  _snapshotCellIsBlockRoot.clear();
  _snapshotCellIsDirectTask.clear();
  _snapshotSubtreeDecomposition = SubtreeDecomposition{};

  // Across a regrid every surviving entity inherits its slot from the
  // predecessor topology, so its pool data stays in place and the migration
  // hop copies nothing for it; new entities fill the freed gaps in ascending
  // order before the slot range grows. A range emptier than half its slots
  // reassigns densely, which repacks the pool through the migration pairs.
  const auto assignSlots = [](
                             const int         numberOfEntities,
                             const auto&       isLocalEntity,
                             const auto&       inheritedSlotOfEntity,
                             const bool        inheritsFromPredecessor,
                             const int         predecessorSlotSpaceSize,
                             std::vector<int>& slotOfEntity,
                             int&              numberOfLocalSlots,
                             int&              slotSpaceSize
                           ) {
    slotOfEntity.assign(numberOfEntities, -1);
    numberOfLocalSlots = 0;
    if (inheritsFromPredecessor) {
      assertion(predecessorSlotSpaceSize >= 0, predecessorSlotSpaceSize);
      std::vector<char> slotTaken(predecessorSlotSpaceSize, 0);
      for (int entityIndex = 0; entityIndex < numberOfEntities; entityIndex++) {
        if (not isLocalEntity(entityIndex)) {
          continue;
        }
        numberOfLocalSlots++;
        const auto inheritedSlot = inheritedSlotOfEntity(entityIndex);
        if (inheritedSlot >= 0) {
          assertion(inheritedSlot < predecessorSlotSpaceSize, inheritedSlot, predecessorSlotSpaceSize);
          assertion(slotTaken[inheritedSlot] == 0, inheritedSlot);
          slotOfEntity[entityIndex] = inheritedSlot;
          slotTaken[inheritedSlot]  = 1;
        }
      }
      if (numberOfLocalSlots >= predecessorSlotSpaceSize / 2 + predecessorSlotSpaceSize % 2) {
        slotSpaceSize    = predecessorSlotSpaceSize;
        int nextFreeSlot = 0;
        for (int entityIndex = 0; entityIndex < numberOfEntities; entityIndex++) {
          if (not isLocalEntity(entityIndex) or slotOfEntity[entityIndex] >= 0) {
            continue;
          }
          while (nextFreeSlot < predecessorSlotSpaceSize and slotTaken[nextFreeSlot] != 0) {
            nextFreeSlot++;
          }
          slotOfEntity[entityIndex] = nextFreeSlot;
          slotSpaceSize             = std::max(slotSpaceSize, nextFreeSlot + 1);
          nextFreeSlot++;
        }
        return true;
      }
      std::fill(slotOfEntity.begin(), slotOfEntity.end(), -1);
      numberOfLocalSlots = 0;
    }
    // The fresh slots number the local entities in index order: Every chunk
    // counts its local entities, and the prefix sums over the chunks place
    // them. Both loops cut the range into the same chunks.
    const int        numberOfChunks = peano4::topology::numberOfChunks(numberOfEntities);
    std::vector<int> firstSlotOfChunk(numberOfChunks + 1, 0);
    peano4::topology::runChunkedTasks(numberOfEntities, [&](const int chunkNumber, const int begin, const int end) {
      int numberOfLocalEntities = 0;
      for (int entityIndex = begin; entityIndex < end; entityIndex++) {
        if (isLocalEntity(entityIndex)) {
          numberOfLocalEntities++;
        }
      }
      firstSlotOfChunk[chunkNumber + 1] = numberOfLocalEntities;
    });
    for (int chunkNumber = 0; chunkNumber < numberOfChunks; chunkNumber++) {
      firstSlotOfChunk[chunkNumber + 1] += firstSlotOfChunk[chunkNumber];
    }
    peano4::topology::runChunkedTasks(numberOfEntities, [&](const int chunkNumber, const int begin, const int end) {
      int slot = firstSlotOfChunk[chunkNumber];
      for (int entityIndex = begin; entityIndex < end; entityIndex++) {
        if (isLocalEntity(entityIndex)) {
          slotOfEntity[entityIndex] = slot;
          slot++;
        }
      }
      assertion(slot == firstSlotOfChunk[chunkNumber + 1], chunkNumber, slot, firstSlotOfChunk[chunkNumber + 1]);
    });
    numberOfLocalSlots = firstSlotOfChunk[numberOfChunks];
    slotSpaceSize      = numberOfLocalSlots;
    return false;
  };

  // The inherited slot assignment fills the freed gaps in index order and
  // therefore stays sequential, but everything it reads per entity is
  // independent. The predecessor lookups are the expensive part, so they run
  // over the whole tree as chunked tasks first and the sequential pass reads
  // an array.
  const int numberOfCells = static_cast<int>(tree.getNumberOfCells());

  std::vector<int> inheritedCellSlotOfCell;
  if (inheritsSlots) {
    inheritedCellSlotOfCell.assign(numberOfCells, -1);
    peano4::topology::runChunkedTasks(numberOfCells, [&](const int chunkNumber, const int begin, const int end) {
      static_cast<void>(chunkNumber);
      for (int cellIndex = begin; cellIndex < end; cellIndex++) {
        const auto& cell = tree.getCell(cellIndex);
        if (cell.ownerTreeId != _id) {
          continue;
        }
        const auto predecessorIndex = predecessorTree->getCellAt(cell.level, cell.latticeIndex);
        inheritedCellSlotOfCell[cellIndex] =
          predecessorIndex >= 0 ? predecessorTraversal->_rankTopology.cellSlotOfTreeCell[predecessorIndex] : -1;
      }
    });
  }

  const auto isLocalCell = [&tree, this](const int cellIndex) { return tree.getCell(cellIndex).ownerTreeId == _id; };
  const auto inheritedCellSlot = [&inheritedCellSlotOfCell](const int cellIndex) {
    return inheritedCellSlotOfCell[cellIndex];
  };
  const bool cellSlotsInherited = assignSlots(
    static_cast<int>(tree.getNumberOfCells()),
    isLocalCell,
    inheritedCellSlot,
    inheritsSlots,
    inheritsSlots ? predecessorTraversal->_rankTopology.cellSlotSpaceSize : 0,
    _rankTopology.cellSlotOfTreeCell,
    _rankTopology.numberOfLocalCellSlots,
    _rankTopology.cellSlotSpaceSize
  );

  // Entity locality follows actual cell ownership. A hanging vertex answers
  // adjacency through its governing coarse vertices and therefore cannot
  // derive its locality from it.
  std::vector<std::atomic<char>> vertexIsLocal(tree.getNumberOfVertices());
  std::vector<int>               inheritedVertexSlotOfVertex;
  std::vector<char>              faceIsLocal(tree.getNumberOfFaces(), 0);
  std::vector<int>               inheritedFaceSlotOfFace;
  std::vector<std::atomic<char>> faceResolved(tree.getNumberOfFaces());
  if (inheritsSlots) {
    inheritedVertexSlotOfVertex.assign(tree.getNumberOfVertices(), -1);
    inheritedFaceSlotOfFace.assign(tree.getNumberOfFaces(), -1);
  }
  peano4::topology::runChunkedTasks(numberOfCells, [&](const int chunkNumber, const int begin, const int end) {
    static_cast<void>(chunkNumber);
    for (int cellIndex = begin; cellIndex < end; cellIndex++) {
      const auto& cell = tree.getCell(cellIndex);
      if (cell.ownerTreeId != _id) {
        continue;
      }
      for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
        const auto vertexIndex = cell.vertexIndices[vertexNumber];
        if (vertexIsLocal[vertexIndex].exchange(1, std::memory_order_relaxed) != 0) {
          continue;
        }
        if (inheritsSlots) {
          const auto predecessorIndex = predecessorTree->getVertexAt(
            tree.getVertex(vertexIndex).level,
            vertexLatticeOfCorner(tree, cell, vertexNumber)
          );
          inheritedVertexSlotOfVertex[vertexIndex] =
            predecessorIndex >= 0 ? predecessorTraversal->_rankTopology.vertexSlotOfTreeVertex[predecessorIndex] : -1;
        }
      }
      for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
        const auto faceIndex = cell.faceIndices[faceNumber];
        if (faceResolved[faceIndex].exchange(1, std::memory_order_relaxed) != 0) {
          continue;
        }
        faceIsLocal[faceIndex] = 1;
        if (inheritsSlots) {
          const auto predecessorIndex =
            predecessorTree->getFaceAt(cell.level, faceNumber % DIMENSIONS, faceLatticeOfSide(tree, cell, faceNumber));
          inheritedFaceSlotOfFace[faceIndex] =
            predecessorIndex >= 0 ? predecessorTraversal->_rankTopology.faceSlotOfTreeFace[predecessorIndex] : -1;
        }
      }
    }
  });
  const auto isLocalVertex = [&vertexIsLocal](const int vertexIndex) {
    return vertexIsLocal[vertexIndex].load(std::memory_order_relaxed) != 0;
  };
  const auto inheritedVertexSlot = [&inheritedVertexSlotOfVertex](const int vertexIndex) {
    return inheritedVertexSlotOfVertex[vertexIndex];
  };
  const bool vertexSlotsInherited = assignSlots(
    static_cast<int>(tree.getNumberOfVertices()),
    isLocalVertex,
    inheritedVertexSlot,
    inheritsSlots,
    inheritsSlots ? predecessorTraversal->_rankTopology.vertexSlotSpaceSize : 0,
    _rankTopology.vertexSlotOfTreeVertex,
    _rankTopology.numberOfLocalVertexSlots,
    _rankTopology.vertexSlotSpaceSize
  );

  const auto isLocalFace       = [&faceIsLocal](const int faceIndex) { return faceIsLocal[faceIndex] != 0; };
  const auto inheritedFaceSlot = [&inheritedFaceSlotOfFace](const int faceIndex) {
    return inheritedFaceSlotOfFace[faceIndex];
  };
  const bool faceSlotsInherited = assignSlots(
    static_cast<int>(tree.getNumberOfFaces()),
    isLocalFace,
    inheritedFaceSlot,
    inheritsSlots,
    inheritsSlots ? predecessorTraversal->_rankTopology.faceSlotSpaceSize : 0,
    _rankTopology.faceSlotOfTreeFace,
    _rankTopology.numberOfLocalFaceSlots,
    _rankTopology.faceSlotSpaceSize
  );
  _rankTopology.slotsInherited = cellSlotsInherited and vertexSlotsInherited and faceSlotsInherited;

  deriveRankBoundary(tree);
  _rankTopology.derived = true;
  assertRankTopologyMatchesTree(tree);
}

/**
 * Derives the rank boundary from the entity slot maps: The neighbour ranks,
 * the pack lists, the frozen slots behind the slot ranges and the remote
 * partners of every rank-boundary entity. A caller establishes the slot maps
 * and the slot ranges before the call.
 */
void peano4::topology::TreeTraversal::deriveRankBoundary(const peano4::topology::Tree& tree) {
  const int numberOfCells = static_cast<int>(tree.getNumberOfCells());
  _rankTopology.neighbourRanks.clear();
  _rankTopology.vertexPackLists.clear();
  _rankTopology.facePackLists.clear();
  _rankTopology.vertexFrozenSlotBases.clear();
  _rankTopology.faceFrozenSlotBases.clear();
  _rankTopology.remoteVertexPartners.clear();
  _rankTopology.remoteFacePartners.clear();
  _rankTopology.numberOfVertexFrozenSlots = 0;
  _rankTopology.numberOfFaceFrozenSlots   = 0;

  // One (entity slot, remote rank) pair per entity this rank and that
  // neighbour both touch; a hanging entity carries a per-visit copy on each
  // side and takes no pair. The pack lists on both sides of a rank boundary
  // must enumerate those entities in the same order, so the entries sort by
  // level, normal and lattice key, which both sides derive from their own
  // views.
  using KeyedBoundaryEntry    = std::pair<EntityOrderKey, std::pair<int, int>>;
  const auto sortAndStripKeys = [](std::vector<std::vector<KeyedBoundaryEntry>>& boundaryEntriesPerChunk) {
    std::vector<KeyedBoundaryEntry> keyedEntries;
    for (const auto& boundaryEntries : boundaryEntriesPerChunk) {
      keyedEntries.insert(keyedEntries.end(), boundaryEntries.begin(), boundaryEntries.end());
    }
    std::sort(keyedEntries.begin(), keyedEntries.end());
    std::vector<std::pair<int, int>> orderedEntries;
    orderedEntries.reserve(keyedEntries.size());
    for (const auto& [orderKey, boundaryEntry] : keyedEntries) {
      orderedEntries.push_back(boundaryEntry);
    }
    return orderedEntries;
  };

  // A partition with one owner has no remote boundary entries, so the two
  // scans below have nothing to find and the boundary structures stay empty.
  const bool derivesBoundaryEntries = tree.hasDistributedOwnership();

  // Every rank-boundary entity is adjacent to a local cell, and the cells
  // around a cell inside a block one rank owns lie in that block, so only a
  // local cell on the rim of a block whose neighbourhood has another owner,
  // in a block several ranks own or above the spawn level can touch the
  // boundary.
  const int spawnLevel     = tree.getSpawnLevel();
  const int blocksPerAxis  = derivesBoundaryEntries ? tree.getCellsPerAxisOfLevel(spawnLevel) : 0;
  int       numberOfBlocks = derivesBoundaryEntries ? 1 : 0;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    numberOfBlocks *= blocksPerAxis;
  }
  const auto blockLatticeOf = [blocksPerAxis](const int blockNumber) {
    tarch::la::Vector<DIMENSIONS, int> blockLattice(0);
    int                                remainingIndex = blockNumber;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      blockLattice(axis) = remainingIndex % blocksPerAxis;
      remainingIndex /= blocksPerAxis;
    }
    return blockLattice;
  };
  std::vector<char> blockIsSplit(numberOfBlocks, 0);
  std::vector<char> blockTouchesBoundary(numberOfBlocks, 0);
  for (int blockNumber = 0; blockNumber < numberOfBlocks; blockNumber++) {
    const auto blockLattice = blockLatticeOf(blockNumber);
    if (tree.isSplitBlockAt(blockLattice)) {
      blockIsSplit[blockNumber]         = 1;
      blockTouchesBoundary[blockNumber] = 1;
      continue;
    }
    const int owner = tree.getOwnerOfCellAt(spawnLevel, blockLattice);
    for (int neighbourNumber = 0;
         neighbourNumber < peano4::CellsPerNeighbourhood and blockTouchesBoundary[blockNumber] == 0;
         neighbourNumber++) {
      tarch::la::Vector<DIMENSIONS, int> neighbourLattice(0);
      int                                remainingIndex = neighbourNumber;
      bool                               isInsideDomain = true;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        int coordinate = blockLattice(axis) + remainingIndex % peano4::NeighbourhoodWidth - 1;
        remainingIndex /= peano4::NeighbourhoodWidth;
        if (tree.getPeriodicBoundaries()[axis]) {
          coordinate = (coordinate + blocksPerAxis) % blocksPerAxis;
        }
        isInsideDomain         = isInsideDomain and coordinate >= 0 and coordinate < blocksPerAxis;
        neighbourLattice(axis) = coordinate;
      }
      if (
        isInsideDomain
        and (tree.isSplitBlockAt(neighbourLattice) or tree.getOwnerOfCellAt(spawnLevel, neighbourLattice) != owner)
      ) {
        blockTouchesBoundary[blockNumber] = 1;
      }
    }
  }
  const auto cellMayTouchBoundary = [&tree, &blockIsSplit, &blockTouchesBoundary, spawnLevel, blocksPerAxis, this](
                                      const peano4::topology::TreeCell& cell
                                    ) {
    if (cell.ownerTreeId != _id) {
      return false;
    }
    if (cell.level < spawnLevel) {
      return true;
    }
    const int cellsPerAxisOfBlock = tree.getCellsPerAxisOfLevel(cell.level) / tree.getCellsPerAxisOfLevel(spawnLevel);
    int       blockNumber         = 0;
    int       blockStride         = 1;
    bool      liesOnBlockRim      = false;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      const int coordinateInBlock = cell.latticeIndex(axis) % cellsPerAxisOfBlock;
      blockNumber += (cell.latticeIndex(axis) / cellsPerAxisOfBlock) * blockStride;
      blockStride *= blocksPerAxis;
      liesOnBlockRim = liesOnBlockRim or coordinateInBlock == 0 or coordinateInBlock == cellsPerAxisOfBlock - 1;
    }
    return blockIsSplit[blockNumber] != 0 or (liesOnBlockRim and blockTouchesBoundary[blockNumber] != 0);
  };

  std::vector<std::vector<KeyedBoundaryEntry>> vertexBoundaryEntriesPerChunk(peano4::topology::maximumNumberOfChunks());
  std::vector<std::atomic<char>>               vertexClaimed(derivesBoundaryEntries ? tree.getNumberOfVertices() : 0);
  peano4::topology::runChunkedTasks(
    derivesBoundaryEntries ? numberOfCells : 0,
    [&](const int chunkNumber, const int begin, const int end) {
      auto& boundaryEntries = vertexBoundaryEntriesPerChunk[chunkNumber];
      for (int cellIndex = begin; cellIndex < end; cellIndex++) {
        const auto& cell = tree.getCell(cellIndex);
        if (not cellMayTouchBoundary(cell)) {
          continue;
        }
        const int cellsPerAxisOnLevel = tree.getCellsPerAxisOfLevel(cell.level);
        for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
          const auto vertexIndex = cell.vertexIndices[vertexNumber];
          if (vertexClaimed[vertexIndex].exchange(1, std::memory_order_relaxed) != 0) {
            continue;
          }
          const auto& vertex = tree.getVertex(vertexIndex);
          if (vertex.getState() == peano4::grid::GridVertex::State::HangingVertex) {
            continue;
          }
          // A persistent vertex has a cell at every adjacent position inside
          // the domain, so the owner reads from the position.
          const auto                              cornerLattice = vertexLatticeOfCorner(tree, cell, vertexNumber);
          std::array<int, peano4::CellsPerVertex> remoteOwners{};
          int                                     numberOfRemoteOwners = 0;
          for (int adjacencyEntry = 0; adjacencyEntry < peano4::CellsPerVertex; adjacencyEntry++) {
            bool                               isInsideDomain = true;
            tarch::la::Vector<DIMENSIONS, int> adjacentCellLattice(0);
            for (int axis = 0; axis < DIMENSIONS; axis++) {
              int adjacentCellCoordinate = cornerLattice(axis) - 1 + ((adjacencyEntry >> axis) & 1);
              if (tree.getPeriodicBoundaries()[axis]) {
                adjacentCellCoordinate = (adjacentCellCoordinate + cellsPerAxisOnLevel) % cellsPerAxisOnLevel;
              }
              isInsideDomain =
                isInsideDomain and adjacentCellCoordinate >= 0 and adjacentCellCoordinate < cellsPerAxisOnLevel;
              adjacentCellLattice(axis) = adjacentCellCoordinate;
            }
            if (not isInsideDomain) {
              continue;
            }
            // The cell around a local cell's vertex lies in a local or a ring
            // block, so its owner reads from the structure.
            const auto adjacentCellIndex = tree.getCellAt(cell.level, adjacentCellLattice);
            const auto owner = adjacentCellIndex >= 0 ? tree.getCell(adjacentCellIndex).ownerTreeId
                                                      : tree.getOwnerOfCellAt(cell.level, adjacentCellLattice);
            if (owner == _id or owner < 0) {
              continue;
            }
            bool alreadyListed = false;
            for (int ownerNumber = 0; ownerNumber < numberOfRemoteOwners; ownerNumber++) {
              alreadyListed = alreadyListed or remoteOwners[ownerNumber] == owner;
            }
            if (not alreadyListed) {
              remoteOwners[numberOfRemoteOwners] = owner;
              numberOfRemoteOwners++;
            }
          }
          const EntityOrderKey orderKey{cell.level, 0, latticeOrderKey(cornerLattice)};
          for (int ownerNumber = 0; ownerNumber < numberOfRemoteOwners; ownerNumber++) {
            boundaryEntries.push_back(
              {orderKey, {_rankTopology.vertexSlotOfTreeVertex[vertexIndex], remoteOwners[ownerNumber]}}
            );
          }
        }
      }
    }
  );
  const auto vertexBoundaryEntries = sortAndStripKeys(vertexBoundaryEntriesPerChunk);

  std::vector<std::vector<KeyedBoundaryEntry>> faceBoundaryEntriesPerChunk(peano4::topology::maximumNumberOfChunks());
  std::vector<std::atomic<char>>               faceVisited(derivesBoundaryEntries ? tree.getNumberOfFaces() : 0);
  peano4::topology::runChunkedTasks(
    derivesBoundaryEntries ? numberOfCells : 0,
    [&](const int chunkNumber, const int begin, const int end) {
      auto& boundaryEntries = faceBoundaryEntriesPerChunk[chunkNumber];
      for (int cellIndex = begin; cellIndex < end; cellIndex++) {
        const auto& cell = tree.getCell(cellIndex);
        if (not cellMayTouchBoundary(cell)) {
          continue;
        }
        for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
          const auto faceIndex = cell.faceIndices[faceNumber];
          if (faceVisited[faceIndex].exchange(1, std::memory_order_relaxed) != 0) {
            continue;
          }

          const int normal              = faceNumber % DIMENSIONS;
          const int cellsPerAxisOnLevel = tree.getCellsPerAxisOfLevel(cell.level);
          auto      neighbourLattice    = cell.latticeIndex;
          neighbourLattice(normal) += faceNumber < DIMENSIONS ? -1 : 1;
          if (tree.getPeriodicBoundaries()[normal]) {
            neighbourLattice(normal) = (neighbourLattice(normal) + cellsPerAxisOnLevel) % cellsPerAxisOnLevel;
          }
          // The neighbour of a local cell lies in the local block or in the
          // neighbour ring, so the structure answers whether it exists; the
          // plan tables may still describe the structure before an in-place
          // append.
          const auto neighbourCellIndex =
            neighbourLattice(normal) >= 0 and neighbourLattice(normal) < cellsPerAxisOnLevel
            ? tree.getCellAt(cell.level, neighbourLattice)
            : -1;
          const int neighbourOwner = neighbourCellIndex >= 0 ? tree.getCell(neighbourCellIndex).ownerTreeId : -1;
          if (neighbourOwner >= 0 and neighbourOwner != _id) {
            const EntityOrderKey orderKey{
              cell.level,
              normal,
              latticeOrderKey(faceLatticeOfSide(tree, cell, faceNumber))
            };
            boundaryEntries.push_back({orderKey, {_rankTopology.faceSlotOfTreeFace[faceIndex], neighbourOwner}});
          }
        }
      }
    }
  );
  const auto faceBoundaryEntries = sortAndStripKeys(faceBoundaryEntriesPerChunk);

  for (const auto& boundaryEntry : vertexBoundaryEntries) {
    _rankTopology.neighbourRanks.push_back(boundaryEntry.second);
  }
  for (const auto& boundaryEntry : faceBoundaryEntries) {
    _rankTopology.neighbourRanks.push_back(boundaryEntry.second);
  }
  std::sort(_rankTopology.neighbourRanks.begin(), _rankTopology.neighbourRanks.end());
  _rankTopology.neighbourRanks.erase(
    std::unique(_rankTopology.neighbourRanks.begin(), _rankTopology.neighbourRanks.end()),
    _rankTopology.neighbourRanks.end()
  );

  const auto buildPackLists = [](
                                const std::vector<std::pair<int, int>>&        boundaryEntries,
                                const std::vector<int>&                        neighbourRanks,
                                const int                                      numberOfLocalSlots,
                                std::vector<std::vector<int>>&                 packLists,
                                std::vector<int>&                              frozenSlotBases,
                                std::vector<std::pair<int, std::vector<int>>>& remotePartners,
                                int&                                           numberOfFrozenSlots
                              ) {
    packLists.resize(neighbourRanks.size());
    frozenSlotBases.resize(neighbourRanks.size());
    for (const auto& [entitySlot, remoteRank] : boundaryEntries) {
      const auto rankNumber =
        std::lower_bound(neighbourRanks.begin(), neighbourRanks.end(), remoteRank) - neighbourRanks.begin();
      packLists[rankNumber].push_back(entitySlot);
    }
    int nextFrozenSlot = numberOfLocalSlots;
    for (std::size_t rankNumber = 0; rankNumber < neighbourRanks.size(); rankNumber++) {
      frozenSlotBases[rankNumber] = nextFrozenSlot;
      nextFrozenSlot += static_cast<int>(packLists[rankNumber].size());
    }
    numberOfFrozenSlots = nextFrozenSlot - numberOfLocalSlots;

    std::map<int, std::vector<int>> partnersOfEntity;
    for (std::size_t rankNumber = 0; rankNumber < neighbourRanks.size(); rankNumber++) {
      for (std::size_t packPosition = 0; packPosition < packLists[rankNumber].size(); packPosition++) {
        partnersOfEntity[packLists[rankNumber][packPosition]].push_back(
          frozenSlotBases[rankNumber] + static_cast<int>(packPosition)
        );
      }
    }
    remotePartners.assign(partnersOfEntity.begin(), partnersOfEntity.end());
  };
  buildPackLists(
    vertexBoundaryEntries,
    _rankTopology.neighbourRanks,
    _rankTopology.vertexSlotSpaceSize,
    _rankTopology.vertexPackLists,
    _rankTopology.vertexFrozenSlotBases,
    _rankTopology.remoteVertexPartners,
    _rankTopology.numberOfVertexFrozenSlots
  );
  buildPackLists(
    faceBoundaryEntries,
    _rankTopology.neighbourRanks,
    _rankTopology.faceSlotSpaceSize,
    _rankTopology.facePackLists,
    _rankTopology.faceFrozenSlotBases,
    _rankTopology.remoteFacePartners,
    _rankTopology.numberOfFaceFrozenSlots
  );
}

/**
 * Extends the slot maps for the entities an in-place append added: Every
 * existing entity and shadow slot keeps its position and every appended local
 * entity gains a fresh slot behind them, so the pools only grow. On a
 * partitioned tree the rank boundary derives anew behind the grown slot
 * ranges, and the returned relocation names the frozen slots that moved.
 */
peano4::topology::TreeTraversal::RankBoundaryRelocation peano4::topology::TreeTraversal::
  extendRankTopologyForAppendedEntities(
    const peano4::topology::Tree&                        tree,
    const peano4::topology::Tree::RefinementDeltaExtent& deltaExtent
  ) {
  assertion(_rankTopology.derived);
  assertion(
    deltaExtent.firstAppendedCell == static_cast<int>(_rankTopology.cellSlotOfTreeCell.size()),
    deltaExtent.firstAppendedCell,
    _rankTopology.cellSlotOfTreeCell.size()
  );

  const int numberOfCells = static_cast<int>(tree.getNumberOfCells());
  _rankTopology.cellSlotOfTreeCell.resize(numberOfCells, -1);
  for (int cellIndex = deltaExtent.firstAppendedCell; cellIndex < numberOfCells; cellIndex++) {
    if (tree.getCell(cellIndex).ownerTreeId == _id) {
      _rankTopology.cellSlotOfTreeCell[cellIndex] = _rankTopology.cellSlotSpaceSize;
      _rankTopology.cellSlotSpaceSize++;
      _rankTopology.numberOfLocalCellSlots++;
    }
  }

  _rankTopology.vertexSlotOfTreeVertex.resize(tree.getNumberOfVertices(), -1);
  _rankTopology.faceSlotOfTreeFace.resize(tree.getNumberOfFaces(), -1);
  // On a partitioned tree the frozen and the shadow slots derive anew behind
  // the grown ranges, so the appended entities take their places and the
  // ranges stay dense; on a single rank both keep their positions.
  const bool reusesFrozenAndShadowSlots = tree.hasDistributedOwnership();
  int        nextVertexSlot             = _rankTopology.vertexSlotSpaceSize;
  int        nextFaceSlot               = _rankTopology.faceSlotSpaceSize;
  if (not reusesFrozenAndShadowSlots) {
    nextVertexSlot += _rankTopology.numberOfVertexFrozenSlots + _interfaceTopology.numberOfVertexShadowSlots;
    nextFaceSlot += _rankTopology.numberOfFaceFrozenSlots + _interfaceTopology.numberOfFaceShadowSlots;
  }
  for (int cellIndex = deltaExtent.firstAppendedCell; cellIndex < numberOfCells; cellIndex++) {
    const auto& cell = tree.getCell(cellIndex);
    if (cell.ownerTreeId != _id) {
      continue;
    }
    for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
      auto& vertexSlot = _rankTopology.vertexSlotOfTreeVertex[cell.vertexIndices[vertexNumber]];
      if (vertexSlot < 0) {
        vertexSlot = nextVertexSlot;
        nextVertexSlot++;
        _rankTopology.numberOfLocalVertexSlots++;
      }
    }
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      auto& faceSlot = _rankTopology.faceSlotOfTreeFace[cell.faceIndices[faceNumber]];
      if (faceSlot < 0) {
        faceSlot = nextFaceSlot;
        nextFaceSlot++;
        _rankTopology.numberOfLocalFaceSlots++;
      }
    }
  }
  _rankTopology.vertexSlotSpaceSize            = nextVertexSlot;
  _rankTopology.faceSlotSpaceSize              = nextFaceSlot;
  _interfaceTopology.numberOfVertexShadowSlots = 0;
  _interfaceTopology.numberOfFaceShadowSlots   = 0;

  _cellIsBlockRoot.resize(tree.getNumberOfCells(), 0);
  _cellIsDirectTask.resize(tree.getNumberOfCells(), 0);

  if (tree.hasDistributedOwnership()) {
    return relocateRankBoundary(tree);
  }
  _rankTopology.numberOfVertexFrozenSlots = 0;
  _rankTopology.numberOfFaceFrozenSlots   = 0;
  return {};
}

/**
 * Derives the rank boundary anew after an in-place prune. Every entity keeps
 * its slot, and the returned relocation names the frozen slots of the
 * surviving rank-boundary entities that moved.
 */
peano4::topology::TreeTraversal::RankBoundaryRelocation peano4::topology::TreeTraversal::rederiveRankBoundary(
  const peano4::topology::Tree& tree
) {
  assertion(_rankTopology.derived);
  assertion(tree.hasDistributedOwnership());
  return relocateRankBoundary(tree);
}

/**
 * Derives the rank boundary anew behind the current slot ranges and pairs
 * every surviving (entity slot, remote rank) entry's old frozen slot with its
 * new one. Both sides of a boundary derive the same entries from the same
 * plan, so the pack lists of the two ranks keep pairing positionally.
 */
peano4::topology::TreeTraversal::RankBoundaryRelocation peano4::topology::TreeTraversal::relocateRankBoundary(
  const peano4::topology::Tree& tree
) {
  const auto oldNeighbourRanks        = _rankTopology.neighbourRanks;
  const auto oldVertexPackLists       = _rankTopology.vertexPackLists;
  const auto oldFacePackLists         = _rankTopology.facePackLists;
  const auto oldVertexFrozenSlotBases = _rankTopology.vertexFrozenSlotBases;
  const auto oldFaceFrozenSlotBases   = _rankTopology.faceFrozenSlotBases;

  deriveRankBoundary(tree);

  const auto pairFrozenSlots = [&oldNeighbourRanks](
                                 const std::vector<std::vector<int>>& oldPackLists,
                                 const std::vector<int>&              oldFrozenSlotBases,
                                 const std::vector<int>&              newNeighbourRanks,
                                 const std::vector<std::vector<int>>& newPackLists,
                                 const std::vector<int>&              newFrozenSlotBases
                               ) {
    std::vector<std::pair<int, int>> pairs;
    for (std::size_t oldRankNumber = 0; oldRankNumber < oldNeighbourRanks.size(); oldRankNumber++) {
      const auto newRankEntry =
        std::find(newNeighbourRanks.begin(), newNeighbourRanks.end(), oldNeighbourRanks[oldRankNumber]);
      if (newRankEntry == newNeighbourRanks.end()) {
        continue;
      }
      const auto  newRankNumber = static_cast<std::size_t>(newRankEntry - newNeighbourRanks.begin());
      const auto& oldPackList   = oldPackLists[oldRankNumber];
      const auto& newPackList   = newPackLists[newRankNumber];
      // Both lists order by the same key, so one merge pass pairs the entries
      // of the entities present in both.
      std::size_t newPosition = 0;
      for (std::size_t oldPosition = 0; oldPosition < oldPackList.size(); oldPosition++) {
        while (newPosition < newPackList.size() and newPackList[newPosition] < oldPackList[oldPosition]) {
          newPosition++;
        }
        if (newPosition < newPackList.size() and newPackList[newPosition] == oldPackList[oldPosition]) {
          pairs.emplace_back(
            oldFrozenSlotBases[oldRankNumber] + static_cast<int>(oldPosition),
            newFrozenSlotBases[newRankNumber] + static_cast<int>(newPosition)
          );
          newPosition++;
        }
      }
    }
    return pairs;
  };
  RankBoundaryRelocation relocation;
  relocation.vertexPairs = pairFrozenSlots(
    oldVertexPackLists,
    oldVertexFrozenSlotBases,
    _rankTopology.neighbourRanks,
    _rankTopology.vertexPackLists,
    _rankTopology.vertexFrozenSlotBases
  );
  relocation.facePairs = pairFrozenSlots(
    oldFacePackLists,
    oldFaceFrozenSlotBases,
    _rankTopology.neighbourRanks,
    _rankTopology.facePackLists,
    _rankTopology.faceFrozenSlotBases
  );
  return relocation;
}

/**
 * Extends the interface topology for the entities an in-place refinement
 * appended, keeping every existing assignment: Units of appended cells follow
 * their parents, an entity several units touch gains the missing touch slots
 * with fresh live and frozen copies behind the slot space, and an entity that
 * only now meets a second unit becomes an interface entity. Every existing
 * touch slot, counter, and pair keeps its number, so the untouched tasks'
 * recordings stay valid. Returns the units whose recordings the
 * delta invalidates: The units of the appended cells, and every toucher of an
 * entity whose touch set or adjacency count changed.
 */
std::vector<int> peano4::topology::TreeTraversal::extendInterfaceTopologyForAppendedEntities(
  const peano4::topology::Tree&                        tree,
  const peano4::topology::Tree::RefinementDeltaExtent& deltaExtent
) {
  assertion(_interfaceTopology.derived);
  assertion(not tree.hasDistributedOwnership());
  const int numberOfCells = static_cast<int>(tree.getNumberOfCells());
  assertion(deltaExtent.firstAppendedCell >= 0 and deltaExtent.firstAppendedCell <= numberOfCells);

  auto& unitOfCell = _interfaceTopology.unitOfCell;
  unitOfCell.resize(numberOfCells, -1);
  for (int cellIndex = deltaExtent.firstAppendedCell; cellIndex < numberOfCells; cellIndex++) {
    const auto parentCellIndex = tree.getCell(cellIndex).parent;
    assertion(parentCellIndex >= 0 and parentCellIndex < cellIndex, cellIndex, parentCellIndex);
    unitOfCell[cellIndex] = (_cellIsDirectTask[parentCellIndex] != 0 or _cellIsBlockRoot[parentCellIndex] != 0)
      ? parentCellIndex
      : unitOfCell[parentCellIndex];
  }

  _interfaceTopology.vertexInterfaceOfSlot.resize(_rankTopology.vertexSlotSpaceSize, -1);
  _interfaceTopology.faceInterfaceOfSlot.resize(_rankTopology.faceSlotSpaceSize, -1);

  // A unit is a root cell's index, so one mark per cell collects the stale
  // units in ascending order.
  std::vector<char> unitIsStale(numberOfCells, 0);
  const auto        markStale = [&unitIsStale](const int unit) {
    assertion(unit >= 0 and static_cast<std::size_t>(unit) < unitIsStale.size(), unit, unitIsStale.size());
    unitIsStale[unit] = 1;
  };

  // One record per corner and side of an appended cell, and one per
  // reclassified vertex and each of its faces with the unit -1: A vertex that
  // flips between hanging and persistent changes the events of every adjacent
  // task, and it and its faces can turn into interface entities between
  // blocks no appended cell touches. Sorted by entity and unit, the records of
  // one entity form a run, and its lattice resolves the existing adjacency.
  struct EntityRecord {
    int                                entityIndex{-1};
    int                                unit{-1};
    int                                level{0};
    int                                normal{0};
    tarch::la::Vector<DIMENSIONS, int> lattice{0};
  };
  const auto recordPrecedes = [](const EntityRecord& left, const EntityRecord& right) {
    return left.entityIndex < right.entityIndex or (left.entityIndex == right.entityIndex and left.unit < right.unit);
  };
  const int                 numberOfAppendedCells = numberOfCells - deltaExtent.firstAppendedCell;
  const int                 numberOfReclassified  = static_cast<int>(deltaExtent.reclassifiedVertices.size());
  std::vector<EntityRecord> vertexRecords;
  std::vector<EntityRecord> faceRecords;
  vertexRecords.reserve(
    static_cast<std::size_t>(numberOfAppendedCells) * peano4::VerticesPerCell + numberOfReclassified
  );
  faceRecords.reserve(
    static_cast<std::size_t>(numberOfAppendedCells) * peano4::FacesPerCell
    + static_cast<std::size_t>(numberOfReclassified) * DIMENSIONS * (peano4::CellsPerVertex / 2)
  );
  for (int cellIndex = deltaExtent.firstAppendedCell; cellIndex < numberOfCells; cellIndex++) {
    const auto& cell = tree.getCell(cellIndex);
    if (cell.ownerTreeId != _id) {
      continue;
    }
    const auto unit = unitOfCell[cellIndex];
    markStale(unit);
    for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
      vertexRecords.push_back(
        {cell.vertexIndices[vertexNumber], unit, cell.level, 0, vertexLatticeOfCorner(tree, cell, vertexNumber)}
      );
    }
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      faceRecords.push_back(
        {cell.faceIndices[faceNumber],
         unit,
         cell.level,
         faceNumber % DIMENSIONS,
         faceLatticeOfSide(tree, cell, faceNumber)}
      );
    }
  }
  for (const auto& reclassifiedVertex : deltaExtent.reclassifiedVertices) {
    vertexRecords.push_back(
      {reclassifiedVertex.vertexIndex, -1, reclassifiedVertex.level, 0, reclassifiedVertex.lattice}
    );
    const int cellsPerAxisOnLevel = peano4::topology::cellsPerAxis(reclassifiedVertex.level);
    for (int normal = 0; normal < DIMENSIONS; normal++) {
      for (int sideCombination = 0; sideCombination < peano4::CellsPerVertex / 2; sideCombination++) {
        bool                               isInsideDomain = true;
        tarch::la::Vector<DIMENSIONS, int> faceLattice(0);
        int                                remainingBits = sideCombination;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          int coordinate = reclassifiedVertex.lattice(axis);
          if (axis != normal) {
            coordinate += remainingBits % 2 - 1;
            remainingBits /= 2;
            if (tree.getPeriodicBoundaries()[axis]) {
              coordinate = (coordinate + cellsPerAxisOnLevel) % cellsPerAxisOnLevel;
            }
            isInsideDomain = isInsideDomain and coordinate >= 0 and coordinate < cellsPerAxisOnLevel;
          }
          faceLattice(axis) = coordinate;
        }
        if (not isInsideDomain) {
          continue;
        }
        const auto faceIndex = tree.getFaceAt(reclassifiedVertex.level, normal, faceLattice);
        if (faceIndex >= 0) {
          faceRecords.push_back({faceIndex, -1, reclassifiedVertex.level, normal, faceLattice});
        }
      }
    }
  }
  std::sort(vertexRecords.begin(), vertexRecords.end(), recordPrecedes);
  std::sort(faceRecords.begin(), faceRecords.end(), recordPrecedes);

  // The touching units of one entity in ascending unit order with the number
  // of adjacent cells per unit.
  struct UnitAdjacency {
    int unit{-1};
    int numberOfAdjacentCells{0};
  };
  using UnitAdjacencies = std::array<UnitAdjacency, peano4::CellsPerVertex>;
  const auto addAdjacentCells =
    [](UnitAdjacencies& adjacencies, int& numberOfUnits, const int unit, const int numberOfAdjacentCells) {
      int position = 0;
      while (position < numberOfUnits and adjacencies[position].unit < unit) {
        position++;
      }
      if (position == numberOfUnits or adjacencies[position].unit != unit) {
        assertion(numberOfUnits < peano4::CellsPerVertex, numberOfUnits, unit);
        for (int shifted = numberOfUnits; shifted > position; shifted--) {
          adjacencies[shifted] = adjacencies[shifted - 1];
        }
        adjacencies[position] = {unit, 0};
        numberOfUnits++;
      }
      adjacencies[position].numberOfAdjacentCells += numberOfAdjacentCells;
    };

  const auto extendEntity = [&](
                              const std::vector<EntityRecord>& records,
                              const std::size_t                firstRecord,
                              const std::size_t                endRecord,
                              const bool                       isVertex,
                              const int                        firstAppendedEntity
                            ) {
    const auto& firstEntityRecord = records[firstRecord];
    const int   entityIndex       = firstEntityRecord.entityIndex;
    const auto  entitySlot =
      isVertex ? _rankTopology.vertexSlotOfTreeVertex[entityIndex] : _rankTopology.faceSlotOfTreeFace[entityIndex];
    assertion(entitySlot >= 0, entityIndex);
    auto& interfaceOfSlot =
      isVertex ? _interfaceTopology.vertexInterfaceOfSlot : _interfaceTopology.faceInterfaceOfSlot;

    // The units of the appended cells that meet the entity, then for a
    // pre-existing entity the units of its existing lattice neighbours.
    UnitAdjacencies adjacentCellsOfUnit{};
    int             numberOfUnits        = 0;
    bool            forcesStaleAdjacency = false;
    for (std::size_t record = firstRecord; record < endRecord; record++) {
#if PEANO_ASSERTIONS
      const auto orderKeyOfRecord      = latticeOrderKey(records[record].lattice);
      const auto orderKeyOfFirstRecord = latticeOrderKey(firstEntityRecord.lattice);
      assertion(orderKeyOfRecord == orderKeyOfFirstRecord, entityIndex, records[record].unit);
#endif
      if (records[record].unit < 0) {
        forcesStaleAdjacency = true;
      } else {
        addAdjacentCells(adjacentCellsOfUnit, numberOfUnits, records[record].unit, 1);
      }
    }
    if (entityIndex < firstAppendedEntity) {
      const int cellsPerAxisOnLevel = peano4::topology::cellsPerAxis(firstEntityRecord.level);
      const int numberOfAdjacencies = isVertex ? peano4::CellsPerVertex : 2;
      for (int adjacencyEntry = 0; adjacencyEntry < numberOfAdjacencies; adjacencyEntry++) {
        bool                               isInsideDomain = true;
        tarch::la::Vector<DIMENSIONS, int> adjacentCellLattice(0);
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          int coordinate = firstEntityRecord.lattice(axis);
          if (isVertex) {
            coordinate += ((adjacencyEntry >> axis) & 1) - 1;
          } else if (axis == firstEntityRecord.normal) {
            coordinate += adjacencyEntry - 1;
          }
          if (tree.getPeriodicBoundaries()[axis]) {
            coordinate = (coordinate + cellsPerAxisOnLevel) % cellsPerAxisOnLevel;
          }
          isInsideDomain            = isInsideDomain and coordinate >= 0 and coordinate < cellsPerAxisOnLevel;
          adjacentCellLattice(axis) = coordinate;
        }
        if (not isInsideDomain) {
          continue;
        }
        const auto adjacentCellIndex = tree.getCellAt(firstEntityRecord.level, adjacentCellLattice);
        if (
          adjacentCellIndex >= 0 and adjacentCellIndex < deltaExtent.firstAppendedCell
          and tree.getCell(adjacentCellIndex).ownerTreeId == _id
        ) {
          addAdjacentCells(adjacentCellsOfUnit, numberOfUnits, unitOfCell[adjacentCellIndex], 1);
        }
      }
    }
    if (forcesStaleAdjacency) {
      for (int position = 0; position < numberOfUnits; position++) {
        markStale(adjacentCellsOfUnit[position].unit);
      }
    }
    if (numberOfUnits <= 1) {
      return;
    }

    auto& numberOfShadowSlots =
      isVertex ? _interfaceTopology.numberOfVertexShadowSlots : _interfaceTopology.numberOfFaceShadowSlots;
    const int slotSpaceSize = isVertex ? _rankTopology.vertexSlotSpaceSize : _rankTopology.faceSlotSpaceSize;
    auto&     copyToFrozenPairs =
      isVertex ? _interfaceTopology.copyToFrozenVertexPairs : _interfaceTopology.copyToFrozenFacePairs;
    auto& seedPairs         = isVertex ? _interfaceTopology.seedVertexPairs : _interfaceTopology.seedFacePairs;
    auto& pendingFlushPairs = isVertex ? _pendingFlushVertexPairs : _pendingFlushFacePairs;

    const auto appendTouchSlot = [&](auto& touchSlots, const int position, const int unit, const int adjacentCells) {
      // reassignInterfaceTouchCounters below renumbers every counter, so the
      // append leaves them alone.
      touchSlots[position].unit                  = unit;
      touchSlots[position].numberOfAdjacentCells = static_cast<std::uint8_t>(adjacentCells);
      touchSlots[position].interfaceSlots        = {
        slotSpaceSize + numberOfShadowSlots,
        slotSpaceSize + numberOfShadowSlots + 1
      };
      numberOfShadowSlots += 2;
      copyToFrozenPairs.emplace_back(
        touchSlots[position].interfaceSlots.live,
        touchSlots[position].interfaceSlots.frozen
      );
      seedPairs.emplace_back(entitySlot, touchSlots[position].interfaceSlots.live);
    };

    // An entity that becomes an interface entity gives every toucher, the
    // existing one included, a touch slot, and every touching task re-records.
    const auto createTouchSlots = [&](auto& touchSlotArrays) {
      interfaceOfSlot[entitySlot] = static_cast<int>(touchSlotArrays.size());
      auto& touchSlots            = touchSlotArrays.emplace_back();
      for (int position = 0; position < numberOfUnits; position++) {
        appendTouchSlot(
          touchSlots,
          position,
          adjacentCellsOfUnit[position].unit,
          adjacentCellsOfUnit[position].numberOfAdjacentCells
        );
        markStale(adjacentCellsOfUnit[position].unit);
      }
      pendingFlushPairs.emplace_back(touchSlots.front().interfaceSlots.live, entitySlot);
    };
    // An interface entity takes its missing touchers in, and every toucher
    // re-records, since the partner set and the adjacency counts of its
    // merges changed.
    const auto updateTouchSlots = [&](auto& touchSlots, const int capacity) {
      int firstFreePosition = 0;
      while (firstFreePosition < capacity and touchSlots[firstFreePosition].unit >= 0) {
        markStale(touchSlots[firstFreePosition].unit);
        firstFreePosition++;
      }
      for (int unitNumber = 0; unitNumber < numberOfUnits; unitNumber++) {
        const auto& [unit, adjacentCells] = adjacentCellsOfUnit[unitNumber];
        bool alreadyListed                = false;
        for (int position = 0; position < firstFreePosition; position++) {
          if (touchSlots[position].unit == unit) {
            touchSlots[position].numberOfAdjacentCells = static_cast<std::uint8_t>(adjacentCells);
            alreadyListed                              = true;
          }
        }
        if (not alreadyListed) {
          assertion(firstFreePosition < capacity, entityIndex, unit);
          appendTouchSlot(touchSlots, firstFreePosition, unit, adjacentCells);
          firstFreePosition++;
          markStale(unit);
        }
      }
    };
    if (interfaceOfSlot[entitySlot] < 0) {
      if (isVertex) {
        createTouchSlots(_interfaceTopology.vertexInterfaceTouchSlots);
      } else {
        createTouchSlots(_interfaceTopology.faceInterfaceTouchSlots);
      }
    } else if (isVertex) {
      updateTouchSlots(
        _interfaceTopology.vertexInterfaceTouchSlots[interfaceOfSlot[entitySlot]],
        peano4::CellsPerVertex
      );
    } else {
      updateTouchSlots(_interfaceTopology.faceInterfaceTouchSlots[interfaceOfSlot[entitySlot]], 2);
    }
  };

  const auto extendEntities =
    [&](const std::vector<EntityRecord>& records, const bool isVertex, const int firstAppendedEntity) {
      std::size_t firstRecord = 0;
      while (firstRecord < records.size()) {
        std::size_t endRecord = firstRecord + 1;
        while (endRecord < records.size() and records[endRecord].entityIndex == records[firstRecord].entityIndex) {
          endRecord++;
        }
        extendEntity(records, firstRecord, endRecord, isVertex, firstAppendedEntity);
        firstRecord = endRecord;
      }
    };
  extendEntities(vertexRecords, true, deltaExtent.firstAppendedVertex);
  extendEntities(faceRecords, false, deltaExtent.firstAppendedFace);
  reassignInterfaceTouchCounters();

  std::vector<int> staleUnits;
  for (int unit = 0; unit < numberOfCells; unit++) {
    if (unitIsStale[unit] != 0) {
      staleUnits.push_back(unit);
    }
  }
  return staleUnits;
}

/**
 * Assigns every interface touch slot its counter in one index space with the
 * plain entities, whose counter is their slot: Slots fill the range below the
 * slot space size, the interface counters follow behind it. The assignment
 * runs again whenever the slot space grows or interface entries disappear, so
 * the two ranges never collide.
 */
void peano4::topology::TreeTraversal::reassignInterfaceTouchCounters() {
  const auto assignTouchCounters = [](auto& interfaceTouchSlots, const int numberOfEntitySlots) {
    int nextCounter = numberOfEntitySlots;
    for (auto& touchSlots : interfaceTouchSlots) {
      for (auto& touchSlot : touchSlots) {
        if (touchSlot.unit < 0) {
          break;
        }
        touchSlot.counter = nextCounter;
        nextCounter++;
      }
    }
    return nextCounter;
  };
  _interfaceTopology.numberOfVertexTouchCounters =
    assignTouchCounters(_interfaceTopology.vertexInterfaceTouchSlots, _rankTopology.vertexSlotSpaceSize);
  _interfaceTopology.numberOfFaceTouchCounters =
    assignTouchCounters(_interfaceTopology.faceInterfaceTouchSlots, _rankTopology.faceSlotSpaceSize);
}

/**
 * Rebuilds the copy-to-frozen, flush, seed and copy-to-entity-slot pair lists from the
 * touch slots as they stand, and drops the deferred flush pairs, which the
 * rebuilt flush list covers.
 */
void peano4::topology::TreeTraversal::rebuildInterfacePairLists() {
  const auto rebuildPairLists = [](
                                  const auto&                       touchSlotArrays,
                                  const std::vector<int>&           interfaceOfSlot,
                                  const int                         slotSpaceSize,
                                  const auto&                       remotePartners,
                                  std::vector<std::pair<int, int>>& copyToFrozenPairs,
                                  std::vector<std::pair<int, int>>& flushPairs,
                                  std::vector<std::pair<int, int>>& seedPairs,
                                  std::vector<std::pair<int, int>>& copyToEntitySlotPairs
                                ) {
    copyToFrozenPairs.clear();
    flushPairs.clear();
    seedPairs.clear();
    copyToEntitySlotPairs.clear();
    for (int entitySlot = 0; entitySlot < slotSpaceSize; entitySlot++) {
      const auto interfaceNumber = interfaceOfSlot[entitySlot];
      if (interfaceNumber < 0) {
        continue;
      }
      const auto& touchSlotsOfUnit = touchSlotArrays[interfaceNumber];
      flushPairs.emplace_back(touchSlotsOfUnit.front().interfaceSlots.live, entitySlot);
      for (const auto& touchSlot : touchSlotsOfUnit) {
        if (touchSlot.unit < 0) {
          break;
        }
        copyToFrozenPairs.emplace_back(touchSlot.interfaceSlots.live, touchSlot.interfaceSlots.frozen);
        seedPairs.emplace_back(entitySlot, touchSlot.interfaceSlots.live);
      }
      if (findRemoteFrozenSlots(remotePartners, entitySlot) != nullptr) {
        copyToEntitySlotPairs.emplace_back(touchSlotsOfUnit.front().interfaceSlots.live, entitySlot);
      }
    }
  };
  rebuildPairLists(
    _interfaceTopology.vertexInterfaceTouchSlots,
    _interfaceTopology.vertexInterfaceOfSlot,
    _rankTopology.vertexSlotSpaceSize,
    _rankTopology.remoteVertexPartners,
    _interfaceTopology.copyToFrozenVertexPairs,
    _interfaceTopology.flushVertexPairs,
    _interfaceTopology.seedVertexPairs,
    _interfaceTopology.creationalCopyToEntitySlotVertexPairs
  );
  rebuildPairLists(
    _interfaceTopology.faceInterfaceTouchSlots,
    _interfaceTopology.faceInterfaceOfSlot,
    _rankTopology.faceSlotSpaceSize,
    _rankTopology.remoteFacePartners,
    _interfaceTopology.copyToFrozenFacePairs,
    _interfaceTopology.flushFacePairs,
    _interfaceTopology.seedFacePairs,
    _interfaceTopology.creationalCopyToEntitySlotFacePairs
  );
  _pendingFlushVertexPairs.clear();
  _pendingFlushFacePairs.clear();
}

/**
 * Marks the unit of every local cell adjacent to one of the given vertices
 * stale. A reclassified vertex flips its hanging state, which changes the
 * events of every adjacent task without moving any touch set.
 */
void peano4::topology::TreeTraversal::markUnitsAdjacentToVertices(
  const peano4::topology::Tree&                                  tree,
  const std::vector<peano4::topology::Tree::ReclassifiedVertex>& vertices,
  std::vector<char>&                                             unitIsStale
) const {
  for (const auto& vertex : vertices) {
    const int cellsPerAxisOnLevel = peano4::topology::cellsPerAxis(vertex.level);
    for (int adjacencyEntry = 0; adjacencyEntry < peano4::CellsPerVertex; adjacencyEntry++) {
      bool                               isInsideDomain = true;
      tarch::la::Vector<DIMENSIONS, int> adjacentCellLattice(0);
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        int coordinate = vertex.lattice(axis) - 1 + ((adjacencyEntry >> axis) & 1);
        if (tree.getPeriodicBoundaries()[axis]) {
          coordinate = (coordinate + cellsPerAxisOnLevel) % cellsPerAxisOnLevel;
        }
        isInsideDomain            = isInsideDomain and coordinate >= 0 and coordinate < cellsPerAxisOnLevel;
        adjacentCellLattice(axis) = coordinate;
      }
      if (not isInsideDomain) {
        continue;
      }
      const auto adjacentCellIndex = tree.getCellAt(vertex.level, adjacentCellLattice);
      if (adjacentCellIndex >= 0 and tree.getCell(adjacentCellIndex).ownerTreeId == _id) {
        const auto adjacentUnit = _interfaceTopology.unitOfCell[adjacentCellIndex];
        if (adjacentUnit >= 0) {
          assertion(static_cast<std::size_t>(adjacentUnit) < unitIsStale.size(), adjacentUnit, unitIsStale.size());
          unitIsStale[adjacentUnit] = 1;
        }
      }
    }
  }
}

/**
 * Returns whether a fresh cut selection on the current tree reproduces the
 * standing task-root marks. The cut selection follows the subtree weights, so
 * a regrid whose delta leaves the heaviest subtrees in place keeps its
 * decomposition, and the kept recordings then match the tasks a rebuild
 * would produce. The marks are restored either way.
 */
bool peano4::topology::TreeTraversal::standingCutsMatchFreshSelection(
  const peano4::topology::Tree& tree,
  const int                     taskLevel
) {
  auto standingBlockRootMarks  = std::move(_cellIsBlockRoot);
  auto standingDirectTaskMarks = std::move(_cellIsDirectTask);
  _cellIsBlockRoot.assign(tree.getNumberOfCells(), 0);
  _cellIsDirectTask.assign(tree.getNumberOfCells(), 0);
  deriveBlockRoots(tree, taskLevel, _interfaceTopology.splitHeavyBlocks, nullptr);
  const bool cutsMatch = _cellIsBlockRoot == standingBlockRootMarks and _cellIsDirectTask == standingDirectTaskMarks;
  _cellIsBlockRoot     = std::move(standingBlockRootMarks);
  _cellIsDirectTask    = std::move(standingDirectTaskMarks);
  return cutsMatch;
}

/**
 * Re-derives the interface topology after an in-place regrid while the task
 * decomposition and the recordings stand: The derivation runs fresh over the
 * patched tree under the restored task-root marks, every surviving touch
 * slot keeps its live and frozen numbers so the kept recordings stay valid,
 * new touch slots allocate above the previous allocation, and the pair lists
 * rebuild from the final slots. Returns the units whose recordings the
 * regrid invalidates, read off the difference between the previous and the
 * fresh derivation plus the vertices the regrid reclassified.
 */
std::vector<int> peano4::topology::TreeTraversal::rederiveInterfaceTopologyForPartialRerecord(
  const peano4::topology::Tree&              tree,
  const peano4::topology::Tree::PruneExtent& pruneExtent
) {
  assertion(_interfaceTopology.derived);
  assertion(not tree.hasDistributedOwnership());
  const auto oldTopology              = std::move(_interfaceTopology);
  _interfaceTopology                  = InterfaceTopology{};
  _interfaceTopology.splitHeavyBlocks = oldTopology.splitHeavyBlocks;
  // Only the membership: The slots below inherit the standing numbering, and
  // the pair lists rebuild from them, so a fresh assignment would be discarded.
  deriveInterfaceMembership(tree);

  const auto remapTouchSlots = [](
                                 auto&                   freshTouchSlotArrays,
                                 const std::vector<int>& freshInterfaceOfSlot,
                                 const auto&             oldTouchSlotArrays,
                                 const std::vector<int>& oldInterfaceOfSlot,
                                 const int               firstShadowSlot,
                                 int&                    numberOfShadowSlots
                               ) {
    int allocationTop = firstShadowSlot;
    for (const auto& oldTouchSlots : oldTouchSlotArrays) {
      for (const auto& oldTouchSlot : oldTouchSlots) {
        if (oldTouchSlot.unit < 0) {
          break;
        }
        allocationTop = std::max(allocationTop, oldTouchSlot.interfaceSlots.frozen + 1);
      }
    }
    for (std::size_t entitySlot = 0; entitySlot < freshInterfaceOfSlot.size(); entitySlot++) {
      const auto freshNumber = freshInterfaceOfSlot[entitySlot];
      if (freshNumber < 0) {
        continue;
      }
      const auto oldNumber = entitySlot < oldInterfaceOfSlot.size() ? oldInterfaceOfSlot[entitySlot] : -1;
      for (auto& freshTouchSlot : freshTouchSlotArrays[freshNumber]) {
        if (freshTouchSlot.unit < 0) {
          break;
        }
        bool retainsOldSlots = false;
        if (oldNumber >= 0) {
          for (const auto& oldTouchSlot : oldTouchSlotArrays[oldNumber]) {
            if (oldTouchSlot.unit < 0) {
              break;
            }
            if (oldTouchSlot.unit == freshTouchSlot.unit) {
              freshTouchSlot.interfaceSlots = oldTouchSlot.interfaceSlots;
              retainsOldSlots               = true;
              break;
            }
          }
        }
        if (not retainsOldSlots) {
          freshTouchSlot.interfaceSlots = {allocationTop, allocationTop + 1};
          allocationTop += 2;
        }
      }
    }
    numberOfShadowSlots = allocationTop - firstShadowSlot;
  };
  remapTouchSlots(
    _interfaceTopology.vertexInterfaceTouchSlots,
    _interfaceTopology.vertexInterfaceOfSlot,
    oldTopology.vertexInterfaceTouchSlots,
    oldTopology.vertexInterfaceOfSlot,
    _rankTopology.vertexSlotSpaceSize + _rankTopology.numberOfVertexFrozenSlots,
    _interfaceTopology.numberOfVertexShadowSlots
  );
  remapTouchSlots(
    _interfaceTopology.faceInterfaceTouchSlots,
    _interfaceTopology.faceInterfaceOfSlot,
    oldTopology.faceInterfaceTouchSlots,
    oldTopology.faceInterfaceOfSlot,
    _rankTopology.faceSlotSpaceSize + _rankTopology.numberOfFaceFrozenSlots,
    _interfaceTopology.numberOfFaceShadowSlots
  );

  rebuildInterfacePairLists();
  _interfaceTopology.derived = true;

  // A unit is a root cell's index, so one mark per cell collects the stale
  // units in ascending order.
  std::vector<char> unitIsStale(std::max(_interfaceTopology.unitOfCell.size(), oldTopology.unitOfCell.size()), 0);
  const auto        markStale = [&unitIsStale](const int unit) {
    assertion(unit >= 0 and static_cast<std::size_t>(unit) < unitIsStale.size(), unit, unitIsStale.size());
    unitIsStale[unit] = 1;
  };
  for (std::size_t cellIndex = 0; cellIndex < _interfaceTopology.unitOfCell.size(); cellIndex++) {
    const auto freshUnit = _interfaceTopology.unitOfCell[cellIndex];
    const auto oldUnit   = cellIndex < oldTopology.unitOfCell.size() ? oldTopology.unitOfCell[cellIndex] : -1;
    if (freshUnit != oldUnit) {
      if (freshUnit >= 0) {
        markStale(freshUnit);
      }
      if (oldUnit >= 0) {
        markStale(oldUnit);
      }
    }
  }
  const auto diffEntityTouches = [&markStale](
                                   const auto&             freshTouchSlotArrays,
                                   const std::vector<int>& freshInterfaceOfSlot,
                                   const auto&             oldTouchSlotArrays,
                                   const std::vector<int>& oldInterfaceOfSlot
                                 ) {
    // The touch slots of an entity are a fixed-capacity array with the unit
    // -1 as its end, and the comparison ignores their order.
    const auto numberOfTouches = [](const auto& touchSlots) {
      int number = 0;
      for (const auto& touchSlot : touchSlots) {
        if (touchSlot.unit < 0) {
          break;
        }
        number++;
      }
      return number;
    };
    const auto holdsEveryTouchOf = [](const auto& touchSlots, const auto& otherTouchSlots) {
      for (const auto& touchSlot : touchSlots) {
        if (touchSlot.unit < 0) {
          break;
        }
        bool found = false;
        for (const auto& otherTouchSlot : otherTouchSlots) {
          if (otherTouchSlot.unit < 0) {
            break;
          }
          if (otherTouchSlot.unit == touchSlot.unit) {
            found = otherTouchSlot.numberOfAdjacentCells == touchSlot.numberOfAdjacentCells;
            break;
          }
        }
        if (not found) {
          return false;
        }
      }
      return true;
    };
    const auto markEveryUnitOf = [&markStale](const auto& touchSlots) {
      for (const auto& touchSlot : touchSlots) {
        if (touchSlot.unit < 0) {
          break;
        }
        markStale(touchSlot.unit);
      }
    };
    const auto numberOfSlots = std::max(freshInterfaceOfSlot.size(), oldInterfaceOfSlot.size());
    for (std::size_t entitySlot = 0; entitySlot < numberOfSlots; entitySlot++) {
      const auto freshNumber = entitySlot < freshInterfaceOfSlot.size() ? freshInterfaceOfSlot[entitySlot] : -1;
      const auto oldNumber   = entitySlot < oldInterfaceOfSlot.size() ? oldInterfaceOfSlot[entitySlot] : -1;
      if (freshNumber < 0 and oldNumber < 0) {
        continue;
      }
      if (freshNumber < 0 or oldNumber < 0) {
        markEveryUnitOf(freshNumber >= 0 ? freshTouchSlotArrays[freshNumber] : oldTouchSlotArrays[oldNumber]);
        continue;
      }
      const auto& freshTouchSlots = freshTouchSlotArrays[freshNumber];
      const auto& oldTouchSlots   = oldTouchSlotArrays[oldNumber];
      if (
        numberOfTouches(freshTouchSlots) != numberOfTouches(oldTouchSlots)
        or not holdsEveryTouchOf(freshTouchSlots, oldTouchSlots)
      ) {
        markEveryUnitOf(freshTouchSlots);
        markEveryUnitOf(oldTouchSlots);
      }
    }
  };
  diffEntityTouches(
    _interfaceTopology.vertexInterfaceTouchSlots,
    _interfaceTopology.vertexInterfaceOfSlot,
    oldTopology.vertexInterfaceTouchSlots,
    oldTopology.vertexInterfaceOfSlot
  );
  diffEntityTouches(
    _interfaceTopology.faceInterfaceTouchSlots,
    _interfaceTopology.faceInterfaceOfSlot,
    oldTopology.faceInterfaceTouchSlots,
    oldTopology.faceInterfaceOfSlot
  );

  markUnitsAdjacentToVertices(tree, pruneExtent.reclassifiedVertices, unitIsStale);

  std::vector<int> staleUnits;
  for (std::size_t unit = 0; unit < unitIsStale.size(); unit++) {
    if (unitIsStale[unit] != 0) {
      staleUnits.push_back(static_cast<int>(unit));
    }
  }
  return staleUnits;
}

/**
 * Shrinks the interface topology for the cells an in-place prune marked dead
 * while the task decomposition and the recordings stand: A dead cell loses
 * its unit, its corners and sides lose that unit's adjacency, a touch slot
 * whose adjacency reaches zero leaves its entity, and an entity with one
 * toucher left leaves the interface; every surviving touch slot keeps its
 * live and frozen numbers, so the kept recordings stay valid. Returns the
 * units whose recordings the prune invalidates: The units of the dead cells,
 * every toucher of an entity whose touch set or adjacency count changed, and
 * the units adjacent to a reclassified vertex. A caller establishes that a
 * fresh cut selection reproduces the standing task roots, so no dead cell is
 * a unit.
 */
std::vector<int> peano4::topology::TreeTraversal::shrinkInterfaceTopologyForPrunedEntities(
  const peano4::topology::Tree&              tree,
  const peano4::topology::Tree::PruneExtent& pruneExtent
) {
  assertion(_interfaceTopology.derived);
  assertion(not tree.hasDistributedOwnership());
#if PEANO_ASSERTIONS
  const auto topologyBeforePrune = _interfaceTopology;
#endif
  auto& unitOfCell = _interfaceTopology.unitOfCell;
  assertion(unitOfCell.size() == tree.getNumberOfCells(), unitOfCell.size(), tree.getNumberOfCells());

  std::vector<char> unitIsStale(unitOfCell.size(), 0);
  const auto        markStale = [&unitIsStale](const int unit) {
    assertion(unit >= 0 and static_cast<std::size_t>(unit) < unitIsStale.size(), unit, unitIsStale.size());
    unitIsStale[unit] = 1;
  };

  // One record per corner and side of a dead cell with the unit the cell
  // held. Sorted by entity and unit, the records of one entity form a run,
  // and the records of one unit within it count that unit's lost
  // adjacencies.
  struct AdjacencyRecord {
    int entitySlot{-1};
    int unit{-1};
  };
  const auto recordPrecedes = [](const AdjacencyRecord& left, const AdjacencyRecord& right) {
    return left.entitySlot < right.entitySlot or (left.entitySlot == right.entitySlot and left.unit < right.unit);
  };
  std::vector<AdjacencyRecord> vertexRecords;
  std::vector<AdjacencyRecord> faceRecords;
  vertexRecords.reserve(pruneExtent.deadCells.size() * peano4::VerticesPerCell);
  faceRecords.reserve(pruneExtent.deadCells.size() * peano4::FacesPerCell);
  for (const auto deadCellIndex : pruneExtent.deadCells) {
    assertion(deadCellIndex >= 0 and static_cast<std::size_t>(deadCellIndex) < unitOfCell.size(), deadCellIndex);
    assertion(_cellIsBlockRoot[deadCellIndex] == 0 and _cellIsDirectTask[deadCellIndex] == 0, deadCellIndex);
    const auto& cell = tree.getCell(deadCellIndex);
    assertion(cell.level < 0, deadCellIndex, cell.level);
    const auto unit           = unitOfCell[deadCellIndex];
    unitOfCell[deadCellIndex] = -1;
    if (unit < 0) {
      continue;
    }
    markStale(unit);
    for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
      const auto vertexSlot = _rankTopology.vertexSlotOfTreeVertex[cell.vertexIndices[vertexNumber]];
      assertion(vertexSlot >= 0 and vertexSlot < _rankTopology.vertexSlotSpaceSize, vertexSlot);
      vertexRecords.push_back({vertexSlot, unit});
    }
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      const auto faceSlot = _rankTopology.faceSlotOfTreeFace[cell.faceIndices[faceNumber]];
      assertion(faceSlot >= 0 and faceSlot < _rankTopology.faceSlotSpaceSize, faceSlot);
      faceRecords.push_back({faceSlot, unit});
    }
  }
  std::sort(vertexRecords.begin(), vertexRecords.end(), recordPrecedes);
  std::sort(faceRecords.begin(), faceRecords.end(), recordPrecedes);

  const auto shrinkEntities = [&markStale](
                                const std::vector<AdjacencyRecord>& records,
                                std::vector<int>&                   interfaceOfSlot,
                                auto&                               touchSlotArrays,
                                const int                           capacity
                              ) {
    std::size_t firstRecord = 0;
    while (firstRecord < records.size()) {
      const auto  entitySlot = records[firstRecord].entitySlot;
      std::size_t endRecord  = firstRecord + 1;
      while (endRecord < records.size() and records[endRecord].entitySlot == entitySlot) {
        endRecord++;
      }
      const auto interfaceNumber = interfaceOfSlot[entitySlot];
      if (interfaceNumber >= 0) {
        auto& touchSlots      = touchSlotArrays[interfaceNumber];
        int   numberOfTouches = 0;
        // Every toucher re-records: The partner set or an adjacency count of
        // its merges changes.
        while (numberOfTouches < capacity and touchSlots[numberOfTouches].unit >= 0) {
          markStale(touchSlots[numberOfTouches].unit);
          numberOfTouches++;
        }
        for (std::size_t record = firstRecord; record < endRecord;) {
          const auto unit            = records[record].unit;
          int        lostAdjacencies = 0;
          while (record < endRecord and records[record].unit == unit) {
            lostAdjacencies++;
            record++;
          }
          int position = 0;
          while (position < numberOfTouches and touchSlots[position].unit != unit) {
            position++;
          }
          assertion(position < numberOfTouches, entitySlot, unit, numberOfTouches);
          assertion(
            touchSlots[position].numberOfAdjacentCells >= lostAdjacencies,
            entitySlot,
            unit,
            touchSlots[position].numberOfAdjacentCells,
            lostAdjacencies
          );
          touchSlots[position].numberOfAdjacentCells =
            static_cast<std::uint8_t>(touchSlots[position].numberOfAdjacentCells - lostAdjacencies);
          if (touchSlots[position].numberOfAdjacentCells == 0) {
            // The removed slot's live and frozen numbers stay unused, and the
            // following slots close the gap.
            for (int shifted = position; shifted + 1 < numberOfTouches; shifted++) {
              touchSlots[shifted] = touchSlots[shifted + 1];
            }
            numberOfTouches--;
            touchSlots[numberOfTouches] = TaskTouchSlot{};
          }
        }
        if (numberOfTouches <= 1) {
          interfaceOfSlot[entitySlot] = -1;
          touchSlots.fill(TaskTouchSlot{});
        }
      }
      firstRecord = endRecord;
    }
  };
  shrinkEntities(
    vertexRecords,
    _interfaceTopology.vertexInterfaceOfSlot,
    _interfaceTopology.vertexInterfaceTouchSlots,
    peano4::CellsPerVertex
  );
  shrinkEntities(faceRecords, _interfaceTopology.faceInterfaceOfSlot, _interfaceTopology.faceInterfaceTouchSlots, 2);
  reassignInterfaceTouchCounters();
  rebuildInterfacePairLists();

  markUnitsAdjacentToVertices(tree, pruneExtent.reclassifiedVertices, unitIsStale);

  std::vector<int> staleUnits;
  for (std::size_t unit = 0; unit < unitIsStale.size(); unit++) {
    if (unitIsStale[unit] != 0) {
      staleUnits.push_back(static_cast<int>(unit));
    }
  }
#if PEANO_ASSERTIONS
  validateShrunkInterfaceTopology(tree, pruneExtent, topologyBeforePrune, staleUnits);
#endif
  return staleUnits;
}

#if PEANO_ASSERTIONS
/**
 * Checks the shrunk interface topology against the full re-derivation from
 * the topology before the prune: The same unit per cell, the same interface
 * entities with the same touching units, adjacency counts and live and
 * frozen slots, and the same stale units. The shrunk topology stands again
 * afterwards.
 */
void peano4::topology::TreeTraversal::validateShrunkInterfaceTopology(
  const peano4::topology::Tree&              tree,
  const peano4::topology::Tree::PruneExtent& pruneExtent,
  const InterfaceTopology&                   topologyBeforePrune,
  const std::vector<int>&                    staleUnits
) {
  auto shrunkTopology        = std::move(_interfaceTopology);
  _interfaceTopology         = topologyBeforePrune;
  const auto freshStaleUnits = rederiveInterfaceTopologyForPartialRerecord(tree, pruneExtent);
  assertion(freshStaleUnits == staleUnits, freshStaleUnits.size(), staleUnits.size());
  assertion(_interfaceTopology.unitOfCell == shrunkTopology.unitOfCell);
  const auto touchesAgree = [](
                              const auto&             freshTouchSlotArrays,
                              const std::vector<int>& freshInterfaceOfSlot,
                              const auto&             shrunkTouchSlotArrays,
                              const std::vector<int>& shrunkInterfaceOfSlot
                            ) {
    assertion(freshInterfaceOfSlot.size() == shrunkInterfaceOfSlot.size());
    for (std::size_t entitySlot = 0; entitySlot < freshInterfaceOfSlot.size(); entitySlot++) {
      const auto freshNumber  = freshInterfaceOfSlot[entitySlot];
      const auto shrunkNumber = shrunkInterfaceOfSlot[entitySlot];
      assertion((freshNumber >= 0) == (shrunkNumber >= 0), entitySlot, freshNumber, shrunkNumber);
      if (freshNumber < 0) {
        continue;
      }
      // The fresh derivation lists the touchers in corner order and an append
      // in unit order, so the lists compare as sets.
      const auto& freshTouchSlots  = freshTouchSlotArrays[freshNumber];
      const auto& shrunkTouchSlots = shrunkTouchSlotArrays[shrunkNumber];
      const auto  numberOfTouches  = [](const auto& touchSlots) {
        std::size_t number = 0;
        while (number < touchSlots.size() and touchSlots[number].unit >= 0) {
          number++;
        }
        return number;
      };
      const auto numberOfFreshTouches  = numberOfTouches(freshTouchSlots);
      const auto numberOfShrunkTouches = numberOfTouches(shrunkTouchSlots);
      assertion(numberOfFreshTouches == numberOfShrunkTouches, entitySlot, numberOfFreshTouches, numberOfShrunkTouches);
      for (const auto& fresh : freshTouchSlots) {
        if (fresh.unit < 0) {
          break;
        }
        const TaskTouchSlot* shrunk = nullptr;
        for (const auto& candidate : shrunkTouchSlots) {
          if (candidate.unit == fresh.unit) {
            shrunk = &candidate;
            break;
          }
        }
        assertion(shrunk != nullptr, entitySlot, fresh.unit);
        assertion(
          fresh.numberOfAdjacentCells == shrunk->numberOfAdjacentCells,
          entitySlot,
          fresh.unit,
          fresh.numberOfAdjacentCells,
          shrunk->numberOfAdjacentCells
        );
        assertion(
          fresh.interfaceSlots.live == shrunk->interfaceSlots.live
            and fresh.interfaceSlots.frozen == shrunk->interfaceSlots.frozen,
          entitySlot,
          fresh.unit,
          fresh.interfaceSlots.live,
          shrunk->interfaceSlots.live
        );
      }
    }
  };
  touchesAgree(
    _interfaceTopology.vertexInterfaceTouchSlots,
    _interfaceTopology.vertexInterfaceOfSlot,
    shrunkTopology.vertexInterfaceTouchSlots,
    shrunkTopology.vertexInterfaceOfSlot
  );
  touchesAgree(
    _interfaceTopology.faceInterfaceTouchSlots,
    _interfaceTopology.faceInterfaceOfSlot,
    shrunkTopology.faceInterfaceTouchSlots,
    shrunkTopology.faceInterfaceOfSlot
  );
  _interfaceTopology = std::move(shrunkTopology);
}
#endif

/**
 * Re-derives the interface topology after an in-place prune. The standing
 * derivation is keyed to the pre-prune structure, so it drops and derives
 * again over the pruned one.
 */
void peano4::topology::TreeTraversal::rederiveInterfaceTopology(
  const peano4::topology::Tree& tree,
  const int                     taskLevel
) {
  _interfaceTopology = InterfaceTopology{};
  _pendingFlushVertexPairs.clear();
  _pendingFlushFacePairs.clear();
  // The derivation reads the task-root marks while it assigns every cell its
  // unit, so the marks of the previous decomposition are cleared first.
  _cellIsBlockRoot.assign(tree.getNumberOfCells(), 0);
  _cellIsDirectTask.assign(tree.getNumberOfCells(), 0);
  _subtreeDecomposition = SubtreeDecomposition{};
  prepareInterfaceTopology(tree, taskLevel);
}

/** Drops any retained recordings, so the next standing sweep records the whole rank. */
void peano4::topology::TreeTraversal::discardRecordingsForFullRerecord() {
  _staleRerecordUnits.clear();
  _partialRerecordPending = false;
  _parallelRecording      = ParallelRecording{};
}

/**
 * Marks the given units' tasks for re-recording while every other recording
 * survives the regrid. Regrids can chain without a standing sweep between
 * them, so the stale sets accumulate until the next standing sweep resolves
 * them.
 */
void peano4::topology::TreeTraversal::retainRecordingsForPartialRerecord(const std::vector<int>& staleUnits) {
  // Without a replayable recording the next standing sweep records everything
  // anyway, so there is nothing to retain.
  if (not _parallelRecording.derived or not _parallelRecording.replaysEntries) {
    return;
  }
  for (const auto staleUnit : staleUnits) {
    if (std::find(_staleRerecordUnits.begin(), _staleRerecordUnits.end(), staleUnit) == _staleRerecordUnits.end()) {
      _staleRerecordUnits.push_back(staleUnit);
    }
  }
  _partialRerecordPending = true;
}

/** Parks the standing decomposition, which a transition's own topology would overwrite. */
void peano4::topology::TreeTraversal::snapshotStandingDecomposition() {
  _snapshotCellIsBlockRoot      = _cellIsBlockRoot;
  _snapshotCellIsDirectTask     = _cellIsDirectTask;
  _snapshotSubtreeDecomposition = _subtreeDecomposition;
}

/** Restores the parked standing decomposition, so the kept recordings match their tasks again. */
void peano4::topology::TreeTraversal::restoreSnapshottedDecomposition() {
  _cellIsBlockRoot      = std::move(_snapshotCellIsBlockRoot);
  _cellIsDirectTask     = std::move(_snapshotCellIsDirectTask);
  _subtreeDecomposition = std::move(_snapshotSubtreeDecomposition);
  _snapshotCellIsBlockRoot.clear();
  _snapshotCellIsDirectTask.clear();
  _snapshotSubtreeDecomposition = SubtreeDecomposition{};
}

/** Joins the deferred flush pairs once the seeding has filled their live copies. */
void peano4::topology::TreeTraversal::commitPendingInterfaceFlushPairs() {
  _interfaceTopology.flushVertexPairs.insert(
    _interfaceTopology.flushVertexPairs.end(),
    _pendingFlushVertexPairs.begin(),
    _pendingFlushVertexPairs.end()
  );
  _interfaceTopology.flushFacePairs
    .insert(_interfaceTopology.flushFacePairs.end(), _pendingFlushFacePairs.begin(), _pendingFlushFacePairs.end());
  _pendingFlushVertexPairs.clear();
  _pendingFlushFacePairs.clear();
}

/**
 * Runs one pool-dialect sweep. Events carry classification sentinels in the
 * data fields and tree entity indices in the pool fields, so the observers
 * move data between their pools and the per-cell call block without any
 * stream.
 */
void peano4::topology::TreeTraversal::traverseWithPools(
  const peano4::topology::Tree&        tree,
  peano4::grid::GridTraversalObserver& observer,
  const PoolSweepKind                  sweepKind,
  const RegridOverrides* const         regridOverrides
) {
  assertRankTopologyMatchesTree(tree);
  if (regridOverrides != nullptr) {
    assertRegridOverridesMatchTree(tree, *regridOverrides);
  }

  _statistics.clear(_id == 0);

  if (sweepKind == PoolSweepKind::Creational or regridOverrides != nullptr) {
    _numberOfStationarySweeps = 0;
    _eventRecording           = EventRecording{};
    _parallelRecording        = ParallelRecording{};
  } else {
    _numberOfStationarySweeps++;
  }

  auto& recording = _eventRecording;

  if (sweepKind == PoolSweepKind::Standing and recording.recorded) {
    observer.beginTraversal(_root.x, _root.h);
    replayEntries(recording.entries, recording.remoteMerges, observer);
    observer.endTraversal(_root.x, _root.h);

    _statistics                          = recording.statistics;
    _statistics.numberOfStationarySweeps = _numberOfStationarySweeps;
    return;
  }

  TraversalScope scope;
  scope.sweepKind       = sweepKind;
  scope.regridOverrides = regridOverrides;
  scope.vertexTouches.assign(_rankTopology.vertexSlotSpaceSize, 0);
  scope.faceTouches.assign(_rankTopology.faceSlotSpaceSize, 0);
  if (regridOverrides != nullptr) {
    _transitionRankBoundaryMerges.clear();
    scope.transitionRankBoundaryMerges = &_transitionRankBoundaryMerges;
  }

  // A transition sweep's classes hold for one sweep only, so it is never
  // recorded.
  if (sweepKind == PoolSweepKind::Standing and regridOverrides == nullptr) {
    recording.entries.clear();
    recording.remoteMerges.clear();
    recording.entries.reserve(2 * tree.getNumberOfCells());
    scope.entries      = &recording.entries;
    scope.inlineMerges = &recording.remoteMerges;
  }

  peano4::grid::GridVertex rootVertices[peano4::VerticesPerCell];
  tree.gatherCellVertices(peano4::topology::Tree::RootCell, rootVertices);

  observer.beginTraversal(_root.x, _root.h);
  descendWithPools(tree, _root, peano4::topology::Tree::RootCell, rootVertices, observer, scope);
  observer.endTraversal(_root.x, _root.h);

  _statistics += scope.statistics;
  _statistics.numberOfStationarySweeps = _numberOfStationarySweeps;

  if (scope.entries != nullptr) {
    recording.statistics = _statistics;
    recording.recorded   = true;
  }
}

/**
 * Derives the block roots both traversal parities use: A spawn-level root
 * preserves the rank partition, a deeper cut splits the heaviest task while
 * the simulated thread schedule gets shorter, and a transition's cuts also
 * divide the changing cells among the tasks. A root's corner vertices are
 * refined in both structures, so a task carries no hanging entity towards its
 * root. A deeper cut descends from the root's parent, leaving the root's data
 * available as coarse-grid data for its children.
 */
void peano4::topology::TreeTraversal::deriveBlockRoots(
  const peano4::topology::Tree& tree,
  const int                     taskLevel,
  const bool                    splitHeavyBlocks,
  const RegridOverrides* const  regridOverrides
) {
  const int numberOfThreads = std::max(tarch::tasking::Runtime::getInstance().getNumberOfThreads(), 1);
  const int blocksPerAxis   = peano4::topology::cellsPerAxis(taskLevel);
  int       numberOfBlocks  = 1;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    numberOfBlocks *= blocksPerAxis;
  }
  std::vector<int> blockRoots;
  for (int blockNumber = 0; blockNumber < numberOfBlocks; blockNumber++) {
    tarch::la::Vector<DIMENSIONS, int> blockLattice(0);
    int                                remainingIndex = blockNumber;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      blockLattice(axis) = remainingIndex % blocksPerAxis;
      remainingIndex /= blocksPerAxis;
    }
    const auto cellIndex = tree.getCellAt(taskLevel, blockLattice);
    if (tree.isSplitBlockAt(blockLattice)) {
      // A split block's ownership is per child, so every rank owning a
      // child roots the whole block and its descent prunes the foreign
      // children. All local children, leaves included, then live inside
      // one subtree task, and the master never overlaps a task inside the
      // block.
      if (tree.subtreeTouchesRank(cellIndex, _id)) {
        _cellIsBlockRoot[cellIndex] = 1;
        blockRoots.push_back(cellIndex);
      }
      continue;
    }
    if (tree.isRefined(cellIndex) and tree.getCell(cellIndex).ownerTreeId == _id) {
      _cellIsBlockRoot[cellIndex] = 1;
      blockRoots.push_back(cellIndex);
    }
  }

  // At spawn level zero the only block root is the domain root, whose event
  // no descent emits. Its local children therefore become direct tasks which
  // start their descent at the root and select one child by unit.
  const bool       createsDirectTasks = taskLevel == 0;
  std::vector<int> directTaskRoots;
  if (createsDirectTasks) {
    for (const auto rootCell : blockRoots) {
      if (not tree.isRefined(rootCell)) {
        continue;
      }
      for (int childPosition = 0; childPosition < ChildrenPerCell; childPosition++) {
        const auto childCellIndex = tree.getChild(rootCell, childPosition);
        if (tree.subtreeTouchesRank(childCellIndex, _id)) {
          _cellIsDirectTask[childCellIndex] = 1;
          directTaskRoots.push_back(childCellIndex);
        }
      }
    }
  }

  if (not splitHeavyBlocks or numberOfThreads <= 1) {
    return;
  }

  struct TaskWeight {
    int numberOfCells{0};
    int numberOfChangingCells{0};
  };
  const auto&             initialTaskRoots = createsDirectTasks ? directTaskRoots : blockRoots;
  std::vector<TaskWeight> subtreeWeights(_rankTopology.cellSlotSpaceSize);
  const auto              deriveSubtreeWeights =
    [this, &tree, &subtreeWeights, regridOverrides](const auto& recurse, const int cellIndex) -> TaskWeight {
    if (not tree.isRefined(cellIndex)) {
      return {};
    }
    TaskWeight weight;
    for (int childPosition = 0; childPosition < ChildrenPerCell; childPosition++) {
      const auto  childCellIndex = tree.getChild(cellIndex, childPosition);
      const auto& childCell      = tree.getCell(childCellIndex);
      if (childCell.level >= tree.getSpawnLevel() and not tree.subtreeTouchesRank(childCellIndex, _id)) {
        continue;
      }
      if (childCell.ownerTreeId == _id) {
        weight.numberOfCells++;
        if (
          regridOverrides != nullptr
          and (regridOverrides->cellCreate[childCellIndex] != 0 or regridOverrides->cellDestroy[childCellIndex] != 0)
        ) {
          weight.numberOfChangingCells++;
        }
      }
      const auto descendantWeight = recurse(recurse, childCellIndex);
      weight.numberOfCells += descendantWeight.numberOfCells;
      weight.numberOfChangingCells += descendantWeight.numberOfChangingCells;
    }
    const auto localSlot = _rankTopology.cellSlotOfTreeCell[cellIndex];
    if (localSlot >= 0) {
      subtreeWeights[localSlot] = weight;
    }
    return weight;
  };

  struct SplitCandidate {
    int        rootCell{-1};
    TaskWeight weight{};
  };
  // The task above a standing task runs the root's events, and the root's
  // corners are refined; a transition task enters its own root, whose cell,
  // faces and corners exist in both structures and none of them hangs. A
  // created, destroyed or hanging entity reads its parent's data on the task
  // above.
  const auto cellCanRootATask = [&tree, regridOverrides](const int cellIndex) {
    if (regridOverrides == nullptr) {
      peano4::grid::GridVertex cornerVertices[peano4::VerticesPerCell];
      tree.gatherCellVertices(cellIndex, cornerVertices);
      return peano4::grid::GridVertex::areAllVerticesRefined(cornerVertices);
    }
    const auto& cell = tree.getCell(cellIndex);
    if (regridOverrides->cellCreate[cellIndex] != 0 or regridOverrides->cellDestroy[cellIndex] != 0) {
      return false;
    }
    for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
      const auto vertexIndex = cell.vertexIndices[vertexNumber];
      if (
        tree.getVertex(vertexIndex).getState() == peano4::grid::GridVertex::State::HangingVertex
        or regridOverrides->vertexCreate[vertexIndex] != 0 or regridOverrides->vertexDestroy[vertexIndex] != 0
      ) {
        return false;
      }
    }
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      const auto faceIndex = cell.faceIndices[faceNumber];
      if (regridOverrides->faceCreate[faceIndex] != 0 or regridOverrides->faceDestroy[faceIndex] != 0) {
        return false;
      }
    }
    return true;
  };
  const auto collectSplitCandidates = [this, &tree, &subtreeWeights, &cellCanRootATask](
                                        const auto&                  recurse,
                                        const int                    cellIndex,
                                        std::vector<SplitCandidate>& candidates
                                      ) -> void {
    if (not tree.isRefined(cellIndex)) {
      return;
    }
    for (int childPosition = 0; childPosition < ChildrenPerCell; childPosition++) {
      const auto  childCellIndex = tree.getChild(cellIndex, childPosition);
      const auto& childCell      = tree.getCell(childCellIndex);
      if (
        not tree.isRefined(childCellIndex)
        or (childCell.level >= tree.getSpawnLevel() and not tree.subtreeTouchesRank(childCellIndex, _id))
      ) {
        continue;
      }
      if (childCell.ownerTreeId == _id and cellCanRootATask(childCellIndex)) {
        const auto localSlot = _rankTopology.cellSlotOfTreeCell[childCellIndex];
        assertion(localSlot >= 0, childCellIndex, childCell.ownerTreeId, _id);
        auto weight = subtreeWeights[localSlot];
        weight.numberOfCells++;
        candidates.push_back({childCellIndex, weight});
      } else {
        recurse(recurse, childCellIndex, candidates);
      }
    }
  };

  struct TaskPlan {
    TaskWeight                  weight{};
    std::vector<SplitCandidate> candidates{};
  };
  constexpr int TaskOverheadInCells             = 10;
  constexpr int MaximumAdditionalTasksPerThread = 24;
  constexpr int ChangingCellCostInCells         = 40;
  const auto    costOf                          = [](const TaskWeight& weight) {
    return weight.numberOfCells + ChangingCellCostInCells * weight.numberOfChangingCells;
  };
  std::vector<TaskPlan> taskPlans;
  taskPlans.reserve(
    initialTaskRoots.size()
    + static_cast<std::size_t>(MaximumAdditionalTasksPerThread) * static_cast<std::size_t>(numberOfThreads)
  );
  taskPlans.resize(initialTaskRoots.size());
  if (not initialTaskRoots.empty()) {
    constexpr int MinimumCellsPerWeightChunk = 4096;
    const int     averageCellsPerRoot        = static_cast<int>(tree.getNumberOfCells() / initialTaskRoots.size());
    const int     minimumRootsPerChunk = std::max(1, MinimumCellsPerWeightChunk / std::max(averageCellsPerRoot, 1));
    peano4::topology::runChunkedTasks(
      static_cast<int>(initialTaskRoots.size()),
      [&](const int chunkNumber, const int begin, const int end) {
        static_cast<void>(chunkNumber);
        for (int taskNumber = begin; taskNumber < end; taskNumber++) {
          const auto rootCell = initialTaskRoots[taskNumber];
          auto&      taskPlan = taskPlans[taskNumber];
          taskPlan.weight     = deriveSubtreeWeights(deriveSubtreeWeights, rootCell);
          if (createsDirectTasks and tree.getCell(rootCell).ownerTreeId == _id) {
            taskPlan.weight.numberOfCells++;
          }
          collectSplitCandidates(collectSplitCandidates, rootCell, taskPlan.candidates);
        }
      },
      minimumRootsPerChunk
    );
  }
  int totalTaskCells     = 0;
  int totalChangingCells = 0;
  for (const auto& taskPlan : taskPlans) {
    totalTaskCells += taskPlan.weight.numberOfCells;
    totalChangingCells += taskPlan.weight.numberOfChangingCells;
  }
  if (totalTaskCells == 0) {
    return;
  }

  const auto maximumNumberOfTasks = taskPlans.size()
    + static_cast<std::size_t>(MaximumAdditionalTasksPerThread) * static_cast<std::size_t>(numberOfThreads);
  // A cut that carries the whole task would leave a walk unit with nothing to
  // emit, and a task root whose rank owns a single child is exactly that case.
  // The task keeps the child and offers the child's own children as its cuts,
  // so the cuts move down a chain of single children until one level branches.
  const auto collectCutsOf =
    [&collectSplitCandidates](const int rootCell, const int taskCells, std::vector<SplitCandidate>& candidates) {
      int cutRoot = rootCell;
      while (true) {
        candidates.clear();
        collectSplitCandidates(collectSplitCandidates, cutRoot, candidates);
        if (candidates.size() != 1 or candidates.front().weight.numberOfCells < taskCells) {
          break;
        }
        cutRoot = candidates.front().rootCell;
      }
    };
  for (std::size_t taskNumber = 0; taskNumber < taskPlans.size(); taskNumber++) {
    collectCutsOf(
      initialTaskRoots[taskNumber],
      taskPlans[taskNumber].weight.numberOfCells,
      taskPlans[taskNumber].candidates
    );
  }
  const auto materialiseCandidate =
    [&collectCutsOf, &taskPlans, this](const int taskNumber, const int candidateNumber) {
      auto&      taskPlan  = taskPlans[taskNumber];
      const auto candidate = taskPlan.candidates[candidateNumber];
      taskPlan.weight.numberOfCells -= candidate.weight.numberOfCells;
      taskPlan.weight.numberOfChangingCells -= candidate.weight.numberOfChangingCells;
      taskPlan.candidates.erase(taskPlan.candidates.begin() + candidateNumber);
      _cellIsDirectTask[candidate.rootCell] = 1;
      if (
        taskPlan.candidates.size() == 1
        and taskPlan.candidates.front().weight.numberOfCells >= taskPlan.weight.numberOfCells
      ) {
        collectCutsOf(taskPlan.candidates.front().rootCell, taskPlan.weight.numberOfCells, taskPlan.candidates);
      }

      TaskPlan newTaskPlan;
      newTaskPlan.weight = candidate.weight;
      collectCutsOf(candidate.rootCell, candidate.weight.numberOfCells, newTaskPlan.candidates);
      taskPlans.push_back(std::move(newTaskPlan));
    };
  const auto bestCandidateOf = [&taskPlans](const int taskNumber, const bool prioritiseChangingCells) {
    const auto& taskPlan               = taskPlans[taskNumber];
    const auto& candidates             = taskPlan.candidates;
    int         candidateToMaterialise = -1;
    for (int candidateNumber = 0; candidateNumber < static_cast<int>(candidates.size()); candidateNumber++) {
      const auto& candidate = candidates[candidateNumber];
      // A cut taking every cell of its task leaves a walk unit with nothing to
      // emit, so the last subtree stays with the task that holds it.
      if (candidate.weight.numberOfCells >= taskPlan.weight.numberOfCells) {
        continue;
      }
      if (candidateToMaterialise < 0) {
        candidateToMaterialise = candidateNumber;
        continue;
      }
      const auto& currentCandidate = candidates[candidateToMaterialise];
      if ((prioritiseChangingCells
           and candidate.weight.numberOfChangingCells > currentCandidate.weight.numberOfChangingCells)
          or (candidate.weight.numberOfChangingCells == currentCandidate.weight.numberOfChangingCells
              and candidate.weight.numberOfCells > currentCandidate.weight.numberOfCells)) {
        candidateToMaterialise = candidateNumber;
      }
    }
    return candidateToMaterialise;
  };
  // Simulates the longest-processing-time schedule of the given task costs:
  // Every task in descending cost order goes to the least loaded thread, and
  // the busiest thread's load is the sweep's wall time.
  const auto makespanOf = [numberOfThreads](std::vector<int>& costs) {
    std::sort(costs.begin(), costs.end(), std::greater<>());
    std::priority_queue<long long, std::vector<long long>, std::greater<>> loads;
    for (int thread = 0; thread < numberOfThreads; thread++) {
      loads.push(0);
    }
    long long makespan = 0;
    for (const auto cost : costs) {
      auto load = loads.top();
      loads.pop();
      load += cost;
      makespan = std::max(makespan, load);
      loads.push(load);
    }
    return makespan;
  };
  // The cuts a task can give up: A cut that took every cell would leave the
  // task with nothing to emit, so the last subtree stays with the task.
  const auto usableCutsOf = [&taskPlans, &costOf](const int taskNumber, std::vector<int>& cutCosts) {
    const auto& taskPlan = taskPlans[taskNumber];
    cutCosts.clear();
    for (const auto& candidate : taskPlan.candidates) {
      if (candidate.weight.numberOfCells < taskPlan.weight.numberOfCells) {
        cutCosts.push_back(costOf(candidate.weight));
      }
    }
  };
  // The threads take the tasks dynamically, so the last task to finish
  // overhangs the schedule by up to its own length: A plan scores its
  // simulated schedule plus half its largest task.
  const auto scoreOf = [&makespanOf](std::vector<int>& costs) {
    const auto makespan = makespanOf(costs);
    return makespan + (costs.empty() ? 0 : costs.front() / 2);
  };
  // Plan k replaces the k heaviest tasks by their cuts and keeps every other
  // task; the plans with k = 1, 2, 4, ... up to the task count are scored.
  // The plan with the lowest score replaces the standing task set, and the
  // pass repeats on the new task set until no plan scores below the standing
  // one.
  while (true) {
    std::vector<int> heaviestFirst(taskPlans.size());
    std::iota(heaviestFirst.begin(), heaviestFirst.end(), 0);
    std::sort(heaviestFirst.begin(), heaviestFirst.end(), [&taskPlans, &costOf](const int left, const int right) {
      return costOf(taskPlans[left].weight) > costOf(taskPlans[right].weight);
    });
    std::vector<int> planCosts;
    planCosts.reserve(maximumNumberOfTasks);
    for (const auto& taskPlan : taskPlans) {
      planCosts.push_back(costOf(taskPlan.weight) + TaskOverheadInCells);
    }
    auto             costs        = planCosts;
    auto             bestScore    = scoreOf(costs);
    int              bestPlan     = 0;
    int              nextScored   = 1;
    std::size_t      plannedTasks = taskPlans.size();
    std::vector<int> cutCosts;
    const auto       numberOfPlans = static_cast<int>(heaviestFirst.size());
    for (int plan = 1; plan <= numberOfPlans; plan++) {
      const auto taskNumber = heaviestFirst[plan - 1];
      usableCutsOf(taskNumber, cutCosts);
      if (not cutCosts.empty()) {
        plannedTasks += cutCosts.size();
        if (plannedTasks > maximumNumberOfTasks) {
          break;
        }
        int remainder = costOf(taskPlans[taskNumber].weight);
        for (const auto cutCost : cutCosts) {
          remainder -= cutCost;
          planCosts.push_back(cutCost + TaskOverheadInCells);
        }
        planCosts[taskNumber] = remainder + TaskOverheadInCells;
      }
      if (plan < nextScored and plan < numberOfPlans) {
        continue;
      }
      nextScored       = 2 * plan;
      costs            = planCosts;
      const auto score = scoreOf(costs);
      if (score < bestScore) {
        bestScore = score;
        bestPlan  = plan;
      }
    }
    if (bestPlan == 0) {
      break;
    }
    for (int plan = 0; plan < bestPlan; plan++) {
      const auto taskNumber = heaviestFirst[plan];
      while (true) {
        const auto candidateNumber = bestCandidateOf(taskNumber, false);
        if (candidateNumber < 0) {
          break;
        }
        materialiseCandidate(taskNumber, candidateNumber);
      }
    }
  }
  const int numberOfDesiredTasks =
    static_cast<int>(std::max(taskPlans.size(), static_cast<std::size_t>(numberOfThreads)));

  if (regridOverrides != nullptr and totalChangingCells > 0) {
    const int targetChangingCells =
      totalChangingCells / numberOfDesiredTasks + (totalChangingCells % numberOfDesiredTasks != 0 ? 1 : 0);
    while (taskPlans.size() < maximumNumberOfTasks) {
      int taskToSplit      = -1;
      int candidateToSplit = -1;
      for (int taskNumber = 0; taskNumber < static_cast<int>(taskPlans.size()); taskNumber++) {
        if (
          taskPlans[taskNumber].weight.numberOfChangingCells <= targetChangingCells
          or (taskToSplit >= 0 and taskPlans[taskNumber].weight.numberOfChangingCells <= taskPlans[taskToSplit].weight.numberOfChangingCells)
        ) {
          continue;
        }
        const auto candidateNumber = bestCandidateOf(taskNumber, true);
        if (candidateNumber >= 0) {
          taskToSplit      = taskNumber;
          candidateToSplit = candidateNumber;
        }
      }
      if (taskToSplit < 0) {
        break;
      }
      materialiseCandidate(taskToSplit, candidateToSplit);
    }
  }

  if (regridOverrides != nullptr) {
    for (int cellIndex = 0; cellIndex < static_cast<int>(tree.getNumberOfCells()); cellIndex++) {
      if (_cellIsBlockRoot[cellIndex] != 0 and tree.getCell(cellIndex).level > 0) {
        _cellIsBlockRoot[cellIndex]  = 0;
        _cellIsDirectTask[cellIndex] = 1;
      }
    }
  } else {
    _subtreeTaskLoad = SubtreeTaskLoad{};
    for (const auto& taskPlan : taskPlans) {
      const auto taskCells = taskPlan.weight.numberOfCells;
      _subtreeTaskLoad.minimumCells =
        _subtreeTaskLoad.numberOfTasks == 0 ? taskCells : std::min(_subtreeTaskLoad.minimumCells, taskCells);
      _subtreeTaskLoad.maximumCells = std::max(_subtreeTaskLoad.maximumCells, taskCells);
      _subtreeTaskLoad.totalCells += taskCells;
      _subtreeTaskLoad.numberOfTasks++;
    }
  }
}

/**
 * Derives the interface topology of the block decomposition from the tree
 * alone: Per entity the touching units, and per (entity, unit) the
 * persistent live and frozen shadow slots behind the frozen slots of the
 * rank boundary. Running this before the first sweep lets the caller size
 * the pools once, so no later replication grows or reallocates them.
 */
void peano4::topology::TreeTraversal::prepareInterfaceTopology(
  const peano4::topology::Tree& tree,
  const int                     taskLevel,
  const bool                    splitHeavyBlocks,
  const RegridOverrides* const  regridOverrides
) {
  assertRankTopologyMatchesTree(tree);
  assertion(taskLevel >= 0 and taskLevel <= tree.getDepth(), taskLevel, tree.getDepth());
  if (regridOverrides != nullptr) {
    assertRegridOverridesMatchTree(tree, *regridOverrides);
  }
  assertion(taskLevel == tree.getSpawnLevel(), taskLevel, tree.getSpawnLevel());
  if (_interfaceTopology.derived) {
    return;
  }

  _interfaceTopology.splitHeavyBlocks = splitHeavyBlocks;
  deriveBlockRoots(tree, taskLevel, splitHeavyBlocks, regridOverrides);
  deriveInterfaceMembership(tree);
  assignInterfaceSlotsAndPairs();
}

/**
 * Derives which units touch each entity, from the task-root marks as they
 * stand: Per cell its unit, per entity the touching units, and the touch
 * counters. A caller that keeps a decomposition derives with its restored
 * marks, so the units match the tasks the recordings were made with. The slots
 * and pair lists follow in assignInterfaceSlotsAndPairs().
 */
void peano4::topology::TreeTraversal::deriveInterfaceMembership(const peano4::topology::Tree& tree) {
  const int  numberOfCells   = static_cast<int>(tree.getNumberOfCells());
  const bool derivesInChunks = peano4::topology::numberOfChunks(numberOfCells) > 1;
  auto&      unitOfCell      = _interfaceTopology.unitOfCell;
  unitOfCell.assign(tree.getNumberOfCells(), -1);
  // The slots of a grid of one cell chunk run as one chunk as well.
  const auto minimumSlotsPerChunk = [derivesInChunks](const int numberOfEntitySlots) {
    return derivesInChunks ? peano4::topology::DefaultMinimumItemsPerChunk : std::max(1, numberOfEntitySlots);
  };

  // Pass one records per entity slot the first unit that touches it, or the
  // candidate mark once a second unit arrives. A cell's unit is the nearest
  // task root at or above it; a chunked derivation walks the ancestors so
  // that no cell waits for its parent's result, a serial one reads its
  // parent, which precedes it.
  std::vector<int> firstUnitOfVertexSlot(_rankTopology.vertexSlotSpaceSize, NoUnitEntry);
  std::vector<int> firstUnitOfFaceSlot(_rankTopology.faceSlotSpaceSize, NoUnitEntry);
  const auto       recordFirstUnit = [derivesInChunks](int& entry, const int unit) {
    if (derivesInChunks) {
      const std::atomic_ref<int> atomicEntry(entry);
      int                        expected = NoUnitEntry;
      if (
        not atomicEntry.compare_exchange_strong(expected, entryOfUnit(unit), std::memory_order_relaxed)
        and expected > NoUnitEntry and expected != entryOfUnit(unit)
      ) {
        atomicEntry.store(CandidateMarkEntry, std::memory_order_relaxed);
      }
    } else if (entry == NoUnitEntry) {
      entry = entryOfUnit(unit);
    } else if (entry > NoUnitEntry and entry != entryOfUnit(unit)) {
      entry = CandidateMarkEntry;
    }
  };
  peano4::topology::runChunkedTasks(numberOfCells, [&](const int chunkNumber, const int begin, const int end) {
    static_cast<void>(chunkNumber);
    for (int cellIndex = begin; cellIndex < end; cellIndex++) {
      const auto& cell = tree.getCell(cellIndex);
      if (cell.parent < 0) {
        continue;
      }
      assertion(cell.parent < cellIndex, cellIndex, cell.parent);
      int unit = cellIndex;
      if (_cellIsDirectTask[cellIndex] != 0) {
        unit = cellIndex;
      } else if (_cellIsBlockRoot[cell.parent] != 0) {
        unit = cell.parent;
      } else if (derivesInChunks) {
        unit = cell.parent;
        while (unit >= 0 and _cellIsBlockRoot[unit] == 0 and _cellIsDirectTask[unit] == 0) {
          unit = tree.getCell(unit).parent;
        }
      } else {
        unit = unitOfCell[cell.parent];
      }
      unitOfCell[cellIndex] = unit;
      if (unit < 0 or cell.ownerTreeId != _id) {
        continue;
      }
      for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
        const auto vertexSlot = _rankTopology.vertexSlotOfTreeVertex[cell.vertexIndices[vertexNumber]];
        assertion(vertexSlot >= 0 and vertexSlot < _rankTopology.vertexSlotSpaceSize, vertexSlot);
        recordFirstUnit(firstUnitOfVertexSlot[vertexSlot], unit);
      }
      for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
        const auto faceSlot = _rankTopology.faceSlotOfTreeFace[cell.faceIndices[faceNumber]];
        assertion(faceSlot >= 0 and faceSlot < _rankTopology.faceSlotSpaceSize, faceSlot);
        recordFirstUnit(firstUnitOfFaceSlot[faceSlot], unit);
      }
    }
  });

  // Pass two numbers the marked slots in slot order: Every chunk counts its
  // marks, and the prefix sums over the chunks place each chunk's candidates.
  const auto numberCandidates = [&minimumSlotsPerChunk](std::vector<int>& firstUnitOfSlot) {
    const int numberOfSlots  = static_cast<int>(firstUnitOfSlot.size());
    const int numberOfChunks = peano4::topology::numberOfChunks(numberOfSlots, minimumSlotsPerChunk(numberOfSlots));
    std::vector<int> firstCandidateOfChunk(numberOfChunks + 1, 0);
    peano4::topology::runChunkedTasks(
      numberOfSlots,
      [&](const int chunkNumber, const int begin, const int end) {
        int numberOfMarks = 0;
        for (int entitySlot = begin; entitySlot < end; entitySlot++) {
          if (firstUnitOfSlot[entitySlot] == CandidateMarkEntry) {
            numberOfMarks++;
          }
        }
        firstCandidateOfChunk[chunkNumber + 1] = numberOfMarks;
      },
      minimumSlotsPerChunk(numberOfSlots)
    );
    for (int chunkNumber = 0; chunkNumber < numberOfChunks; chunkNumber++) {
      firstCandidateOfChunk[chunkNumber + 1] += firstCandidateOfChunk[chunkNumber];
    }
    peano4::topology::runChunkedTasks(
      numberOfSlots,
      [&](const int chunkNumber, const int begin, const int end) {
        int candidate = firstCandidateOfChunk[chunkNumber];
        for (int entitySlot = begin; entitySlot < end; entitySlot++) {
          if (firstUnitOfSlot[entitySlot] == CandidateMarkEntry) {
            firstUnitOfSlot[entitySlot] = entryOfCandidate(candidate);
            candidate++;
          }
        }
        assertion(candidate == firstCandidateOfChunk[chunkNumber + 1], chunkNumber, candidate);
      },
      minimumSlotsPerChunk(numberOfSlots)
    );
    return firstCandidateOfChunk[numberOfChunks];
  };
  const int numberOfVertexCandidates = numberCandidates(firstUnitOfVertexSlot);
  const int numberOfFaceCandidates   = numberCandidates(firstUnitOfFaceSlot);

  // Pass three writes the adjacency records of the candidates, one record per
  // adjacency entry, which the cells of concurrent chunks write disjointly.
  std::vector<AdjacentUnit> vertexRecords(static_cast<std::size_t>(numberOfVertexCandidates) * peano4::CellsPerVertex);
  std::vector<AdjacentUnit> faceRecords(static_cast<std::size_t>(numberOfFaceCandidates) * 2);
  peano4::topology::runChunkedTasks(numberOfCells, [&](const int chunkNumber, const int begin, const int end) {
    static_cast<void>(chunkNumber);
    for (int cellIndex = begin; cellIndex < end; cellIndex++) {
      const auto& cell = tree.getCell(cellIndex);
      const int   unit = unitOfCell[cellIndex];
      if (unit < 0 or cell.ownerTreeId != _id) {
        continue;
      }
      for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
        const auto vertexSlot = _rankTopology.vertexSlotOfTreeVertex[cell.vertexIndices[vertexNumber]];
        const auto entry      = firstUnitOfVertexSlot[vertexSlot];
        if (entry > CandidateMarkEntry) {
          continue;
        }
        assertion(entry < CandidateMarkEntry, vertexSlot, entry);
        const int adjacencyEntry = peano4::CellsPerVertex - 1 - vertexNumber;
        auto&     record =
          vertexRecords[static_cast<std::size_t>(candidateOfEntry(entry)) * peano4::CellsPerVertex + adjacencyEntry];
        assertion(record.unit < 0, vertexSlot, adjacencyEntry, record.unit, unit);
        record = {unit, cellIndex};
      }
      for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
        const auto faceSlot = _rankTopology.faceSlotOfTreeFace[cell.faceIndices[faceNumber]];
        const auto entry    = firstUnitOfFaceSlot[faceSlot];
        if (entry > CandidateMarkEntry) {
          continue;
        }
        assertion(entry < CandidateMarkEntry, faceSlot, entry);
        const int adjacentSide = faceNumber < DIMENSIONS ? 1 : 0;
        auto&     record       = faceRecords[static_cast<std::size_t>(candidateOfEntry(entry)) * 2 + adjacentSide];
        assertion(record.unit < 0, faceSlot, adjacentSide, record.unit, unit);
        record = {unit, cellIndex};
      }
    }
  });

  // An entity below a second unit has no records: The count of one or zero is
  // all the materialisation reads of it. Every chunk lists its interface
  // entities in slot order and the chunks join in chunk order, so the
  // interface numbers follow the slot order.
  const auto gatherTouches = [](
                               const std::vector<int>&                          firstUnitOfSlot,
                               const std::vector<AdjacentUnit>&                 records,
                               const int                                        recordsPerCandidate,
                               const int                                        entitySlot,
                               std::array<EntityTouch, peano4::CellsPerVertex>& touches
                             ) {
    const auto entry = firstUnitOfSlot[entitySlot];
    if (entry > CandidateMarkEntry) {
      return entry > NoUnitEntry ? 1 : 0;
    }
    assertion(entry < CandidateMarkEntry, entitySlot, entry);
    const int numberOfTouches = gatherTouchesFromRecords(
      records.data() + static_cast<std::size_t>(candidateOfEntry(entry)) * recordsPerCandidate,
      recordsPerCandidate,
      touches
    );
    assertion(numberOfTouches >= 2, entitySlot, numberOfTouches);
    return numberOfTouches;
  };
  const auto materialiseInterfaces = [&minimumSlotsPerChunk](
                                       const int         numberOfEntitySlots,
                                       const auto&       gatherTouchesOfSlot,
                                       std::vector<int>& interfaceOfSlot,
                                       auto&             interfaceTouchSlots
                                     ) {
    using TouchSlots = typename std::decay_t<decltype(interfaceTouchSlots)>::value_type;
    interfaceOfSlot.assign(numberOfEntitySlots, -1);
    interfaceTouchSlots.clear();
    std::vector<std::vector<int>>        interfaceSlotsOfChunk(peano4::topology::maximumNumberOfChunks());
    std::vector<std::vector<TouchSlots>> touchSlotsOfChunk(peano4::topology::maximumNumberOfChunks());
    peano4::topology::runChunkedTasks(
      numberOfEntitySlots,
      [&](const int chunkNumber, const int begin, const int end) {
        auto&                                           interfaceSlots        = interfaceSlotsOfChunk[chunkNumber];
        auto&                                           touchSlotsOfInterface = touchSlotsOfChunk[chunkNumber];
        std::array<EntityTouch, peano4::CellsPerVertex> touches{};
        for (int entitySlot = begin; entitySlot < end; entitySlot++) {
          const int numberOfTouches = gatherTouchesOfSlot(entitySlot, touches);
          assertion(numberOfTouches >= 0 and numberOfTouches <= peano4::CellsPerVertex, entitySlot, numberOfTouches);
          if (numberOfTouches <= 1) {
            continue;
          }
          interfaceSlots.push_back(entitySlot);
          auto& touchSlots = touchSlotsOfInterface.emplace_back();
          for (int position = 0; position < numberOfTouches; position++) {
            touchSlots[position].unit                  = touches[position].unit;
            touchSlots[position].numberOfAdjacentCells = touches[position].numberOfAdjacentCells;
          }
        }
      },
      minimumSlotsPerChunk(numberOfEntitySlots)
    );
    for (std::size_t chunkNumber = 0; chunkNumber < interfaceSlotsOfChunk.size(); chunkNumber++) {
      const auto& interfaceSlots = interfaceSlotsOfChunk[chunkNumber];
      for (std::size_t entry = 0; entry < interfaceSlots.size(); entry++) {
        assertion(interfaceOfSlot[interfaceSlots[entry]] < 0, interfaceSlots[entry]);
        interfaceOfSlot[interfaceSlots[entry]] = static_cast<int>(interfaceTouchSlots.size() + entry);
      }
      interfaceTouchSlots.insert(
        interfaceTouchSlots.end(),
        touchSlotsOfChunk[chunkNumber].begin(),
        touchSlotsOfChunk[chunkNumber].end()
      );
    }
  };
  materialiseInterfaces(
    _rankTopology.vertexSlotSpaceSize,
    [&](const int vertexSlot, std::array<EntityTouch, peano4::CellsPerVertex>& touches) {
      return gatherTouches(firstUnitOfVertexSlot, vertexRecords, peano4::CellsPerVertex, vertexSlot, touches);
    },
    _interfaceTopology.vertexInterfaceOfSlot,
    _interfaceTopology.vertexInterfaceTouchSlots
  );
  materialiseInterfaces(
    _rankTopology.faceSlotSpaceSize,
    [&](const int faceSlot, std::array<EntityTouch, peano4::CellsPerVertex>& touches) {
      return gatherTouches(firstUnitOfFaceSlot, faceRecords, 2, faceSlot, touches);
    },
    _interfaceTopology.faceInterfaceOfSlot,
    _interfaceTopology.faceInterfaceTouchSlots
  );

  reassignInterfaceTouchCounters();
}

/**
 * Assigns the live and frozen slot of every touch and builds the copy-to-frozen, flush,
 * seed, and copy-to-entity-slot pair lists from them.
 *
 * A caller that inherits the standing slots derives the membership alone and
 * remaps the slots itself, so this second half stays separate from the
 * membership derivation it follows.
 */
void peano4::topology::TreeTraversal::assignInterfaceSlotsAndPairs() {
  // The task shadow slots start behind the frozen slots of the rank
  // boundary, so both slot families keep their indices for the tree's life.
  const auto assignSlots = [](
                             auto&                             touchSlotsOfUnit,
                             const int                         numberOfEntitySlots,
                             const int                         numberOfRankFrozenSlots,
                             std::vector<std::pair<int, int>>& copyToFrozenPairs,
                             int&                              numberOfShadowSlots
                           ) {
    assertion(numberOfRankFrozenSlots >= 0, numberOfRankFrozenSlots);
    assertion(numberOfEntitySlots >= 0, numberOfEntitySlots);
    if (numberOfEntitySlots > std::numeric_limits<int>::max() - numberOfRankFrozenSlots) {
      tarch::terminateWithError("The interface requires more slots than a 32-bit slot index can represent.");
    }
    const int firstShadowSlot = numberOfEntitySlots + numberOfRankFrozenSlots;
    int       nextSlot        = firstShadowSlot;
    for (auto& touchSlots : touchSlotsOfUnit) {
      for (auto& touchSlot : touchSlots) {
        if (touchSlot.unit < 0) {
          break;
        }
        if (nextSlot > std::numeric_limits<int>::max() - 2) {
          tarch::terminateWithError("The interface requires more slots than a 32-bit slot index can represent.");
        }
        touchSlot.interfaceSlots = {nextSlot, nextSlot + 1};
        nextSlot += 2;
        copyToFrozenPairs.emplace_back(touchSlot.interfaceSlots.live, touchSlot.interfaceSlots.frozen);
      }
    }
    numberOfShadowSlots = nextSlot - firstShadowSlot;
  };
  assignSlots(
    _interfaceTopology.vertexInterfaceTouchSlots,
    _rankTopology.vertexSlotSpaceSize,
    _rankTopology.numberOfVertexFrozenSlots,
    _interfaceTopology.copyToFrozenVertexPairs,
    _interfaceTopology.numberOfVertexShadowSlots
  );
  assignSlots(
    _interfaceTopology.faceInterfaceTouchSlots,
    _rankTopology.faceSlotSpaceSize,
    _rankTopology.numberOfFaceFrozenSlots,
    _interfaceTopology.copyToFrozenFacePairs,
    _interfaceTopology.numberOfFaceShadowSlots
  );

  // A rank-boundary interface entity is packed from its entity slot, which
  // no unit writes, so the creational sweep copies the first toucher's
  // created copy there. The flush pairs move the first toucher's live
  // copy into the entity slot before a regrid migrates it, and the seed
  // pairs move the migrated entity slot back into every toucher's live copy
  // afterwards.
  for (int vertexSlot = 0; vertexSlot < _rankTopology.vertexSlotSpaceSize; vertexSlot++) {
    const auto interfaceNumber = _interfaceTopology.vertexInterfaceOfSlot[vertexSlot];
    if (interfaceNumber < 0) {
      continue;
    }
    const auto& touchSlotsOfUnit = _interfaceTopology.vertexInterfaceTouchSlots[interfaceNumber];
    _interfaceTopology.flushVertexPairs.emplace_back(touchSlotsOfUnit.front().interfaceSlots.live, vertexSlot);
    for (const auto& touchSlot : touchSlotsOfUnit) {
      if (touchSlot.unit < 0) {
        break;
      }
      _interfaceTopology.seedVertexPairs.emplace_back(vertexSlot, touchSlot.interfaceSlots.live);
    }
    if (findRemoteFrozenSlots(_rankTopology.remoteVertexPartners, vertexSlot) != nullptr) {
      _interfaceTopology.creationalCopyToEntitySlotVertexPairs.emplace_back(
        touchSlotsOfUnit.front().interfaceSlots.live,
        vertexSlot
      );
    }
  }
  for (int faceSlot = 0; faceSlot < _rankTopology.faceSlotSpaceSize; faceSlot++) {
    const auto interfaceNumber = _interfaceTopology.faceInterfaceOfSlot[faceSlot];
    if (interfaceNumber < 0) {
      continue;
    }
    const auto& touchSlotsOfUnit = _interfaceTopology.faceInterfaceTouchSlots[interfaceNumber];
    _interfaceTopology.flushFacePairs.emplace_back(touchSlotsOfUnit.front().interfaceSlots.live, faceSlot);
    for (const auto& touchSlot : touchSlotsOfUnit) {
      if (touchSlot.unit < 0) {
        break;
      }
      _interfaceTopology.seedFacePairs.emplace_back(faceSlot, touchSlot.interfaceSlots.live);
    }
    if (findRemoteFrozenSlots(_rankTopology.remoteFacePartners, faceSlot) != nullptr) {
      _interfaceTopology.creationalCopyToEntitySlotFacePairs.emplace_back(
        touchSlotsOfUnit.front().interfaceSlots.live,
        faceSlot
      );
    }
  }
  _interfaceTopology.derived = true;
}

/**
 * Derives the transition of one regrid: The classes of the transition sweep
 * over the union tree, plus the pool slot pairs of both migration hops, old to
 * union before the sweep and union to new after it. An entity only the old
 * structure holds destroys at its last touch, an entity only the new one holds
 * creates at its first touch, and the refined-before and refined-after vertex
 * states feed the events' refinement masks. Hanging entities carry per-visit
 * copies, so they take no classes and never migrate.
 */
peano4::topology::TreeTraversal::RegridTransition peano4::topology::TreeTraversal::deriveRegridTransition(
  const peano4::topology::Tree&                 oldTree,
  const peano4::topology::Tree&                 unionTree,
  const peano4::topology::TreeTraversal&        unionTraversal,
  const peano4::topology::Tree&                 newTree,
  const peano4::topology::TreeTraversal&        newTraversal,
  const peano4::topology::Tree::RefinementPlan& changedFlags,
  const bool                                    partitionMoved,
  const bool                                    unionIsStandingStructure
) const {
  // The old and the new side may be plan-query views without any structure,
  // whose classifications need no slot maps.
  if (oldTree.getNumberOfCells() > 0) {
    assertRankTopologyMatchesTree(oldTree);
  }
  unionTraversal.assertRankTopologyMatchesTree(unionTree);
  if (newTree.getNumberOfCells() > 0) {
    newTraversal.assertRankTopologyMatchesTree(newTree);
  }

  RegridTransition transition;
  transition.overrides.cellCreate.assign(unionTree.getNumberOfCells(), 0);
  transition.overrides.cellDestroy.assign(unionTree.getNumberOfCells(), 0);
  transition.overrides.faceCreate.assign(unionTree.getNumberOfFaces(), 0);
  transition.overrides.faceDestroy.assign(unionTree.getNumberOfFaces(), 0);
  transition.overrides.vertexCreate.assign(unionTree.getNumberOfVertices(), 0);
  transition.overrides.vertexDestroy.assign(unionTree.getNumberOfVertices(), 0);
  transition.overrides.vertexRefinedBefore.assign(unionTree.getNumberOfVertices(), 0);
  transition.overrides.vertexRefinedAfter.assign(unionTree.getNumberOfVertices(), 0);

  const auto faceIsHanging =
    [](const peano4::topology::Tree& tree, const peano4::topology::TreeCell& cell, const int faceNumber) {
      const int normal   = faceNumber % DIMENSIONS;
      const int faceSide = faceNumber < DIMENSIONS ? 0 : 1;
      for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
        if (
          ((vertexNumber >> normal) & 1) == faceSide
          and tree.getVertex(cell.vertexIndices[vertexNumber]).getState()
            != peano4::grid::GridVertex::State::HangingVertex
        ) {
          return false;
        }
      }
      return true;
    };
  // A face is persistent when any vertex on its plane is persistent,
  // whichever adjacent cell carries it in the queried tree.
  const auto faceIsPersistentAt = [](
                                    const peano4::topology::Tree&             tree,
                                    const int                                 level,
                                    const int                                 normal,
                                    const tarch::la::Vector<DIMENSIONS, int>& faceLattice
                                  ) {
    const int cellsPerAxisOnLevel = peano4::topology::cellsPerAxis(level);
    bool      sideCellExists      = false;
    for (int side = 0; side < 2; side++) {
      auto sideLattice = faceLattice;
      sideLattice(normal) += side - 1;
      if (tree.getPeriodicBoundaries()[normal]) {
        sideLattice(normal) = (sideLattice(normal) + cellsPerAxisOnLevel) % cellsPerAxisOnLevel;
      }
      sideCellExists = sideCellExists
        or (sideLattice(normal) >= 0 and sideLattice(normal) < cellsPerAxisOnLevel
            and tree.cellExistsInPlan(level, sideLattice));
    }
    if (not sideCellExists) {
      return false;
    }
    for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
      if (((vertexNumber >> normal) & 1) != 0) {
        continue;
      }
      tarch::la::Vector<DIMENSIONS, int> vertexLattice(0);
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        int coordinate = faceLattice(axis) + (axis == normal ? 0 : ((vertexNumber >> axis) & 1));
        if (tree.getPeriodicBoundaries()[axis] and axis != normal and coordinate == cellsPerAxisOnLevel) {
          coordinate = 0;
        }
        vertexLattice(axis) = coordinate;
      }
      if (tree.isVertexPersistentInPlan(level, vertexLattice)) {
        return true;
      }
    }
    return false;
  };

  // Classifies one union cell and its corners and sides against the old and
  // the new structure. Every classified value depends only on the entity,
  // whichever adjacent cell computes it, so concurrent classifications of one
  // entity write the same value; the atomic visited flags let each entity's
  // classification run once across the chunks.
  const auto classifyCell = [&](
                              RegridOverrides&                overrides,
                              std::vector<std::atomic<char>>& vertexVisited,
                              std::vector<std::atomic<char>>& faceVisited,
                              const int                       cellIndex
                            ) {
    const auto& cell = unionTree.getCell(cellIndex);

    overrides.cellCreate[cellIndex]  = oldTree.cellExistsInPlan(cell.level, cell.latticeIndex) ? 0 : 1;
    overrides.cellDestroy[cellIndex] = newTree.cellExistsInPlan(cell.level, cell.latticeIndex) ? 0 : 1;

    for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
      const auto vertexIndex = cell.vertexIndices[vertexNumber];
      if (vertexVisited[vertexIndex].exchange(1, std::memory_order_relaxed) != 0) {
        continue;
      }
      const auto vertexLattice                   = vertexLatticeOfCorner(unionTree, cell, vertexNumber);
      overrides.vertexRefinedBefore[vertexIndex] = oldTree.isVertexRefinedInPlan(cell.level, vertexLattice) ? 1 : 0;
      overrides.vertexRefinedAfter[vertexIndex]  = newTree.isVertexRefinedInPlan(cell.level, vertexLattice) ? 1 : 0;
      if (unionTree.getVertex(vertexIndex).getState() == peano4::grid::GridVertex::State::HangingVertex) {
        continue;
      }
      overrides.vertexCreate[vertexIndex]  = oldTree.isVertexPersistentInPlan(cell.level, vertexLattice) ? 0 : 1;
      overrides.vertexDestroy[vertexIndex] = newTree.isVertexPersistentInPlan(cell.level, vertexLattice) ? 0 : 1;
    }

    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      const auto faceIndex = cell.faceIndices[faceNumber];
      if (faceVisited[faceIndex].exchange(1, std::memory_order_relaxed) != 0) {
        continue;
      }
      if (faceIsHanging(unionTree, cell, faceNumber)) {
        continue;
      }
      const int  normal                = faceNumber % DIMENSIONS;
      const auto faceLattice           = faceLatticeOfSide(unionTree, cell, faceNumber);
      overrides.faceCreate[faceIndex]  = faceIsPersistentAt(oldTree, cell.level, normal, faceLattice) ? 0 : 1;
      overrides.faceDestroy[faceIndex] = faceIsPersistentAt(newTree, cell.level, normal, faceLattice) ? 0 : 1;
    }
  };

  // Outside the reach of the changed flags an entity keeps its structure:
  // Nothing creates or destroys, and both refinement masks equal the union
  // structure's own vertex state. Only the changed flags' adjacent cells and
  // their descendants can classify differently, so the exact classification
  // runs over that set alone and the derivation cost follows the plan delta.
  std::vector<int>  affectedCells;
  std::vector<char> cellIsAffected;
  {
    auto& overrides = transition.overrides;
    peano4::topology::runChunkedTasks(
      static_cast<int>(unionTree.getNumberOfVertices()),
      [&](const int chunkNumber, const int begin, const int end) {
        static_cast<void>(chunkNumber);
        for (int vertexIndex = begin; vertexIndex < end; vertexIndex++) {
          const char isRefined =
            unionTree.getVertex(vertexIndex).getState() == peano4::grid::GridVertex::State::Refined ? 1 : 0;
          overrides.vertexRefinedBefore[vertexIndex] = isRefined;
          overrides.vertexRefinedAfter[vertexIndex]  = isRefined;
        }
      }
    );

    cellIsAffected.assign(unionTree.getNumberOfCells(), 0);
    const auto addAffectedSubtree = [&](const int seedCellIndex) {
      if (cellIsAffected[seedCellIndex] != 0) {
        return;
      }
      std::vector<int> pendingCells{seedCellIndex};
      cellIsAffected[seedCellIndex] = 1;
      while (not pendingCells.empty()) {
        const auto cellIndex = pendingCells.back();
        pendingCells.pop_back();
        affectedCells.push_back(cellIndex);
        const auto& cell = unionTree.getCell(cellIndex);
        if (cell.childrenBlock >= 0) {
          for (int childNumber = 0; childNumber < ChildrenPerCell; childNumber++) {
            const auto childCellIndex = cell.childrenBlock + childNumber;
            if (cellIsAffected[childCellIndex] == 0) {
              cellIsAffected[childCellIndex] = 1;
              pendingCells.push_back(childCellIndex);
            }
          }
        }
      }
    };
    for (std::size_t planLevel = 0; planLevel < changedFlags.size(); planLevel++) {
      const int level               = unionTree.getDepth() + static_cast<int>(planLevel);
      const int cellsPerAxisOnLevel = peano4::topology::cellsPerAxis(level);
      for (const auto& flaggedVertex : changedFlags[planLevel]) {
        for (int adjacencyEntry = 0; adjacencyEntry < peano4::CellsPerVertex; adjacencyEntry++) {
          bool                               isInsideDomain = true;
          tarch::la::Vector<DIMENSIONS, int> cellLattice(0);
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            int cellCoordinate = flaggedVertex(axis) - 1 + ((adjacencyEntry >> axis) & 1);
            if (unionTree.getPeriodicBoundaries()[axis]) {
              cellCoordinate = (cellCoordinate + cellsPerAxisOnLevel) % cellsPerAxisOnLevel;
            }
            isInsideDomain    = isInsideDomain and cellCoordinate >= 0 and cellCoordinate < cellsPerAxisOnLevel;
            cellLattice(axis) = cellCoordinate;
          }
          const auto cellIndex = isInsideDomain ? unionTree.getCellAt(level, cellLattice) : -1;
          if (cellIndex >= 0) {
            addAffectedSubtree(cellIndex);
          }
        }
      }
    }

    std::vector<std::atomic<char>> vertexVisited(unionTree.getNumberOfVertices());
    std::vector<std::atomic<char>> faceVisited(unionTree.getNumberOfFaces());
    peano4::topology::runChunkedTasks(
      static_cast<int>(affectedCells.size()),
      [&](const int chunkNumber, const int begin, const int end) {
        static_cast<void>(chunkNumber);
        for (int itemNumber = begin; itemNumber < end; itemNumber++) {
          classifyCell(overrides, vertexVisited, faceVisited, affectedCells[itemNumber]);
        }
      }
    );
  }

  // The slot pairs of one migration hop: Every persistent entity of the
  // source tree that persists in the target moves its pool data along. The
  // pairs derive in chunked tasks; the copies they drive target disjoint
  // slots, so the pair order across chunks carries no meaning.
  const auto deriveMigrationPairs = [&](
                                      const peano4::topology::Tree&          sourceTree,
                                      const peano4::topology::TreeTraversal& sourceTraversal,
                                      const peano4::topology::Tree&          targetTree,
                                      const peano4::topology::TreeTraversal& targetTraversal,
                                      std::vector<std::pair<int, int>>&      cellPairs,
                                      std::vector<std::pair<int, int>>&      facePairs,
                                      std::vector<std::pair<int, int>>&      vertexPairs
                                    ) {
    // A target topology that inherited its slots keeps every surviving
    // entity in place, so only the frozen slots, whose bases shift with the
    // slot range, need migration pairs.
    const bool                     identityEntitySlots = targetTraversal._rankTopology.slotsInherited;
    std::vector<std::atomic<char>> vertexVisited(identityEntitySlots ? 0 : sourceTree.getNumberOfVertices());
    std::vector<std::atomic<char>> faceVisited(identityEntitySlots ? 0 : sourceTree.getNumberOfFaces());
    struct ChunkPairs {
      std::vector<std::pair<int, int>> cellPairs{};
      std::vector<std::pair<int, int>> facePairs{};
      std::vector<std::pair<int, int>> vertexPairs{};
    };
    std::vector<ChunkPairs> chunkPairs(peano4::topology::maximumNumberOfChunks());
    peano4::topology::runChunkedTasks(
      identityEntitySlots ? 0 : static_cast<int>(sourceTree.getNumberOfCells()),
      [&](const int chunkNumber, const int begin, const int end) {
        auto& pairs = chunkPairs[chunkNumber];
        for (int cellIndex = begin; cellIndex < end; cellIndex++) {
          const auto& cell = sourceTree.getCell(cellIndex);
          if (cell.level < 0) {
            continue;
          }

          const auto targetCellIndex = targetTree.getCellAt(cell.level, cell.latticeIndex);
          if (targetCellIndex >= 0) {
            const auto sourceSlot = sourceTraversal._rankTopology.cellSlotOfTreeCell[cellIndex];
            const auto targetSlot = targetTraversal._rankTopology.cellSlotOfTreeCell[targetCellIndex];
            if (sourceSlot >= 0 and targetSlot >= 0) {
              pairs.cellPairs.emplace_back(sourceSlot, targetSlot);
            }
          }

          for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
            const auto vertexIndex = cell.vertexIndices[vertexNumber];
            if (vertexVisited[vertexIndex].exchange(1, std::memory_order_relaxed) != 0) {
              continue;
            }
            if (sourceTree.getVertex(vertexIndex).getState() == peano4::grid::GridVertex::State::HangingVertex) {
              continue;
            }
            const auto targetVertexIndex =
              targetTree.getVertexAt(cell.level, vertexLatticeOfCorner(sourceTree, cell, vertexNumber));
            if (
              targetVertexIndex < 0
              or targetTree.getVertex(targetVertexIndex).getState() == peano4::grid::GridVertex::State::HangingVertex
            ) {
              continue;
            }
            const auto sourceSlot = sourceTraversal._rankTopology.vertexSlotOfTreeVertex[vertexIndex];
            const auto targetSlot = targetTraversal._rankTopology.vertexSlotOfTreeVertex[targetVertexIndex];
            if (sourceSlot >= 0 and targetSlot >= 0) {
              pairs.vertexPairs.emplace_back(sourceSlot, targetSlot);
            }
          }

          for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
            const auto faceIndex = cell.faceIndices[faceNumber];
            if (faceVisited[faceIndex].exchange(1, std::memory_order_relaxed) != 0) {
              continue;
            }
            if (faceIsHanging(sourceTree, cell, faceNumber)) {
              continue;
            }
            const int  normal          = faceNumber % DIMENSIONS;
            const auto faceLattice     = faceLatticeOfSide(sourceTree, cell, faceNumber);
            const auto targetFaceIndex = targetTree.getFaceAt(cell.level, normal, faceLattice);
            if (targetFaceIndex < 0 or not faceIsPersistentAt(targetTree, cell.level, normal, faceLattice)) {
              continue;
            }
            const auto sourceSlot = sourceTraversal._rankTopology.faceSlotOfTreeFace[faceIndex];
            const auto targetSlot = targetTraversal._rankTopology.faceSlotOfTreeFace[targetFaceIndex];
            if (sourceSlot >= 0 and targetSlot >= 0) {
              pairs.facePairs.emplace_back(sourceSlot, targetSlot);
            }
          }
        }
      }
    );
    for (const auto& pairs : chunkPairs) {
      cellPairs.insert(cellPairs.end(), pairs.cellPairs.begin(), pairs.cellPairs.end());
      facePairs.insert(facePairs.end(), pairs.facePairs.begin(), pairs.facePairs.end());
      vertexPairs.insert(vertexPairs.end(), pairs.vertexPairs.begin(), pairs.vertexPairs.end());
    }

    // The (entity, remote rank) frozen slots migrate with their entities.
    // Matching the persistent slot pairs against both topologies' pack
    // lists maps every surviving frozen slot onto its counterpart.
    const auto appendFrozenPairs = [identityEntitySlots](
                                     const std::vector<int>&                 sourceNeighbourRanks,
                                     const std::vector<std::vector<int>>&    sourcePackLists,
                                     const std::vector<int>&                 sourceFrozenSlotBases,
                                     const std::vector<int>&                 targetNeighbourRanks,
                                     const std::vector<std::vector<int>>&    targetPackLists,
                                     const std::vector<int>&                 targetFrozenSlotBases,
                                     const int                               numberOfTargetLocalSlots,
                                     const std::vector<std::pair<int, int>>& entityPairs,
                                     std::vector<std::pair<int, int>>&       pairs
                                   ) {
      if (sourceNeighbourRanks.empty() or targetNeighbourRanks.empty()) {
        return;
      }
      std::vector<int> targetSlotOfSourceSlot;
      for (const auto& [sourceSlot, targetSlot] : entityPairs) {
        if (sourceSlot >= static_cast<int>(targetSlotOfSourceSlot.size())) {
          targetSlotOfSourceSlot.resize(sourceSlot + 1, -1);
        }
        targetSlotOfSourceSlot[sourceSlot] = targetSlot;
      }
      std::vector<int> packPositionOfTargetSlot(numberOfTargetLocalSlots, -1);
      for (std::size_t rankNumber = 0; rankNumber < sourceNeighbourRanks.size(); rankNumber++) {
        const auto targetRankEntry =
          std::find(targetNeighbourRanks.begin(), targetNeighbourRanks.end(), sourceNeighbourRanks[rankNumber]);
        if (targetRankEntry == targetNeighbourRanks.end()) {
          continue;
        }
        const auto  targetRankNumber = static_cast<std::size_t>(targetRankEntry - targetNeighbourRanks.begin());
        const auto& targetPackList   = targetPackLists[targetRankNumber];
        for (std::size_t packPosition = 0; packPosition < targetPackList.size(); packPosition++) {
          packPositionOfTargetSlot[targetPackList[packPosition]] = static_cast<int>(packPosition);
        }
        const auto& sourcePackList = sourcePackLists[rankNumber];
        for (std::size_t packPosition = 0; packPosition < sourcePackList.size(); packPosition++) {
          const auto sourceSlot = sourcePackList[packPosition];
          int        targetSlot = -1;
          if (identityEntitySlots) {
            assertion(sourceSlot < numberOfTargetLocalSlots, sourceSlot, numberOfTargetLocalSlots);
            targetSlot = sourceSlot;
          } else if (sourceSlot < static_cast<int>(targetSlotOfSourceSlot.size())) {
            targetSlot = targetSlotOfSourceSlot[sourceSlot];
          }
          if (targetSlot >= 0 and packPositionOfTargetSlot[targetSlot] >= 0) {
            pairs.emplace_back(
              sourceFrozenSlotBases[rankNumber] + static_cast<int>(packPosition),
              targetFrozenSlotBases[targetRankNumber] + packPositionOfTargetSlot[targetSlot]
            );
          }
        }
        for (const auto targetSlot : targetPackList) {
          packPositionOfTargetSlot[targetSlot] = -1;
        }
      }
    };
    appendFrozenPairs(
      sourceTraversal._rankTopology.neighbourRanks,
      sourceTraversal._rankTopology.vertexPackLists,
      sourceTraversal._rankTopology.vertexFrozenSlotBases,
      targetTraversal._rankTopology.neighbourRanks,
      targetTraversal._rankTopology.vertexPackLists,
      targetTraversal._rankTopology.vertexFrozenSlotBases,
      targetTraversal._rankTopology.vertexSlotSpaceSize,
      vertexPairs,
      vertexPairs
    );
    appendFrozenPairs(
      sourceTraversal._rankTopology.neighbourRanks,
      sourceTraversal._rankTopology.facePackLists,
      sourceTraversal._rankTopology.faceFrozenSlotBases,
      targetTraversal._rankTopology.neighbourRanks,
      targetTraversal._rankTopology.facePackLists,
      targetTraversal._rankTopology.faceFrozenSlotBases,
      targetTraversal._rankTopology.faceSlotSpaceSize,
      facePairs,
      facePairs
    );
  };
  if (not unionIsStandingStructure) {
    deriveMigrationPairs(
      oldTree,
      *this,
      unionTree,
      unionTraversal,
      transition.oldToUnionCellPairs,
      transition.oldToUnionFacePairs,
      transition.oldToUnionVertexPairs
    );
  }
  // A transition whose union and final traversal coincide keeps every slot,
  // so nothing migrates between them.
  if (&unionTraversal != &newTraversal) {
    deriveMigrationPairs(
      unionTree,
      unionTraversal,
      newTree,
      newTraversal,
      transition.unionToNewCellPairs,
      transition.unionToNewFacePairs,
      transition.unionToNewVertexPairs
    );
  }

  // A repartition changes owners between the union and the new structure, and
  // only then does anything have to change rank.
  //
  // Every entity whose data has to change rank joins the per-rank transfer
  // lists: The source of a cell is its union owner, the source of a face or
  // vertex is the union owner of its first existing adjacent cell, and a
  // rank receives every entity it touches in the new structure but did not
  // touch in the union.
  if (partitionMoved) {
    // The owner probes classify cells anywhere in the domain, so they answer
    // from the plan and the ownership nodes and never require the cell.
    const auto ownerOfUnionCellAt = [&unionTree](const int level, const tarch::la::Vector<DIMENSIONS, int>& lattice) {
      return unionTree.cellExistsInPlan(level, lattice) ? unionTree.getOwnerOfCellAt(level, lattice) : -1;
    };
    const auto ownerOfNewCellAt = [&newTree](const int level, const tarch::la::Vector<DIMENSIONS, int>& lattice) {
      return newTree.cellExistsInPlan(level, lattice) ? newTree.getOwnerOfCellAt(level, lattice) : -1;
    };

    // The send and receive lists pair positionally across each rank pair, so
    // they order by the entities' level, normal, and lattice key, an order
    // both sides derive from their own views for the entities they exchange.
    using KeyedSlots = std::map<int, std::vector<std::pair<EntityOrderKey, int>>>;
    KeyedSlots keyedSendCellSlots;
    KeyedSlots keyedReceiveCellSlots;
    KeyedSlots keyedSendFaceSlots;
    KeyedSlots keyedReceiveFaceSlots;
    KeyedSlots keyedSendVertexSlots;
    KeyedSlots keyedReceiveVertexSlots;
    const auto emitOrdered = [](KeyedSlots& keyedSlots, std::map<int, std::vector<int>>& slots) {
      for (auto& [remoteRank, entries] : keyedSlots) {
        std::sort(entries.begin(), entries.end());
        auto& slotList = slots[remoteRank];
        slotList.reserve(entries.size());
        for (const auto& [orderKey, slot] : entries) {
          slotList.push_back(slot);
        }
      }
    };

    // An entity changes rank only where a rank touches it under the new
    // owners and not under the union owners, so only a cell whose owner
    // changes or a cell the changed flags affect can carry one.
    const auto markCellsWithChangedOwner = [](
                                             const peano4::topology::Tree& tree,
                                             const auto&                   ownerInOtherStructureAt,
                                             std::vector<char>&            cellIsCandidate,
                                             std::vector<int>&             otherOwnerOfCell
                                           ) {
      const int numberOfCells = static_cast<int>(tree.getNumberOfCells());
      cellIsCandidate.assign(numberOfCells, 0);
      otherOwnerOfCell.assign(numberOfCells, -1);
      peano4::topology::runChunkedTasks(numberOfCells, [&](const int chunkNumber, const int begin, const int end) {
        static_cast<void>(chunkNumber);
        for (int cellIndex = begin; cellIndex < end; cellIndex++) {
          const auto& cell = tree.getCell(cellIndex);
          if (cell.level < 0) {
            continue;
          }
          otherOwnerOfCell[cellIndex] = ownerInOtherStructureAt(cell.level, cell.latticeIndex);
          if (otherOwnerOfCell[cellIndex] != cell.ownerTreeId) {
            cellIsCandidate[cellIndex] = 1;
          }
        }
      });
    };
    std::vector<char> unionCellIsCandidate;
    std::vector<int>  newOwnerOfUnionCell;
    markCellsWithChangedOwner(unionTree, ownerOfNewCellAt, unionCellIsCandidate, newOwnerOfUnionCell);
    std::vector<char> newCellIsCandidate;
    std::vector<int>  unionOwnerOfNewCell;
    markCellsWithChangedOwner(newTree, ownerOfUnionCellAt, newCellIsCandidate, unionOwnerOfNewCell);
    // A cell with no vertex of an affected cell keeps the structure of every
    // entity, so the persistence and the owners of its entities read from the
    // structure. The affected cells cover this rank's view alone, so the
    // coarsening of a face side reads from the plans.
    std::vector<char> unionVertexNearChange(unionTree.getNumberOfVertices(), 0);
    std::vector<char> newVertexNearChange(newTree.getNumberOfVertices(), 0);
    for (const auto cellIndex : affectedCells) {
      unionCellIsCandidate[cellIndex] = 1;
      const auto& cell                = unionTree.getCell(cellIndex);
      for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
        unionVertexNearChange[cell.vertexIndices[vertexNumber]] = 1;
      }
      const auto newCellIndex = newTree.getCellAt(cell.level, cell.latticeIndex);
      if (newCellIndex >= 0) {
        newCellIsCandidate[newCellIndex] = 1;
        const auto& newCell              = newTree.getCell(newCellIndex);
        for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
          newVertexNearChange[newCell.vertexIndices[vertexNumber]] = 1;
        }
      }
    }
    const auto cellKeepsStructure =
      [](const peano4::topology::TreeCell& cell, const std::vector<char>& vertexNearChange) {
        for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
          if (vertexNearChange[cell.vertexIndices[vertexNumber]] != 0) {
            return false;
          }
        }
        return true;
      };
    // The owners of an existing cell in both structures read from the
    // structure that holds it and the table the marking filled; a position
    // outside the materialised blocks answers from the plans.
    const auto ownersOfExistingCellAt = [&unionTree, &newTree, &newOwnerOfUnionCell, &unionOwnerOfNewCell](
                                          const bool                                queriedTreeIsUnion,
                                          const int                                 level,
                                          const tarch::la::Vector<DIMENSIONS, int>& lattice
                                        ) {
      const auto& queriedTree      = queriedTreeIsUnion ? unionTree : newTree;
      const auto& otherOwnerOfCell = queriedTreeIsUnion ? newOwnerOfUnionCell : unionOwnerOfNewCell;
      const auto  cellIndex        = queriedTree.getCellAt(level, lattice);
      if (cellIndex >= 0 and queriedTree.getCell(cellIndex).level >= 0) {
        const auto ownOwner   = queriedTree.getCell(cellIndex).ownerTreeId;
        const auto otherOwner = otherOwnerOfCell[cellIndex];
        return queriedTreeIsUnion ? std::pair<int, int>{ownOwner, otherOwner}
                                  : std::pair<int, int>{otherOwner, ownOwner};
      }
      return std::pair<int, int>{unionTree.getOwnerOfCellAt(level, lattice), newTree.getOwnerOfCellAt(level, lattice)};
    };
    const auto adjacentOwnersOfExistingCells = [&ownersOfExistingCellAt](
                                                 const bool                                queriedTreeIsUnion,
                                                 const int                                 level,
                                                 const tarch::la::Vector<DIMENSIONS, int>& vertexLattice,
                                                 const std::bitset<DIMENSIONS>&            periodicBoundaries,
                                                 std::array<int, peano4::CellsPerVertex>&  unionOwners,
                                                 std::array<int, peano4::CellsPerVertex>&  newOwners
                                               ) {
      const int cellsPerAxisOnLevel = peano4::topology::cellsPerAxis(level);
      for (int adjacencyEntry = 0; adjacencyEntry < peano4::CellsPerVertex; adjacencyEntry++) {
        bool                               isInsideDomain = true;
        tarch::la::Vector<DIMENSIONS, int> adjacentCellLattice(0);
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          int adjacentCellCoordinate = vertexLattice(axis) - 1 + ((adjacencyEntry >> axis) & 1);
          if (periodicBoundaries[axis]) {
            adjacentCellCoordinate = (adjacentCellCoordinate + cellsPerAxisOnLevel) % cellsPerAxisOnLevel;
          }
          isInsideDomain =
            isInsideDomain and adjacentCellCoordinate >= 0 and adjacentCellCoordinate < cellsPerAxisOnLevel;
          adjacentCellLattice(axis) = adjacentCellCoordinate;
        }
        if (isInsideDomain) {
          const auto [unionOwner, newOwner] = ownersOfExistingCellAt(queriedTreeIsUnion, level, adjacentCellLattice);
          unionOwners[adjacencyEntry]       = unionOwner;
          newOwners[adjacencyEntry]         = newOwner;
        } else {
          unionOwners[adjacencyEntry] = -1;
          newOwners[adjacencyEntry]   = -1;
        }
      }
    };
    const auto faceHasPersistentVertex =
      [](const peano4::topology::Tree& tree, const peano4::topology::TreeCell& cell, const int faceNumber) {
        const int normal   = faceNumber % DIMENSIONS;
        const int faceSide = faceNumber < DIMENSIONS ? 0 : 1;
        for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
          if (
            ((vertexNumber >> normal) & 1) == faceSide
            and tree.getVertex(cell.vertexIndices[vertexNumber]).getState()
              != peano4::grid::GridVertex::State::HangingVertex
          ) {
            return true;
          }
        }
        return false;
      };
    const auto candidateCellsOf = [](const std::vector<char>& cellIsCandidate) {
      std::vector<int> candidateCells;
      for (int cellIndex = 0; cellIndex < static_cast<int>(cellIsCandidate.size()); cellIndex++) {
        if (cellIsCandidate[cellIndex] != 0) {
          candidateCells.push_back(cellIndex);
        }
      }
      return candidateCells;
    };
    const auto unionCandidateCells = candidateCellsOf(unionCellIsCandidate);
    const auto newCandidateCells   = candidateCellsOf(newCellIsCandidate);

    // Sends derive from the union structure, where the sender holds every
    // cell it owns, and receives from the new structure, where the receiver
    // holds every cell it acquires. The classifications answer from the plan
    // and the ownership nodes, so both sides list the exchanged entities
    // identically however far an entity migrates.
    for (const auto cellIndex : unionCandidateCells) {
      const auto& cell = unionTree.getCell(cellIndex);
      if (cell.ownerTreeId != _id) {
        continue;
      }
      const auto newOwner = ownerOfNewCellAt(cell.level, cell.latticeIndex);
      if (newOwner < 0 or newOwner == _id) {
        continue;
      }
      keyedSendCellSlots[newOwner].emplace_back(
        EntityOrderKey{cell.level, 0, latticeOrderKey(cell.latticeIndex)},
        unionTraversal._rankTopology.cellSlotOfTreeCell[cellIndex]
      );
    }
    for (const auto cellIndex : newCandidateCells) {
      const auto& cell = newTree.getCell(cellIndex);
      if (cell.ownerTreeId != _id) {
        continue;
      }
      const auto unionOwner = ownerOfUnionCellAt(cell.level, cell.latticeIndex);
      if (unionOwner < 0 or unionOwner == _id) {
        continue;
      }
      keyedReceiveCellSlots[unionOwner].emplace_back(
        EntityOrderKey{cell.level, 0, latticeOrderKey(cell.latticeIndex)},
        newTraversal._rankTopology.cellSlotOfTreeCell[cellIndex]
      );
    }

    const auto adjacentCellOwners = [](
                                      const auto&                               ownerOfCellAt,
                                      const int                                 level,
                                      const tarch::la::Vector<DIMENSIONS, int>& vertexLattice,
                                      const std::bitset<DIMENSIONS>&            periodicBoundaries
                                    ) {
      std::array<int, peano4::CellsPerVertex> owners{};
      const int                               cellsPerAxisOnLevel = peano4::topology::cellsPerAxis(level);
      for (int adjacencyEntry = 0; adjacencyEntry < peano4::CellsPerVertex; adjacencyEntry++) {
        bool                               isInsideDomain = true;
        tarch::la::Vector<DIMENSIONS, int> adjacentCellLattice(0);
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          int adjacentCellCoordinate = vertexLattice(axis) - 1 + ((adjacencyEntry >> axis) & 1);
          if (periodicBoundaries[axis]) {
            adjacentCellCoordinate = (adjacentCellCoordinate + cellsPerAxisOnLevel) % cellsPerAxisOnLevel;
          }
          isInsideDomain =
            isInsideDomain and adjacentCellCoordinate >= 0 and adjacentCellCoordinate < cellsPerAxisOnLevel;
          adjacentCellLattice(axis) = adjacentCellCoordinate;
        }
        owners[adjacencyEntry] = isInsideDomain ? ownerOfCellAt(level, adjacentCellLattice) : -1;
      }
      return owners;
    };

    // Every entity of a cell inside a block that one rank owns in the union
    // and one rank owns in the new structure changes rank with the block, so
    // its owners read once per cell and the persistence per entity.
    struct UniformBlockOwners {
      bool uniform{false};
      int  unionOwner{-1};
      int  newOwner{-1};
    };
    const int  spawnLevel = unionTree.getSpawnLevel();
    const auto uniformBlockOwnersOf =
      [&unionTree, &newTree, spawnLevel](const peano4::topology::Tree& tree, const peano4::topology::TreeCell& cell) {
        UniformBlockOwners owners;
        if (cell.level <= spawnLevel) {
          return owners;
        }
        const int cellsPerAxisOfBlock =
          tree.getCellsPerAxisOfLevel(cell.level) / tree.getCellsPerAxisOfLevel(spawnLevel);
        tarch::la::Vector<DIMENSIONS, int> blockLattice(0);
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          const int coordinateInBlock = cell.latticeIndex(axis) % cellsPerAxisOfBlock;
          if (coordinateInBlock == 0 or coordinateInBlock == cellsPerAxisOfBlock - 1) {
            return owners;
          }
          blockLattice(axis) = cell.latticeIndex(axis) / cellsPerAxisOfBlock;
        }
        if (unionTree.isSplitBlockAt(blockLattice) or newTree.isSplitBlockAt(blockLattice)) {
          return owners;
        }
        owners.uniform    = true;
        owners.unionOwner = unionTree.getOwnerOfCellAt(spawnLevel, blockLattice);
        owners.newOwner   = newTree.getOwnerOfCellAt(spawnLevel, blockLattice);
        return owners;
      };
    const auto listEntitiesOfUniformCell = [&newTree](
                                             const peano4::topology::Tree&                      tree,
                                             const peano4::topology::TreeCell&                  cell,
                                             const std::vector<int>&                            vertexSlotOfTreeVertex,
                                             const std::vector<int>&                            faceSlotOfTreeFace,
                                             std::vector<char>&                                 vertexVisited,
                                             std::vector<char>&                                 faceVisited,
                                             std::vector<std::pair<EntityOrderKey, int>>* const vertexSlots,
                                             std::vector<std::pair<EntityOrderKey, int>>* const faceSlots
                                           ) {
      for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
        const auto vertexIndex = cell.vertexIndices[vertexNumber];
        if (vertexVisited[vertexIndex] != 0) {
          continue;
        }
        vertexVisited[vertexIndex] = 1;
        const auto vertexLattice   = vertexLatticeOfCorner(tree, cell, vertexNumber);
        if (vertexSlots != nullptr and newTree.isVertexPersistentInPlan(cell.level, vertexLattice)) {
          vertexSlots->emplace_back(
            EntityOrderKey{cell.level, 0, latticeOrderKey(vertexLattice)},
            vertexSlotOfTreeVertex[vertexIndex]
          );
        }
      }
      for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
        const auto faceIndex = cell.faceIndices[faceNumber];
        if (faceVisited[faceIndex] != 0) {
          continue;
        }
        faceVisited[faceIndex] = 1;
        if (faceSlots == nullptr) {
          continue;
        }
        const int normal            = faceNumber % DIMENSIONS;
        const int faceSide          = faceNumber < DIMENSIONS ? 0 : 1;
        bool      facePersistsInNew = false;
        for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
          if (
            ((vertexNumber >> normal) & 1) == faceSide
            and newTree.isVertexPersistentInPlan(cell.level, vertexLatticeOfCorner(tree, cell, vertexNumber))
          ) {
            facePersistsInNew = true;
          }
        }
        if (facePersistsInNew) {
          faceSlots->emplace_back(
            EntityOrderKey{cell.level, normal, latticeOrderKey(faceLatticeOfSide(tree, cell, faceNumber))},
            faceSlotOfTreeFace[faceIndex]
          );
        }
      }
    };

    std::vector<char> vertexVisited(unionTree.getNumberOfVertices(), 0);
    std::vector<char> faceVisited(unionTree.getNumberOfFaces(), 0);
    for (const auto cellIndex : unionCandidateCells) {
      const auto& cell          = unionTree.getCell(cellIndex);
      const auto  uniformOwners = uniformBlockOwnersOf(unionTree, cell);
      if (uniformOwners.uniform) {
        const bool sends = uniformOwners.unionOwner == _id and uniformOwners.newOwner != _id;
        listEntitiesOfUniformCell(
          unionTree,
          cell,
          unionTraversal._rankTopology.vertexSlotOfTreeVertex,
          unionTraversal._rankTopology.faceSlotOfTreeFace,
          vertexVisited,
          faceVisited,
          sends ? &keyedSendVertexSlots[uniformOwners.newOwner] : nullptr,
          sends ? &keyedSendFaceSlots[uniformOwners.newOwner] : nullptr
        );
        continue;
      }
      for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
        const auto vertexIndex = cell.vertexIndices[vertexNumber];
        if (vertexVisited[vertexIndex] != 0) {
          continue;
        }
        vertexVisited[vertexIndex] = 1;
        const auto vertexLattice   = vertexLatticeOfCorner(unionTree, cell, vertexNumber);
        const bool keepsStructure  = cellKeepsStructure(cell, unionVertexNearChange);
        if (
          keepsStructure ? unionTree.getVertex(vertexIndex).getState() == peano4::grid::GridVertex::State::HangingVertex
                         : not newTree.isVertexPersistentInPlan(cell.level, vertexLattice)
        ) {
          continue;
        }
        std::array<int, peano4::CellsPerVertex> newOwners{};
        std::array<int, peano4::CellsPerVertex> unionOwners{};
        if (keepsStructure) {
          adjacentOwnersOfExistingCells(
            true,
            cell.level,
            vertexLattice,
            unionTree.getPeriodicBoundaries(),
            unionOwners,
            newOwners
          );
        } else {
          newOwners = adjacentCellOwners(ownerOfNewCellAt, cell.level, vertexLattice, newTree.getPeriodicBoundaries());
          unionOwners =
            adjacentCellOwners(ownerOfUnionCellAt, cell.level, vertexLattice, unionTree.getPeriodicBoundaries());
        }
        int sourceRank = -1;
        for (const auto owner : unionOwners) {
          if (owner >= 0) {
            sourceRank = owner;
            break;
          }
        }
        for (int adjacencyEntry = 0; adjacencyEntry < peano4::CellsPerVertex; adjacencyEntry++) {
          const auto toucher = newOwners[adjacencyEntry];
          if (
            toucher < 0 or sourceRank < 0
            or std::find(unionOwners.begin(), unionOwners.end(), toucher) != unionOwners.end()
            or std::find(newOwners.begin(), newOwners.begin() + adjacencyEntry, toucher)
              != newOwners.begin() + adjacencyEntry
          ) {
            continue;
          }
          if (sourceRank == _id and toucher != _id) {
            keyedSendVertexSlots[toucher].emplace_back(
              EntityOrderKey{cell.level, 0, latticeOrderKey(vertexLattice)},
              unionTraversal._rankTopology.vertexSlotOfTreeVertex[vertexIndex]
            );
          }
        }
      }
      for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
        const auto faceIndex = cell.faceIndices[faceNumber];
        if (faceVisited[faceIndex] != 0) {
          continue;
        }
        faceVisited[faceIndex]       = 1;
        const int  normal            = faceNumber % DIMENSIONS;
        const int  faceSide          = faceNumber < DIMENSIONS ? 0 : 1;
        const bool keepsStructure    = cellKeepsStructure(cell, unionVertexNearChange);
        bool       facePersistsInNew = keepsStructure and faceHasPersistentVertex(unionTree, cell, faceNumber);
        for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell and not keepsStructure; vertexNumber++) {
          if (
            ((vertexNumber >> normal) & 1) == faceSide
            and newTree.isVertexPersistentInPlan(cell.level, vertexLatticeOfCorner(unionTree, cell, vertexNumber))
          ) {
            facePersistsInNew = true;
          }
        }
        if (not facePersistsInNew) {
          continue;
        }
        const auto          faceLattice = faceLatticeOfSide(unionTree, cell, faceNumber);
        std::array<int, 2>  newOwners{};
        std::array<int, 2>  unionOwners{};
        std::array<bool, 2> sideCoarsens{};
        for (int side = 0; side < 2; side++) {
          auto sideLattice = faceLattice;
          sideLattice(normal) += side - 1;
          const int cellsPerAxisOnLevel = peano4::topology::cellsPerAxis(cell.level);
          if (newTree.getPeriodicBoundaries()[normal]) {
            sideLattice(normal) = (sideLattice(normal) + cellsPerAxisOnLevel) % cellsPerAxisOnLevel;
          }
          const bool isInsideDomain = sideLattice(normal) >= 0 and sideLattice(normal) < cellsPerAxisOnLevel;
          // The coarsening reads from the plans, which every rank holds in
          // full, so a rank whose view lacks the coarsening cell derives the
          // same source rank as the rank that holds it.
          sideCoarsens[side] = isInsideDomain and unionTree.isCellRefinedInPlan(cell.level, sideLattice)
            and newTree.cellExistsInPlan(cell.level, sideLattice)
            and not newTree.isCellRefinedInPlan(cell.level, sideLattice);
          if (keepsStructure) {
            const bool sideExists = isInsideDomain and unionTree.cellExistsInPlan(cell.level, sideLattice);
            const auto [unionOwner, newOwner] =
              sideExists ? ownersOfExistingCellAt(true, cell.level, sideLattice) : std::pair<int, int>{-1, -1};
            newOwners[side]   = newOwner;
            unionOwners[side] = unionOwner;
            continue;
          }
          newOwners[side]   = isInsideDomain ? ownerOfNewCellAt(cell.level, sideLattice) : -1;
          unionOwners[side] = isInsideDomain ? ownerOfUnionCellAt(cell.level, sideLattice) : -1;
        }
        // A face with exactly one coarsening side becomes a resolution
        // transition, whose whole slot ships to every rank that touches the
        // face under the new owners, the union owners included. Every write
        // the transition sweep makes to it lands on that side's union owner,
        // and no rank-boundary merge repairs the others, because the merge
        // needs both sides unrefined and this face keeps one refined side.
        if (sideCoarsens[0] != sideCoarsens[1] and unionOwners[sideCoarsens[0] ? 0 : 1] >= 0) {
          const int faceSourceRank = unionOwners[sideCoarsens[0] ? 0 : 1];
          for (int side = 0; side < 2; side++) {
            const auto toucher = newOwners[side];
            if (toucher < 0 or toucher == faceSourceRank or (side == 1 and toucher == newOwners[0])) {
              continue;
            }
            if (faceSourceRank == _id and toucher != _id) {
              keyedSendFaceSlots[toucher].emplace_back(
                EntityOrderKey{cell.level, normal, latticeOrderKey(faceLattice)},
                unionTraversal._rankTopology.faceSlotOfTreeFace[faceIndex]
              );
            }
          }
          continue;
        }
        const int sourceRank = unionOwners[0] >= 0 ? unionOwners[0] : unionOwners[1];
        for (int side = 0; side < 2; side++) {
          const auto toucher = newOwners[side];
          if (
            toucher < 0 or sourceRank < 0 or toucher == unionOwners[0] or toucher == unionOwners[1]
            or (side == 1 and toucher == newOwners[0])
          ) {
            continue;
          }
          if (sourceRank == _id and toucher != _id) {
            keyedSendFaceSlots[toucher].emplace_back(
              EntityOrderKey{cell.level, normal, latticeOrderKey(faceLattice)},
              unionTraversal._rankTopology.faceSlotOfTreeFace[faceIndex]
            );
          }
        }
      }
    }

    std::vector<char> newVertexVisited(newTree.getNumberOfVertices(), 0);
    std::vector<char> newFaceVisited(newTree.getNumberOfFaces(), 0);
    for (const auto cellIndex : newCandidateCells) {
      const auto& cell          = newTree.getCell(cellIndex);
      const auto  uniformOwners = uniformBlockOwnersOf(newTree, cell);
      if (uniformOwners.uniform) {
        const bool receives = uniformOwners.newOwner == _id and uniformOwners.unionOwner != _id;
        listEntitiesOfUniformCell(
          newTree,
          cell,
          newTraversal._rankTopology.vertexSlotOfTreeVertex,
          newTraversal._rankTopology.faceSlotOfTreeFace,
          newVertexVisited,
          newFaceVisited,
          receives ? &keyedReceiveVertexSlots[uniformOwners.unionOwner] : nullptr,
          receives ? &keyedReceiveFaceSlots[uniformOwners.unionOwner] : nullptr
        );
        continue;
      }
      for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
        const auto vertexIndex = cell.vertexIndices[vertexNumber];
        if (newVertexVisited[vertexIndex] != 0) {
          continue;
        }
        newVertexVisited[vertexIndex] = 1;
        const auto vertexLattice      = vertexLatticeOfCorner(newTree, cell, vertexNumber);
        const bool keepsStructure     = cellKeepsStructure(cell, newVertexNearChange);
        if (
          keepsStructure ? newTree.getVertex(vertexIndex).getState() == peano4::grid::GridVertex::State::HangingVertex
                         : not newTree.isVertexPersistentInPlan(cell.level, vertexLattice)
        ) {
          continue;
        }
        std::array<int, peano4::CellsPerVertex> newOwners{};
        std::array<int, peano4::CellsPerVertex> unionOwners{};
        if (keepsStructure) {
          adjacentOwnersOfExistingCells(
            false,
            cell.level,
            vertexLattice,
            newTree.getPeriodicBoundaries(),
            unionOwners,
            newOwners
          );
        } else {
          newOwners = adjacentCellOwners(ownerOfNewCellAt, cell.level, vertexLattice, newTree.getPeriodicBoundaries());
          unionOwners =
            adjacentCellOwners(ownerOfUnionCellAt, cell.level, vertexLattice, unionTree.getPeriodicBoundaries());
        }
        int sourceRank = -1;
        for (const auto owner : unionOwners) {
          if (owner >= 0) {
            sourceRank = owner;
            break;
          }
        }
        for (int adjacencyEntry = 0; adjacencyEntry < peano4::CellsPerVertex; adjacencyEntry++) {
          const auto toucher = newOwners[adjacencyEntry];
          if (
            toucher < 0 or sourceRank < 0
            or std::find(unionOwners.begin(), unionOwners.end(), toucher) != unionOwners.end()
            or std::find(newOwners.begin(), newOwners.begin() + adjacencyEntry, toucher)
              != newOwners.begin() + adjacencyEntry
          ) {
            continue;
          }
          if (toucher == _id and sourceRank != _id) {
            keyedReceiveVertexSlots[sourceRank].emplace_back(
              EntityOrderKey{cell.level, 0, latticeOrderKey(vertexLattice)},
              newTraversal._rankTopology.vertexSlotOfTreeVertex[vertexIndex]
            );
          }
        }
      }
      for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
        const auto faceIndex = cell.faceIndices[faceNumber];
        if (newFaceVisited[faceIndex] != 0) {
          continue;
        }
        newFaceVisited[faceIndex]    = 1;
        const int  normal            = faceNumber % DIMENSIONS;
        const int  faceSide          = faceNumber < DIMENSIONS ? 0 : 1;
        const bool keepsStructure    = cellKeepsStructure(cell, newVertexNearChange);
        bool       facePersistsInNew = keepsStructure and faceHasPersistentVertex(newTree, cell, faceNumber);
        for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell and not keepsStructure; vertexNumber++) {
          if (
            ((vertexNumber >> normal) & 1) == faceSide
            and newTree.isVertexPersistentInPlan(cell.level, vertexLatticeOfCorner(newTree, cell, vertexNumber))
          ) {
            facePersistsInNew = true;
          }
        }
        if (not facePersistsInNew) {
          continue;
        }
        const auto          faceLattice = faceLatticeOfSide(newTree, cell, faceNumber);
        std::array<int, 2>  newOwners{};
        std::array<int, 2>  unionOwners{};
        std::array<bool, 2> sideCoarsens{};
        for (int side = 0; side < 2; side++) {
          auto sideLattice = faceLattice;
          sideLattice(normal) += side - 1;
          const int cellsPerAxisOnLevel = peano4::topology::cellsPerAxis(cell.level);
          if (newTree.getPeriodicBoundaries()[normal]) {
            sideLattice(normal) = (sideLattice(normal) + cellsPerAxisOnLevel) % cellsPerAxisOnLevel;
          }
          const bool isInsideDomain = sideLattice(normal) >= 0 and sideLattice(normal) < cellsPerAxisOnLevel;
          sideCoarsens[side]        = isInsideDomain and unionTree.isCellRefinedInPlan(cell.level, sideLattice)
            and newTree.cellExistsInPlan(cell.level, sideLattice)
            and not newTree.isCellRefinedInPlan(cell.level, sideLattice);
          if (keepsStructure) {
            const bool sideExists = isInsideDomain and newTree.cellExistsInPlan(cell.level, sideLattice);
            const auto [unionOwner, newOwner] =
              sideExists ? ownersOfExistingCellAt(false, cell.level, sideLattice) : std::pair<int, int>{-1, -1};
            newOwners[side]   = newOwner;
            unionOwners[side] = unionOwner;
            continue;
          }
          newOwners[side]   = isInsideDomain ? ownerOfNewCellAt(cell.level, sideLattice) : -1;
          unionOwners[side] = isInsideDomain ? ownerOfUnionCellAt(cell.level, sideLattice) : -1;
        }
        if (sideCoarsens[0] != sideCoarsens[1] and unionOwners[sideCoarsens[0] ? 0 : 1] >= 0) {
          const int faceSourceRank = unionOwners[sideCoarsens[0] ? 0 : 1];
          for (int side = 0; side < 2; side++) {
            const auto toucher = newOwners[side];
            if (toucher < 0 or toucher == faceSourceRank or (side == 1 and toucher == newOwners[0])) {
              continue;
            }
            if (toucher == _id and faceSourceRank != _id) {
              keyedReceiveFaceSlots[faceSourceRank].emplace_back(
                EntityOrderKey{cell.level, normal, latticeOrderKey(faceLattice)},
                newTraversal._rankTopology.faceSlotOfTreeFace[faceIndex]
              );
            }
          }
          continue;
        }
        const int sourceRank = unionOwners[0] >= 0 ? unionOwners[0] : unionOwners[1];
        for (int side = 0; side < 2; side++) {
          const auto toucher = newOwners[side];
          if (
            toucher < 0 or sourceRank < 0 or toucher == unionOwners[0] or toucher == unionOwners[1]
            or (side == 1 and toucher == newOwners[0])
          ) {
            continue;
          }
          if (toucher == _id and sourceRank != _id) {
            keyedReceiveFaceSlots[sourceRank].emplace_back(
              EntityOrderKey{cell.level, normal, latticeOrderKey(faceLattice)},
              newTraversal._rankTopology.faceSlotOfTreeFace[faceIndex]
            );
          }
        }
      }
    }
    emitOrdered(keyedSendCellSlots, transition.sendCellSlots);
    emitOrdered(keyedReceiveCellSlots, transition.receiveCellSlots);
    emitOrdered(keyedSendFaceSlots, transition.sendFaceSlots);
    emitOrdered(keyedReceiveFaceSlots, transition.receiveFaceSlots);
    emitOrdered(keyedSendVertexSlots, transition.sendVertexSlots);
    emitOrdered(keyedReceiveVertexSlots, transition.receiveVertexSlots);
  }

  // An entity the union exchanges but the old structure did not has no migrated
  // remote copy yet, so the transition sweep merges only frozen slots the first hop
  // fills; the union exchange refreshes every slot for the sweep after.
  // On the standing structure every frozen slot still holds its remote
  // partner's data, so the transition may merge them all.
  transition.overrides.vertexFrozenSlotValid.assign(
    unionTraversal._rankTopology.vertexSlotSpaceSize + unionTraversal._rankTopology.numberOfVertexFrozenSlots,
    unionIsStandingStructure ? 1 : 0
  );
  for (const auto& [sourceSlot, targetSlot] : transition.oldToUnionVertexPairs) {
    assertion(
      sourceSlot >= 0
        and static_cast<std::size_t>(sourceSlot) < static_cast<std::size_t>(_rankTopology.vertexSlotSpaceSize)
            + static_cast<std::size_t>(_rankTopology.numberOfVertexFrozenSlots),
      sourceSlot
    );
    assertion(
      targetSlot >= 0 and static_cast<std::size_t>(targetSlot) < transition.overrides.vertexFrozenSlotValid.size(),
      targetSlot,
      transition.overrides.vertexFrozenSlotValid.size()
    );
    if (targetSlot >= unionTraversal._rankTopology.vertexSlotSpaceSize) {
      transition.overrides.vertexFrozenSlotValid[targetSlot] = 1;
    }
  }
  transition.overrides.faceFrozenSlotValid.assign(
    unionTraversal._rankTopology.faceSlotSpaceSize + unionTraversal._rankTopology.numberOfFaceFrozenSlots,
    unionIsStandingStructure ? 1 : 0
  );
  for (const auto& [sourceSlot, targetSlot] : transition.oldToUnionFacePairs) {
    assertion(
      sourceSlot >= 0
        and static_cast<std::size_t>(sourceSlot) < static_cast<std::size_t>(_rankTopology.faceSlotSpaceSize)
            + static_cast<std::size_t>(_rankTopology.numberOfFaceFrozenSlots),
      sourceSlot
    );
    assertion(
      targetSlot >= 0 and static_cast<std::size_t>(targetSlot) < transition.overrides.faceFrozenSlotValid.size(),
      targetSlot,
      transition.overrides.faceFrozenSlotValid.size()
    );
    if (targetSlot >= unionTraversal._rankTopology.faceSlotSpaceSize) {
      transition.overrides.faceFrozenSlotValid[targetSlot] = 1;
    }
  }

  unionTraversal.assertRegridOverridesMatchTree(unionTree, transition.overrides);

  return transition;
}

/**
 * Selects task roots whose own entity state is unchanged by the transition.
 * A deeper root becomes the first cell of its task, so its data is present as
 * the coarse-grid parent while the task creates or destroys descendants.
 */
void peano4::topology::TreeTraversal::prepareTransitionTopology(
  const peano4::topology::Tree& tree,
  const int                     taskLevel,
  const RegridOverrides&        regridOverrides
) {
  assertRegridOverridesMatchTree(tree, regridOverrides);
  assertion(taskLevel >= 0 and taskLevel <= tree.getDepth(), taskLevel, tree.getDepth());
  _cellIsBlockRoot.assign(tree.getNumberOfCells(), 0);
  _cellIsDirectTask.assign(tree.getNumberOfCells(), 0);
  _subtreeDecomposition = SubtreeDecomposition{};
  deriveBlockRoots(tree, taskLevel, true, &regridOverrides);
}

/** Restores the task roots described by the persistent interface slots. */
void peano4::topology::TreeTraversal::restoreStandingTaskDecomposition(
  const peano4::topology::Tree& tree,
  const int                     taskLevel
) {
  assertion(taskLevel >= 0 and taskLevel <= tree.getDepth(), taskLevel, tree.getDepth());
  _cellIsBlockRoot.assign(tree.getNumberOfCells(), 0);
  _cellIsDirectTask.assign(tree.getNumberOfCells(), 0);
  _subtreeDecomposition = SubtreeDecomposition{};
  deriveBlockRoots(tree, taskLevel, _interfaceTopology.splitHeavyBlocks, nullptr);
}

/**
 * Derives the subtree decomposition before a parallel sweep,
 * so the caller can size the per-task observer slots. The interface
 * topology has to exist when task-local entity slots are used. The task level
 * must equal the tree's spawn level, so the subtree tasks of a rank are
 * exactly its blocks.
 */
void peano4::topology::TreeTraversal::prepareParallelSweep(
  const peano4::topology::Tree& tree,
  const int                     taskLevel,
  const bool                    requiresInterfaceTopology
) {
  assertRankTopologyMatchesTree(tree);
  assertion(taskLevel >= 0 and taskLevel <= tree.getDepth(), taskLevel, tree.getDepth());
  assertion(not requiresInterfaceTopology or _interfaceTopology.derived);
  assertion(taskLevel == tree.getSpawnLevel(), taskLevel, tree.getSpawnLevel());
  deriveSubtreeDecomposition(tree);
}

/**
 * Derives the subtree decomposition: One entry per local spawn-level block
 * with the automaton state of its root, in curve order. Every parallel sweep
 * reuses the result.
 */
void peano4::topology::TreeTraversal::deriveSubtreeDecomposition(const peano4::topology::Tree& tree) {
  auto& decomposition = _subtreeDecomposition;
  if (decomposition.derived) {
    return;
  }
  // The descent visits the levels down to the deepest task root and skips
  // the cells below, which hold no root.
  const auto numberOfCells        = static_cast<int>(tree.getNumberOfCells());
  int        deepestTaskRootLevel = 0;
  for (int cellIndex = 0; cellIndex < numberOfCells; cellIndex++) {
    if (_cellIsBlockRoot[cellIndex] != 0 or _cellIsDirectTask[cellIndex] != 0) {
      deepestTaskRootLevel = std::max(deepestTaskRootLevel, tree.getCell(cellIndex).level);
    }
  }
  decomposition.tasks.clear();
  collectSubtreeTasks(tree, _root, peano4::topology::Tree::RootCell, deepestTaskRootLevel, decomposition);
#if PEANO_ASSERTIONS
  SubtreeDecomposition unboundedDecomposition;
  collectSubtreeTasks(
    tree,
    _root,
    peano4::topology::Tree::RootCell,
    std::numeric_limits<int>::max(),
    unboundedDecomposition
  );
  assertion(
    decomposition.tasks.size() == unboundedDecomposition.tasks.size(),
    decomposition.tasks.size(),
    unboundedDecomposition.tasks.size(),
    deepestTaskRootLevel
  );
  for (std::size_t taskNumber = 0; taskNumber < decomposition.tasks.size(); taskNumber++) {
    assertion(
      decomposition.tasks[taskNumber].unit == unboundedDecomposition.tasks[taskNumber].unit
        and decomposition.tasks[taskNumber].rootCell == unboundedDecomposition.tasks[taskNumber].rootCell,
      taskNumber,
      decomposition.tasks[taskNumber].unit,
      unboundedDecomposition.tasks[taskNumber].unit,
      deepestTaskRootLevel
    );
  }
#endif
  decomposition.derived = true;
}

/**
 * Collects the local block roots below one refined cell in curve order
 * through a structure-only descent, refining the automaton state without
 * touching any observer. The descent stops at the deepest task-root level,
 * below which no root lies. A block whose wrapped neighbourhood contains a
 * remote cell position, or whose own children span several ranks, touches
 * the rank boundary, so its task follows the wait for the neighbour ranks'
 * data.
 */
void peano4::topology::TreeTraversal::collectSubtreeTasks(
  const peano4::topology::Tree&       tree,
  const peano4::grid::AutomatonState& state,
  const int                           cellIndex,
  const int                           deepestTaskRootLevel,
  SubtreeDecomposition&               decomposition
) const {
  peano4::grid::AutomatonState fineGridStates[ChildrenPerCell];
  peano4::grid::AutomatonState::refineState(state, fineGridStates);

  for (const auto& child : peano4::sfc::ActiveCurve::children(state.curveState)) {
    const auto  cellPositionLinearised = child.linearisedPosition;
    const auto  childCellIndex         = tree.getChild(cellIndex, cellPositionLinearised);
    const auto& childCell              = tree.getCell(childCellIndex);
    if (childCell.level >= tree.getSpawnLevel() and not tree.subtreeTouchesRank(childCellIndex, _id)) {
      continue;
    }
    const bool isDirectTask = _cellIsDirectTask[childCellIndex] != 0;
    if (isDirectTask or _cellIsBlockRoot[childCellIndex] != 0) {
      const int   cellsPerAxisOnLevel = peano4::topology::cellsPerAxis(childCell.level);
      SubtreeTask task;
      task.rootState    = fineGridStates[cellPositionLinearised];
      task.descentState = isDirectTask ? state : task.rootState;
      task.rootCell     = isDirectTask ? cellIndex : childCellIndex;
      task.unit         = childCellIndex;
      for (int neighbourNumber = 0; neighbourNumber < peano4::CellsPerNeighbourhood; neighbourNumber++) {
        auto neighbourLattice = childCell.latticeIndex;
        int  remainingDigits  = neighbourNumber;
        bool neighbourExists  = neighbourNumber != (peano4::CellsPerNeighbourhood - 1) / 2;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          neighbourLattice(axis) += remainingDigits % peano4::NeighbourhoodWidth - 1;
          remainingDigits /= peano4::NeighbourhoodWidth;
          if (tree.getPeriodicBoundaries()[axis]) {
            neighbourLattice(axis) = (neighbourLattice(axis) + cellsPerAxisOnLevel) % cellsPerAxisOnLevel;
          }
          neighbourExists =
            neighbourExists and neighbourLattice(axis) >= 0 and neighbourLattice(axis) < cellsPerAxisOnLevel;
        }
        // A neighbouring block that several ranks own can carry another rank's
        // cells right next to this root even when its first owner is this rank.
        if (neighbourExists
            and (tree.getOwnerOfCellAt(childCell.level, neighbourLattice) != _id
                 or (childCell.level == tree.getSpawnLevel() and tree.isSplitBlockAt(neighbourLattice)))) {
          task.touchesRankBoundary = true;
        }
      }
      // A split block's rank boundary runs between its own children, so its
      // root touches the boundary whatever the neighbouring blocks are.
      if (childCell.level == tree.getSpawnLevel() and tree.isSplitBlockAt(childCell.latticeIndex)) {
        task.touchesRankBoundary = true;
      }
      decomposition.tasks.push_back(task);
    }
    if (childCell.level < deepestTaskRootLevel and tree.isRefined(childCellIndex)) {
      collectSubtreeTasks(
        tree,
        fineGridStates[cellPositionLinearised],
        childCellIndex,
        deepestTaskRootLevel,
        decomposition
      );
    }
  }
}

/**
 * Derives the ordering dependencies of a transition sweep from the task roots.
 *
 * Closed root boxes that touch, overlap, or meet through a periodic boundary
 * run in colour order. Roots on one level take one of two parity colours per
 * axis, and the level separates roots of different sizes. A periodic axis
 * with an odd subdivision factor holds an odd cell count, whose wrap joins
 * the even indices 0 and the last one, so such an axis takes three colours.
 */
void peano4::topology::TreeTraversal::deriveTransitionDependencies(
  const peano4::topology::Tree&    tree,
  const SubtreeDecomposition&      decomposition,
  std::vector<char>&               completionTokens,
  std::vector<std::vector<void*>>& dependencies,
  std::vector<int>&                spawnOrder
) const {
  const auto numberOfTasks   = decomposition.tasks.size();
  int        finestTaskLevel = 0;
  for (const auto& task : decomposition.tasks) {
    finestTaskLevel = std::max(finestTaskLevel, tree.getCell(task.unit).level);
  }

  struct TaskBox {
    std::array<std::int64_t, DIMENSIONS> lower{};
    std::array<std::int64_t, DIMENSIONS> upper{};
    int                                  colour{0};
  };
  std::vector<TaskBox> taskBoxes(numberOfTasks);
  const auto           taskGridWidth = static_cast<std::int64_t>(peano4::topology::cellsPerAxis(finestTaskLevel));
  for (std::size_t taskNumber = 0; taskNumber < numberOfTasks; taskNumber++) {
    const auto& rootCell = tree.getCell(decomposition.tasks[taskNumber].unit);
    const auto  scale    = static_cast<std::int64_t>(peano4::topology::cellsPerAxis(finestTaskLevel - rootCell.level));
    int         latticeColour = 0;
    for (int axis = DIMENSIONS - 1; axis >= 0; axis--) {
      const int coloursOnAxis = tree.getPeriodicBoundaries()[axis] and SubdivisionFactor % 2 == 1 ? 3 : 2;
      latticeColour           = latticeColour * coloursOnAxis + rootCell.latticeIndex(axis) % coloursOnAxis;
    }
    assertion(latticeColour >= 0 and latticeColour < peano4::CellsPerNeighbourhood, latticeColour);
    taskBoxes[taskNumber].colour = rootCell.level * peano4::CellsPerNeighbourhood + latticeColour;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      taskBoxes[taskNumber].lower[axis] = static_cast<std::int64_t>(rootCell.latticeIndex(axis)) * scale;
      taskBoxes[taskNumber].upper[axis] = static_cast<std::int64_t>(rootCell.latticeIndex(axis) + 1) * scale;
    }
  }

  const auto boxesTouch = [&tree, taskGridWidth](const TaskBox& left, const TaskBox& right) {
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      bool intervalsTouch = left.lower[axis] <= right.upper[axis] and right.lower[axis] <= left.upper[axis];
      if (tree.getPeriodicBoundaries()[axis] and not intervalsTouch) {
        for (const auto shift : {-taskGridWidth, taskGridWidth}) {
          intervalsTouch = intervalsTouch
            or (left.lower[axis] <= right.upper[axis] + shift and right.lower[axis] + shift <= left.upper[axis]);
        }
      }
      if (not intervalsTouch) {
        return false;
      }
    }
    return true;
  };
  // A bucket as wide as the largest box means a box spans at most two buckets
  // per axis, so everything it can touch sits within one bucket of it.
  std::int64_t bucketWidth = 1;
  for (const auto& taskBox : taskBoxes) {
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      bucketWidth = std::max(bucketWidth, taskBox.upper[axis] - taskBox.lower[axis]);
    }
  }
  const auto bucketsPerAxis = std::max<std::int64_t>(1, (taskGridWidth + bucketWidth - 1) / bucketWidth);
  const auto bucketKeyOf    = [bucketsPerAxis](const std::array<std::int64_t, DIMENSIONS>& bucketLattice) {
    std::int64_t key = 0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      key = key * bucketsPerAxis + bucketLattice[axis];
    }
    return key;
  };

  std::unordered_map<std::int64_t, std::vector<std::size_t>> tasksOfBucket;
  tasksOfBucket.reserve(numberOfTasks);
  for (std::size_t taskNumber = 0; taskNumber < numberOfTasks; taskNumber++) {
    std::array<std::int64_t, DIMENSIONS> bucketLattice{};
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      bucketLattice[axis] = (taskBoxes[taskNumber].lower[axis] / bucketWidth) % bucketsPerAxis;
    }
    tasksOfBucket[bucketKeyOf(bucketLattice)].push_back(taskNumber);
  }

  std::vector<std::size_t> touchingTasks;
  for (std::size_t leftTask = 0; leftTask < numberOfTasks; leftTask++) {
    touchingTasks.clear();
    for (int neighbourNumber = 0; neighbourNumber < peano4::CellsPerNeighbourhood; neighbourNumber++) {
      std::array<std::int64_t, DIMENSIONS> bucketLattice{};
      int                                  remainingNeighbour = neighbourNumber;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        const auto ownBucket = taskBoxes[leftTask].lower[axis] / bucketWidth;
        bucketLattice[axis] =
          (ownBucket + remainingNeighbour % peano4::NeighbourhoodWidth - 1 + bucketsPerAxis) % bucketsPerAxis;
        remainingNeighbour /= peano4::NeighbourhoodWidth;
      }
      const auto bucket = tasksOfBucket.find(bucketKeyOf(bucketLattice));
      if (bucket == tasksOfBucket.end()) {
        continue;
      }
      for (const auto rightTask : bucket->second) {
        if (rightTask > leftTask and boxesTouch(taskBoxes[leftTask], taskBoxes[rightTask])) {
          touchingTasks.push_back(rightTask);
        }
      }
    }
    // One box can reach the same bucket from two sides of a short periodic
    // axis, and the pairs have to enter in task order so the dependency lists
    // do not depend on the bucket walk.
    std::sort(touchingTasks.begin(), touchingTasks.end());
    touchingTasks.erase(std::unique(touchingTasks.begin(), touchingTasks.end()), touchingTasks.end());

    for (const auto rightTask : touchingTasks) {
      const bool leftRunsFirst = taskBoxes[leftTask].colour < taskBoxes[rightTask].colour
        or (taskBoxes[leftTask].colour == taskBoxes[rightTask].colour and leftTask < rightTask);
      const auto prerequisiteTask = leftRunsFirst ? leftTask : rightTask;
      const auto dependentTask    = leftRunsFirst ? rightTask : leftTask;
      dependencies[dependentTask].push_back(&completionTokens[prerequisiteTask]);
    }
  }

  spawnOrder.resize(numberOfTasks);
  std::iota(spawnOrder.begin(), spawnOrder.end(), 0);
  std::stable_sort(spawnOrder.begin(), spawnOrder.end(), [&taskBoxes](const int left, const int right) {
    return taskBoxes[left].colour < taskBoxes[right].colour;
  });
}

/**
 * Runs one parallel pool sweep over the spawn-level decomposition. A standing
 * sweep with a recording replays it. Every other sweep descends in parallel,
 * one task per local block and the master on the spine, and a standing sweep
 * records itself, so no serial sweep precedes the parallel ones.
 */
void peano4::topology::TreeTraversal::traverseWithPoolTasks(
  const peano4::topology::Tree&        tree,
  peano4::grid::GridTraversalObserver& observer,
  const PoolSweepKind                  sweepKind,
  const int                            taskLevel,
  const std::function<void()>&         waitForRemoteData,
  const RegridOverrides* const         regridOverrides
) {
  assertRankTopologyMatchesTree(tree);
  assertion(regridOverrides != nullptr or _interfaceTopology.derived);
  assertion(taskLevel == tree.getSpawnLevel(), taskLevel, tree.getSpawnLevel());
  if (regridOverrides != nullptr) {
    assertRegridOverridesMatchTree(tree, *regridOverrides);
  }

  if (sweepKind == PoolSweepKind::Standing and _parallelRecording.replaysEntries and regridOverrides == nullptr) {
    if (_partialRerecordPending) {
      runPartialRerecordSweep(tree, observer, waitForRemoteData);
    } else {
      runParallelReplay(observer, waitForRemoteData);
    }
  } else {
    runParallelSweep(tree, observer, sweepKind, taskLevel, waitForRemoteData, regridOverrides);
  }
}

/**
 * Runs one pool sweep as a parallel descent of the spawn-level decomposition:
 * One task per local block, the master on the spine, stopping at the block
 * roots. Every task owns its scope, an entity several blocks touch sits
 * on each toucher's own slot, and the merges of the partners' frozen copies
 * run at each unit's first touch, so no two tasks write the same data. A
 * standing sweep records itself per task and assembles the consolidations
 * from the tasks' first-touch seeds in task order, which is the curve order
 * of the blocks.
 */
void peano4::topology::TreeTraversal::runParallelSweep(
  const peano4::topology::Tree&        tree,
  peano4::grid::GridTraversalObserver& observer,
  const PoolSweepKind                  sweepKind,
  const int                            taskLevel,
  const std::function<void()>&         waitForRemoteData,
  const RegridOverrides* const         regridOverrides
) {
  const bool isRecording  = sweepKind == PoolSweepKind::Standing and regridOverrides == nullptr;
  const bool isTransition = regridOverrides != nullptr;

  if (sweepKind == PoolSweepKind::Creational or regridOverrides != nullptr) {
    _numberOfStationarySweeps = 0;
    _eventRecording           = EventRecording{};
    // An in-place regrid keeps the recordings for its untouched tasks, so
    // only its stale tasks re-record after the transition.
    if (not _partialRerecordPending) {
      _parallelRecording = ParallelRecording{};
    }
  } else {
    _numberOfStationarySweeps++;
  }

  deriveSubtreeDecomposition(tree);
  const auto& decomposition = _subtreeDecomposition;
  const auto  numberOfTasks = decomposition.tasks.size();

  auto&            parallel = _parallelRecording;
  std::vector<int> taskCells(numberOfTasks, 0);
  if (isRecording) {
    parallel = ParallelRecording{};
    parallel.subtreeTasks.resize(numberOfTasks);
    const auto countTaskCells = [this, &tree](const auto& recurse, const int cellIndex, const int unit) -> int {
      if (not tree.isRefined(cellIndex)) {
        return 0;
      }
      int numberOfCells = 0;
      for (int childPosition = 0; childPosition < ChildrenPerCell; childPosition++) {
        const auto childCellIndex = tree.getChild(cellIndex, childPosition);
        const auto childLevel     = tree.getCell(childCellIndex).level;
        if (childLevel >= tree.getSpawnLevel() and not tree.subtreeTouchesRank(childCellIndex, _id)) {
          continue;
        }
        if (_interfaceTopology.unitOfCell[childCellIndex] != unit) {
          continue;
        }
        numberOfCells++;
        if (_cellIsBlockRoot[childCellIndex] == 0) {
          numberOfCells += recurse(recurse, childCellIndex, unit);
        }
      }
      return numberOfCells;
    };
    int spineCells   = 0;
    int cellsOnLevel = 1;
    for (int level = 1; level <= taskLevel; level++) {
      cellsOnLevel *= ChildrenPerCell;
      spineCells += cellsOnLevel;
    }
    parallel.masterEntries.reserve(2 * static_cast<std::size_t>(spineCells));
    // The counts walk disjoint subtrees and write disjoint entries, so they
    // run as chunked tasks.
    peano4::topology::runChunkedTasks(
      static_cast<int>(numberOfTasks),
      [&](const int chunkNumber, const int begin, const int end) {
        static_cast<void>(chunkNumber);
        for (int taskNumber = begin; taskNumber < end; taskNumber++) {
          auto& subtreeTask               = parallel.subtreeTasks[taskNumber];
          subtreeTask.x                   = decomposition.tasks[taskNumber].rootState.x;
          subtreeTask.h                   = decomposition.tasks[taskNumber].rootState.h;
          subtreeTask.touchesRankBoundary = decomposition.tasks[taskNumber].touchesRankBoundary;
          taskCells[taskNumber]           = countTaskCells(
            countTaskCells,
            decomposition.tasks[taskNumber].rootCell,
            decomposition.tasks[taskNumber].unit
          );
          subtreeTask.entries.reserve(2 * static_cast<std::size_t>(taskCells[taskNumber]));
        }
      },
      TasksPerCountChunk
    );
  }

  std::vector<TraversalScope>                        taskScopes(numberOfTasks);
  std::vector<std::vector<ConsolidationSeed>>        taskSeeds(numberOfTasks);
  std::vector<std::vector<InterfaceMergeDescriptor>> taskRankBoundaryMerges(isTransition ? numberOfTasks : 0);
  std::vector<char>                                  completionTokens(numberOfTasks, 0);
  std::atomic<double>                                taskSeconds{0.0};
  const auto                                         windowStart = now();

  std::vector<std::uint8_t>       vertexTouchesOfAllUnits;
  std::vector<std::uint8_t>       faceTouchesOfAllUnits;
  std::vector<std::vector<void*>> transitionDependencies(numberOfTasks);
  std::vector<int>                spawnOrder(numberOfTasks);
  std::iota(spawnOrder.begin(), spawnOrder.end(), 0);
  if (isRecording) {
    // The largest tasks spawn first, so the small ones fill the end of the sweep.
    std::stable_sort(spawnOrder.begin(), spawnOrder.end(), [&taskCells](const int left, const int right) {
      return taskCells[left] > taskCells[right];
    });
  }
  vertexTouchesOfAllUnits.assign(
    isTransition ? _rankTopology.vertexSlotSpaceSize : _interfaceTopology.numberOfVertexTouchCounters,
    0
  );
  faceTouchesOfAllUnits.assign(
    isTransition ? _rankTopology.faceSlotSpaceSize : _interfaceTopology.numberOfFaceTouchCounters,
    0
  );
  if (isTransition) {
    deriveTransitionDependencies(tree, decomposition, completionTokens, transitionDependencies, spawnOrder);
  }

  observer.beginTraversal(_root.x, _root.h);
  const auto spawnTasks = [this,
                           &tree,
                           &decomposition,
                           &parallel,
                           &taskScopes,
                           &taskSeeds,
                           &taskRankBoundaryMerges,
                           &completionTokens,
                           &observer,
                           &transitionDependencies,
                           &spawnOrder,
                           &vertexTouchesOfAllUnits,
                           &faceTouchesOfAllUnits,
                           &taskSeconds,
                           isRecording,
                           isTransition,
                           sweepKind,
                           regridOverrides](const bool spawnRankBoundaryTasks) {
    for (const auto taskNumber : spawnOrder) {
      const auto& task = decomposition.tasks[taskNumber];
      if (not isTransition and task.touchesRankBoundary != spawnRankBoundaryTasks) {
        continue;
      }
      auto* const taskObserver       = &observerOfTask(observer, static_cast<int>(taskNumber));
      auto* const scope              = &taskScopes[taskNumber];
      auto* const seeds              = isRecording ? &taskSeeds[taskNumber] : nullptr;
      auto* const rankBoundaryMerges = isTransition ? &taskRankBoundaryMerges[taskNumber] : nullptr;
      auto* const subtreeTask        = isRecording ? &parallel.subtreeTasks[taskNumber] : nullptr;
      auto* const vertexTouches      = &vertexTouchesOfAllUnits;
      auto* const faceTouches        = &faceTouchesOfAllUnits;

      auto runSubtreeTask = [this,
                             &tree,
                             &taskSeconds,
                             sweepKind,
                             regridOverrides,
                             task,
                             taskObserver,
                             scope,
                             seeds,
                             rankBoundaryMerges,
                             subtreeTask,
                             isTransition,
                             vertexTouches,
                             faceTouches]() {
        const auto taskStart             = now();
        scope->sweepKind                 = sweepKind;
        scope->regridOverrides           = regridOverrides;
        scope->unit                      = task.unit;
        scope->directTaskRoot            = isTransition and _cellIsDirectTask[task.unit] != 0 ? task.unit : -1;
        scope->skipsBlockRoots           = true;
        scope->writesEntitySlotsDirectly = isTransition;
        scope->vertexTouchesOfAllUnits   = vertexTouches;
        scope->faceTouchesOfAllUnits     = faceTouches;
        if (subtreeTask != nullptr) {
          scope->entries      = &subtreeTask->entries;
          scope->inlineMerges = &subtreeTask->interfaceMerges;
        }
        scope->transitionRankBoundaryMerges = rankBoundaryMerges;
        scope->consolidationSeeds           = seeds;

        peano4::grid::GridVertex rootVertices[peano4::VerticesPerCell];
        tree.gatherCellVertices(task.rootCell, rootVertices);
        taskObserver->beginTraversal(task.rootState.x, task.rootState.h);
        descendWithPools(tree, task.descentState, task.rootCell, rootVertices, *taskObserver, *scope);
        taskObserver->endTraversal(task.rootState.x, task.rootState.h);
        accumulateSeconds(taskSeconds, now() - taskStart);
      };
      const auto&          inDependencies = transitionDependencies[taskNumber];
      std::array<void*, 1> outDependencies{&completionTokens[taskNumber]};
      tarch::tasking::spawnTask(
        new tarch::tasking::FunctorTask(runSubtreeTask),
        std::span<void* const>(inDependencies.data(), inDependencies.size()),
        outDependencies
      );
    }
  };
  // The remote frozen copies must have landed before any task merges them.
  // A transition's tasks and a recording's tasks all spawn after the wait,
  // because a task fills touchesRankBoundary only while it records.
  if (waitForRemoteData) {
    waitForRemoteData();
  }
  if (isTransition) {
    spawnTasks(true);
  } else {
    spawnTasks(false);
    spawnTasks(true);
  }

  std::vector<InterfaceMergeDescriptor> masterRankBoundaryMerges;
  TraversalScope                        masterScope;
  masterScope.sweepKind                 = sweepKind;
  masterScope.regridOverrides           = regridOverrides;
  masterScope.skipsBlockRoots           = true;
  masterScope.writesEntitySlotsDirectly = isTransition;
  masterScope.vertexTouchesOfAllUnits   = &vertexTouchesOfAllUnits;
  masterScope.faceTouchesOfAllUnits     = &faceTouchesOfAllUnits;
  if (isRecording) {
    masterScope.entries      = &parallel.masterEntries;
    masterScope.inlineMerges = &parallel.masterMerges;
  }
  if (isTransition) {
    masterScope.transitionRankBoundaryMerges = &masterRankBoundaryMerges;
  }
  peano4::grid::GridVertex rootVertices[peano4::VerticesPerCell];
  tree.gatherCellVertices(peano4::topology::Tree::RootCell, rootVertices);
  descendWithPools(tree, _root, peano4::topology::Tree::RootCell, rootVertices, observer, masterScope);

  std::vector<void*> completionDependencies;
  completionDependencies.reserve(numberOfTasks);
  for (std::size_t taskNumber = 0; taskNumber < numberOfTasks; taskNumber++) {
    completionDependencies.push_back(&completionTokens[taskNumber]);
  }
  tarch::tasking::waitForDependencies(
    std::span<void* const>(completionDependencies.data(), completionDependencies.size())
  );
  if (isTransition) {
    _transitionRankBoundaryMerges = std::move(masterRankBoundaryMerges);
    for (auto& boundaryMerges : taskRankBoundaryMerges) {
      _transitionRankBoundaryMerges.insert(
        _transitionRankBoundaryMerges.end(),
        std::make_move_iterator(boundaryMerges.begin()),
        std::make_move_iterator(boundaryMerges.end())
      );
    }
  }
  _statistics.clear(_id == 0);
  _statistics += masterScope.statistics;
  for (const auto& scope : taskScopes) {
    _statistics += scope.statistics;
  }

  _lastSweepTaskSeconds = taskSeconds.load();

  // A transition uses a decomposition of its own, so it must not overwrite
  // what the standing sweeps are balanced against.
  if (not isTransition) {
    _subtreeTaskLoad               = SubtreeTaskLoad{};
    _subtreeTaskLoad.taskSeconds   = _lastSweepTaskSeconds;
    _subtreeTaskLoad.windowSeconds = now() - windowStart;
    for (const auto& scope : taskScopes) {
      const auto taskCells = scope.statistics.numberOfLocalUnrefinedCells + scope.statistics.numberOfLocalRefinedCells;
      _subtreeTaskLoad.minimumCells =
        _subtreeTaskLoad.numberOfTasks == 0 ? taskCells : std::min(_subtreeTaskLoad.minimumCells, taskCells);
      _subtreeTaskLoad.maximumCells = std::max(_subtreeTaskLoad.maximumCells, taskCells);
      _subtreeTaskLoad.totalCells += taskCells;
      _subtreeTaskLoad.numberOfTasks++;
    }
  }
  _statistics.numberOfStationarySweeps = _numberOfStationarySweeps;

  if (isRecording) {
    // The first toucher's copy rebuilds the entity slot and the other
    // touchers' copies merge into it, so the consolidated slot never keeps
    // state that no task's copy holds any more.
    std::map<std::pair<bool, int>, std::size_t> consolidationOfEntity;
    for (std::size_t taskNumber = 0; taskNumber < numberOfTasks; taskNumber++) {
      for (const auto& seed : taskSeeds[taskNumber]) {
        const auto [entry, isNew] =
          consolidationOfEntity.emplace(std::make_pair(seed.isVertex, seed.entitySlot), parallel.consolidations.size());
        if (isNew) {
          parallel.consolidations.push_back({seed.event, seed.position, seed.isVertex, {seed.liveSlot}, 1, true});
        } else {
          parallel.consolidations[entry->second].sourceSlots.push_back(seed.liveSlot);
          parallel.consolidations[entry->second].numberOfPartnerSlots++;
        }
      }
    }
    parallel.statistics       = _statistics;
    parallel.masterStatistics = masterScope.statistics;
    for (std::size_t taskNumber = 0; taskNumber < numberOfTasks; taskNumber++) {
      auto& subtreeTask      = parallel.subtreeTasks[taskNumber];
      subtreeTask.statistics = taskScopes[taskNumber].statistics;
      // A task with a neighbour rank's copy among its recorded merges follows
      // the wait for that copy in every replay, whatever its root's neighbours
      // said.
      for (const auto& merge : subtreeTask.interfaceMerges) {
        if (merge.sourceSlots.size() > static_cast<std::size_t>(merge.numberOfPartnerSlots)) {
          subtreeTask.touchesRankBoundary = true;
        }
      }
    }
    deriveReplayOrder();
    parallel.derived        = true;
    parallel.replaysEntries = true;
  }
  observer.endTraversal(_root.x, _root.h);
}

/**
 * Sorts the replay tasks by falling entry count. A replay spawns the tasks
 * in this order, so the longest replays start first and the short ones fill
 * the end of the sweep.
 */
void peano4::topology::TreeTraversal::deriveReplayOrder() {
  auto& parallel = _parallelRecording;
  parallel.replayOrder.resize(parallel.subtreeTasks.size());
  std::iota(parallel.replayOrder.begin(), parallel.replayOrder.end(), 0);
  std::stable_sort(
    parallel.replayOrder.begin(),
    parallel.replayOrder.end(),
    [&parallel](const int left, const int right) {
      return parallel.subtreeTasks[left].entries.size() > parallel.subtreeTasks[right].entries.size();
    }
  );
}

/**
 * Runs one standing pool sweep as a parallel replay of the recording: Every
 * subtree task replays its entry buffer on its own observer clone and the
 * master replays the spine meanwhile. A task first merges its interface
 * entities' partner copies into its own live copies through the observer's
 * generated merge routines; the source copies stay untouched all sweep and
 * every lifecycle runs on the toucher's own copy, so no two tasks write the
 * same data and the result is deterministic under any schedule. The caller
 * copies the live copies onto the frozen ones and runs the consolidations
 * after every task has finished.
 */
void peano4::topology::TreeTraversal::runParallelReplay(
  peano4::grid::GridTraversalObserver& observer,
  const std::function<void()>&         waitForRemoteData
) {
  const auto& parallel = _parallelRecording;
  assertion(parallel.derived);
  assertion(
    parallel.replayOrder.size() == parallel.subtreeTasks.size(),
    parallel.replayOrder.size(),
    parallel.subtreeTasks.size()
  );

  _numberOfStationarySweeps++;

  const auto          numberOfTasks = parallel.subtreeTasks.size();
  std::vector<char>   completionTokens(numberOfTasks, 0);
  std::atomic<double> taskSeconds{0.0};
  const auto          windowStart = now();

  observer.beginTraversal(_root.x, _root.h);
  const auto spawnTasks =
    [this, &parallel, &completionTokens, &observer, &taskSeconds](const bool spawnRankBoundaryTasks) {
      for (const auto taskNumber : parallel.replayOrder) {
        if (parallel.subtreeTasks[taskNumber].touchesRankBoundary != spawnRankBoundaryTasks) {
          continue;
        }
        auto* const       taskObserver = &observerOfTask(observer, taskNumber);
        const auto* const subtreeTask  = &parallel.subtreeTasks[taskNumber];

        auto replaySubtree = [taskObserver, subtreeTask, &taskSeconds]() {
          const auto taskStart = now();
          taskObserver->beginTraversal(subtreeTask->x, subtreeTask->h);
          replayEntries(subtreeTask->entries, subtreeTask->interfaceMerges, *taskObserver);
          taskObserver->endTraversal(subtreeTask->x, subtreeTask->h);
          accumulateSeconds(taskSeconds, now() - taskStart);
        };
        tarch::tasking::spawnTask(new tarch::tasking::FunctorTask(replaySubtree), {}, {&completionTokens[taskNumber]});
      }
    };
  spawnTasks(false);
  if (waitForRemoteData) {
    waitForRemoteData();
  }
  spawnTasks(true);
  replayEntries(parallel.masterEntries, parallel.masterMerges, observer);
  std::vector<void*> completionDependencies;
  completionDependencies.reserve(numberOfTasks);
  for (auto& token : completionTokens) {
    completionDependencies.push_back(&token);
  }
  tarch::tasking::waitForDependencies(
    std::span<void* const>(completionDependencies.data(), completionDependencies.size())
  );
  observer.endTraversal(_root.x, _root.h);

  _lastSweepTaskSeconds          = taskSeconds.load();
  _subtreeTaskLoad.taskSeconds   = _lastSweepTaskSeconds;
  _subtreeTaskLoad.windowSeconds = now() - windowStart;

  _statistics                          = parallel.statistics;
  _statistics.numberOfStationarySweeps = _numberOfStationarySweeps;
}

/**
 * Runs one standing sweep that replays the untouched tasks and re-records the
 * stale ones after an in-place regrid. A stale task descends its subtree like
 * a recording sweep, while every other task and the master replay their kept
 * buffers, whose slots the in-place regrid preserved. The totals recompose
 * from the per-task recordings, so they match a full re-recording.
 */
void peano4::topology::TreeTraversal::runPartialRerecordSweep(
  const peano4::topology::Tree&        tree,
  peano4::grid::GridTraversalObserver& observer,
  const std::function<void()>&         waitForRemoteData
) {
  auto& parallel = _parallelRecording;
  assertion(parallel.derived);
  assertion(_subtreeDecomposition.derived);
  assertion(
    parallel.subtreeTasks.size() == _subtreeDecomposition.tasks.size(),
    parallel.subtreeTasks.size(),
    _subtreeDecomposition.tasks.size()
  );

  assertion(
    parallel.replayOrder.size() == parallel.subtreeTasks.size(),
    parallel.replayOrder.size(),
    parallel.subtreeTasks.size()
  );

  _numberOfStationarySweeps++;

  const auto        numberOfTasks = parallel.subtreeTasks.size();
  std::vector<char> taskIsStale(numberOfTasks, 0);
  for (std::size_t taskNumber = 0; taskNumber < numberOfTasks; taskNumber++) {
    for (const auto staleUnit : _staleRerecordUnits) {
      if (staleUnit == _subtreeDecomposition.tasks[taskNumber].unit) {
        taskIsStale[taskNumber] = 1;
      }
    }
  }

  std::vector<TraversalScope> taskScopes(numberOfTasks);
  std::vector<char>           completionTokens(numberOfTasks, 0);
  std::atomic<double>         taskSeconds{0.0};
  const auto                  windowStart = now();

  std::vector<std::uint8_t> vertexTouchesOfAllUnits(_interfaceTopology.numberOfVertexTouchCounters, 0);
  std::vector<std::uint8_t> faceTouchesOfAllUnits(_interfaceTopology.numberOfFaceTouchCounters, 0);

  // The interior replays spawn before the wait for the neighbour ranks' data
  // and hide it; a task that re-records does not know its rank-boundary merges
  // yet, so it follows the wait like the boundary replays.
  observer.beginTraversal(_root.x, _root.h);
  const auto spawnTasks = [&](const bool spawnAfterWait) {
    for (const auto taskNumber : parallel.replayOrder) {
      auto* const taskObserver = &observerOfTask(observer, taskNumber);
      auto* const subtreeTask  = &parallel.subtreeTasks[taskNumber];
      const bool  followsWait  = taskIsStale[taskNumber] != 0 or subtreeTask->touchesRankBoundary;
      if (followsWait != spawnAfterWait) {
        continue;
      }
      if (taskIsStale[taskNumber] != 0) {
        const auto  task          = _subtreeDecomposition.tasks[taskNumber];
        auto* const scope         = &taskScopes[taskNumber];
        auto* const vertexTouches = &vertexTouchesOfAllUnits;
        auto* const faceTouches   = &faceTouchesOfAllUnits;
        auto        rerecordSubtree =
          [this, &tree, &taskSeconds, task, taskObserver, scope, subtreeTask, vertexTouches, faceTouches]() {
            const auto taskStart = now();
            subtreeTask->entries.clear();
            subtreeTask->interfaceMerges.clear();
            scope->sweepKind               = PoolSweepKind::Standing;
            scope->unit                    = task.unit;
            scope->skipsBlockRoots         = true;
            scope->vertexTouchesOfAllUnits = vertexTouches;
            scope->faceTouchesOfAllUnits   = faceTouches;
            scope->entries                 = &subtreeTask->entries;
            scope->inlineMerges            = &subtreeTask->interfaceMerges;
            peano4::grid::GridVertex rootVertices[peano4::VerticesPerCell];
            tree.gatherCellVertices(task.rootCell, rootVertices);
            taskObserver->beginTraversal(task.rootState.x, task.rootState.h);
            descendWithPools(tree, task.descentState, task.rootCell, rootVertices, *taskObserver, *scope);
            taskObserver->endTraversal(task.rootState.x, task.rootState.h);
            subtreeTask->statistics = scope->statistics;
            accumulateSeconds(taskSeconds, now() - taskStart);
          };
        tarch::tasking::spawnTask(
          new tarch::tasking::FunctorTask(rerecordSubtree),
          {},
          {&completionTokens[taskNumber]}
        );
      } else {
        auto replaySubtree = [taskObserver, subtreeTask, &taskSeconds]() {
          const auto taskStart = now();
          taskObserver->beginTraversal(subtreeTask->x, subtreeTask->h);
          replayEntries(subtreeTask->entries, subtreeTask->interfaceMerges, *taskObserver);
          taskObserver->endTraversal(subtreeTask->x, subtreeTask->h);
          accumulateSeconds(taskSeconds, now() - taskStart);
        };
        tarch::tasking::spawnTask(new tarch::tasking::FunctorTask(replaySubtree), {}, {&completionTokens[taskNumber]});
      }
    }
  };
  spawnTasks(false);
  if (waitForRemoteData) {
    waitForRemoteData();
  }
  spawnTasks(true);
  replayEntries(parallel.masterEntries, parallel.masterMerges, observer);
  std::vector<void*> completionDependencies;
  completionDependencies.reserve(numberOfTasks);
  for (auto& token : completionTokens) {
    completionDependencies.push_back(&token);
  }
  tarch::tasking::waitForDependencies(
    std::span<void* const>(completionDependencies.data(), completionDependencies.size())
  );
  observer.endTraversal(_root.x, _root.h);

  _lastSweepTaskSeconds = taskSeconds.load();
  _statistics.clear(_id == 0);
  _statistics += parallel.masterStatistics;
  _subtreeTaskLoad               = SubtreeTaskLoad{};
  _subtreeTaskLoad.taskSeconds   = _lastSweepTaskSeconds;
  _subtreeTaskLoad.windowSeconds = now() - windowStart;
  for (const auto& subtreeTask : parallel.subtreeTasks) {
    _statistics += subtreeTask.statistics;
    const auto taskCells =
      subtreeTask.statistics.numberOfLocalUnrefinedCells + subtreeTask.statistics.numberOfLocalRefinedCells;
    _subtreeTaskLoad.minimumCells =
      _subtreeTaskLoad.numberOfTasks == 0 ? taskCells : std::min(_subtreeTaskLoad.minimumCells, taskCells);
    _subtreeTaskLoad.maximumCells = std::max(_subtreeTaskLoad.maximumCells, taskCells);
    _subtreeTaskLoad.totalCells += taskCells;
    _subtreeTaskLoad.numberOfTasks++;
  }
  _statistics.numberOfStationarySweeps = _numberOfStationarySweeps;
  parallel.statistics                  = _statistics;

  deriveReplayOrder();
  _staleRerecordUnits.clear();
  _partialRerecordPending = false;
}

/**
 * Merges every task's frozen interface partners into its live copy before a
 * regrid flushes one complete live copy into the entity slot. With neighbour
 * ranks, local-only merges run while their receives complete.
 */
void peano4::topology::TreeTraversal::mergeTaskInterfaceData(
  peano4::grid::GridTraversalObserver& observer,
  const std::function<void()>&         waitForRemoteData
) const {
  const auto& parallel = _parallelRecording;
  assertion(parallel.derived);

  const auto        numberOfTasks = parallel.subtreeTasks.size();
  std::vector<char> completionTokens(numberOfTasks, 0);
  const auto        spawnTasks = [this, &parallel, &completionTokens, &observer](const bool spawnRankBoundaryTasks) {
    for (std::size_t taskNumber = 0; taskNumber < parallel.subtreeTasks.size(); taskNumber++) {
      const auto* const subtreeTask = &parallel.subtreeTasks[taskNumber];
      if (subtreeTask->touchesRankBoundary != spawnRankBoundaryTasks) {
        continue;
      }
      auto* const taskObserver = &observerOfTask(observer, static_cast<int>(taskNumber));
      tarch::tasking::spawnTask(
        new tarch::tasking::FunctorTask([subtreeTask, taskObserver]() {
          runMerges(subtreeTask->interfaceMerges, *taskObserver);
        }),
        {},
        {&completionTokens[taskNumber]}
      );
    }
  };
  spawnTasks(false);
  if (waitForRemoteData) {
    waitForRemoteData();
  }
  spawnTasks(true);
  runMerges(parallel.masterMerges, observer);

  std::vector<void*> completionDependencies;
  completionDependencies.reserve(numberOfTasks);
  for (auto& token : completionTokens) {
    completionDependencies.push_back(&token);
  }
  tarch::tasking::waitForDependencies(
    std::span<void* const>(completionDependencies.data(), completionDependencies.size())
  );
}

/**
 * Merges every neighbour rank's post-transition copy into the live
 * rank-boundary entities. A repartition hop can delete a boundary's exchange
 * channel before the following sweep merges the partner's copy, so a caller
 * replays the transition's recorded merges once the receives have landed and
 * before the hop adopts the new ownership.
 */
void peano4::topology::TreeTraversal::mergeTransitionRankBoundaryData(
  peano4::grid::GridTraversalObserver& observer
) const {
  runMerges(_transitionRankBoundaryMerges, observer);
}

/**
 * Rebuilds every consolidated rank-boundary entity's slot from the touchers'
 * copies. The first source rebuilds the slot and the further sources merge
 * into it, so the neighbour ranks receive the state of all tasks' copies.
 * Runs after the sweep and before the pack.
 */
void peano4::topology::TreeTraversal::runConsolidations(peano4::grid::GridTraversalObserver& observer) const {
  runMerges(_parallelRecording.consolidations, observer);
}

int peano4::topology::TreeTraversal::getNumberOfReplayTasks() const {
  return static_cast<int>(_parallelRecording.subtreeTasks.size());
}

int peano4::topology::TreeTraversal::getNumberOfSubtreeTasks() const {
  return static_cast<int>(_subtreeDecomposition.tasks.size());
}

/**
 * Returns the cell load of the subtree tasks, empty while no sweep has decomposed.
 */
const peano4::topology::TreeTraversal::SubtreeTaskLoad& peano4::topology::TreeTraversal::getSubtreeTaskLoad() const {
  return _subtreeTaskLoad;
}

/** Returns the task time of the sweep that ran last, a transition included. */
double peano4::topology::TreeTraversal::getLastSweepTaskSeconds() const { return _lastSweepTaskSeconds; }

const std::vector<std::pair<int, int>>& peano4::topology::TreeTraversal::getInterfaceCopyToFrozenVertexPairs() const {
  return _interfaceTopology.copyToFrozenVertexPairs;
}

const std::vector<std::pair<int, int>>& peano4::topology::TreeTraversal::getInterfaceCopyToFrozenFacePairs() const {
  return _interfaceTopology.copyToFrozenFacePairs;
}

const std::vector<std::pair<int, int>>& peano4::topology::TreeTraversal::
  getCreationalCopyToEntitySlotVertexPairs() const {
  return _interfaceTopology.creationalCopyToEntitySlotVertexPairs;
}

const std::vector<std::pair<int, int>>& peano4::topology::TreeTraversal::
  getCreationalCopyToEntitySlotFacePairs() const {
  return _interfaceTopology.creationalCopyToEntitySlotFacePairs;
}

const std::vector<std::pair<int, int>>& peano4::topology::TreeTraversal::getInterfaceFlushVertexPairs() const {
  return _interfaceTopology.flushVertexPairs;
}

const std::vector<std::pair<int, int>>& peano4::topology::TreeTraversal::getInterfaceFlushFacePairs() const {
  return _interfaceTopology.flushFacePairs;
}

const std::vector<std::pair<int, int>>& peano4::topology::TreeTraversal::getInterfaceSeedVertexPairs() const {
  return _interfaceTopology.seedVertexPairs;
}

const std::vector<std::pair<int, int>>& peano4::topology::TreeTraversal::getInterfaceSeedFacePairs() const {
  return _interfaceTopology.seedFacePairs;
}

const std::vector<int>& peano4::topology::TreeTraversal::getNeighbourRanks() const {
  return _rankTopology.neighbourRanks;
}

const std::vector<std::vector<int>>& peano4::topology::TreeTraversal::getVertexPackLists() const {
  return _rankTopology.vertexPackLists;
}

const std::vector<std::vector<int>>& peano4::topology::TreeTraversal::getFacePackLists() const {
  return _rankTopology.facePackLists;
}

const std::vector<int>& peano4::topology::TreeTraversal::getVertexFrozenSlotBases() const {
  return _rankTopology.vertexFrozenSlotBases;
}

const std::vector<int>& peano4::topology::TreeTraversal::getFaceFrozenSlotBases() const {
  return _rankTopology.faceFrozenSlotBases;
}

int peano4::topology::TreeTraversal::getNumberOfVertexRankFrozenSlots() const {
  return _rankTopology.numberOfVertexFrozenSlots;
}

int peano4::topology::TreeTraversal::getNumberOfFaceRankFrozenSlots() const {
  return _rankTopology.numberOfFaceFrozenSlots;
}

int peano4::topology::TreeTraversal::getNumberOfLocalCellSlots() const { return _rankTopology.numberOfLocalCellSlots; }

int peano4::topology::TreeTraversal::getCellSlotSpaceSize() const { return _rankTopology.cellSlotSpaceSize; }

int peano4::topology::TreeTraversal::getVertexSlotSpaceSize() const { return _rankTopology.vertexSlotSpaceSize; }

int peano4::topology::TreeTraversal::getFaceSlotSpaceSize() const { return _rankTopology.faceSlotSpaceSize; }

int peano4::topology::TreeTraversal::getNumberOfVertexShadowSlots() const {
  return _interfaceTopology.numberOfVertexShadowSlots;
}

int peano4::topology::TreeTraversal::getNumberOfFaceShadowSlots() const {
  return _interfaceTopology.numberOfFaceShadowSlots;
}

/**
 * Emits the pool-dialect enter and leave events of every child of a refined
 * cell and recurses into refined children. First and last touches come from
 * the scope's touch counters against the entity's adjacent-cell count, the
 * whole-domain count for the master and the serial traversal and the
 * precomputed unit-local count for a subtree task; both are known at enter,
 * because the siblings that touch one entity are visited only after this
 * cell's leave. A block task runs the lifecycle of an entity several blocks
 * touch on its own live slot and merges the partners' and the neighbour
 * ranks' frozen copies at its first touch, so no two concurrent tasks write
 * the same data.
 */
void peano4::topology::TreeTraversal::descendWithPools(
  const peano4::topology::Tree&        tree,
  const peano4::grid::AutomatonState&  state,
  const int                            cellIndex,
  peano4::grid::GridVertex             coarseGridVertices[peano4::VerticesPerCell],
  peano4::grid::GridTraversalObserver& observer,
  TraversalScope&                      scope
) const {
  assertion(_rankTopology.derived);
  assertion(cellIndex >= 0 and static_cast<std::size_t>(cellIndex) < tree.getNumberOfCells(), cellIndex);
  assertion(tree.isRefined(cellIndex), cellIndex);
  if (scope.unit >= 0 and not scope.writesEntitySlotsDirectly) {
    assertion(_interfaceTopology.derived);
    assertion(_interfaceTopology.unitOfCell.size() == tree.getNumberOfCells());
  }

  constexpr int FirstOrLastTouch = static_cast<int>(peano4::grid::StackTopology::BaseStack::Input);
  constexpr int RepeatTouch      = static_cast<int>(peano4::grid::StackTopology::BaseStack::Call);
  const bool    isCreational     = scope.sweepKind == PoolSweepKind::Creational;

  // The descent counts a cell's vertices with the cell's events, and the root
  // cell is nobody's child, so its own corners count here. A subtree task
  // whose root sits on level one descends from the root cell too, so only
  // the master's and the serial sweep's scope (unit -1) counts them.
  if (cellIndex == peano4::topology::Tree::RootCell and scope.unit < 0 and tree.getCell(cellIndex).ownerTreeId == _id) {
    scope.statistics.numberOfVertices += peano4::VerticesPerCell;
  }

  peano4::grid::AutomatonState fineGridStates[ChildrenPerCell];
  peano4::grid::AutomatonState::refineState(state, fineGridStates);

  // The rank owns every cell of its subtree tasks, so the cell, face and
  // vertex localities are rank-wide. A transition writes one copy per entity
  // and keeps that rank-wide view. The particle localities, which decide
  // which copy of a vertex holds a particle, narrow to the toucher's unit
  // while the tasks run the persistent-copy protocol: A task's copy then
  // holds the particles of the task's own cells, a partner task's copy
  // carries them as halo copies, and a lift out of the task's top level goes
  // through the sieve set.
  const bool narrowsLocalityToUnit = scope.skipsBlockRoots and not scope.writesEntitySlotsDirectly;
  assertion(not narrowsLocalityToUnit or _interfaceTopology.derived);
  const auto& unitOfCell       = _interfaceTopology.unitOfCell;
  const auto& parentCell       = tree.getCell(cellIndex);
  const auto  parentAreaOwners = gatherNeighbourhoodOwners(tree, parentCell, persistentCornerMask(coarseGridVertices));
  const bool  parentInUnit     = not narrowsLocalityToUnit or unitOfCell[cellIndex] == scope.unit;
  const bool  parentCellLocal  = state.level > 0 and parentCell.ownerTreeId == _id and parentInUnit;
  NeighbourhoodUnits parentAreaUnits{};
  auto parentVerticesLocal = areVerticesLocal(tree, parentCell, coarseGridVertices, parentAreaOwners, _id);
  if (narrowsLocalityToUnit) {
    parentAreaUnits = gatherNeighbourhoodUnits(tree, parentCell, unitOfCell);
    for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
      if (coarseGridVertices[vertexNumber].getState() == peano4::grid::GridVertex::State::HangingVertex) {
        parentVerticesLocal[vertexNumber] = parentVerticesLocal[vertexNumber] and parentInUnit;
        continue;
      }
      bool touchesUnit = false;
      for (int adjacencyEntry = 0; adjacencyEntry < peano4::CellsPerVertex and not touchesUnit; adjacencyEntry++) {
        touchesUnit = parentAreaUnits[AdjacentCells.cellInPatch[vertexNumber][adjacencyEntry]] == scope.unit;
      }
      parentVerticesLocal[vertexNumber] = parentVerticesLocal[vertexNumber] and touchesUnit;
    }
  }

  for (const auto& child : peano4::sfc::ActiveCurve::children(state.curveState)) {
    const auto  cellPositionLinearised = child.linearisedPosition;
    const auto  childCellIndex         = tree.getChild(cellIndex, cellPositionLinearised);
    const auto& childCell              = tree.getCell(childCellIndex);
    const auto& fineGridState          = fineGridStates[cellPositionLinearised];

    // A subtree without local cells is skipped entirely: Its events carry no
    // data, so a rank visits only its own blocks and the spine, and the
    // statistics count only what it visits. A split block's spawn cell stays
    // on the spine, so the descent reaches its owned children.
    if (childCell.level >= tree.getSpawnLevel() and not tree.subtreeTouchesRank(childCellIndex, _id)) {
      continue;
    }
    if (scope.writesEntitySlotsDirectly) {
      if (
        scope.directTaskRoot >= 0 and cellIndex == tree.getCell(scope.directTaskRoot).parent
        and childCellIndex != scope.directTaskRoot
      ) {
        continue;
      }
      if (_cellIsDirectTask[childCellIndex] != 0 and childCellIndex != scope.directTaskRoot) {
        continue;
      }
    } else {
      if (scope.unit >= 0 and _interfaceTopology.unitOfCell[childCellIndex] != scope.unit) {
        continue;
      }
      if (scope.skipsBlockRoots and scope.unit < 0 and _cellIsDirectTask[childCellIndex] != 0) {
        continue;
      }
    }

    peano4::grid::GridVertex fineGridVertices[peano4::VerticesPerCell];
    tree.gatherCellVertices(childCellIndex, fineGridVertices);

#if PEANO_DEBUG
    // A vertex carries the position its lattice index gives it, while the cell
    // geometry comes from the automaton's recursive refinement. The two
    // derivations have no code in common, so a mismatch exposes a cell that is visited at
    // the wrong place or a vertex index that names the wrong lattice site.
    for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        const double positionFromAutomaton =
          fineGridState.x(axis) + fineGridState.h(axis) * ((vertexNumber >> axis) & 1);
        const double positionFromLattice = fineGridVertices[vertexNumber].getX(axis);
        // The automaton and lattice calculations round independently.
        const double tolerance = tarch::la::NUMERICAL_ZERO_DIFFERENCE * _root.h(axis);
        // A periodic axis keeps one vertex for both walls, so the upper corner of
        // the last cell carries the lower wall's position.
        const bool wrapsOntoLowerWall = tree.getPeriodicBoundaries()[axis]
          and tarch::la::equals(positionFromLattice, positionFromAutomaton - _root.h(axis), tolerance);
        const bool positionsAgree =
          tarch::la::equals(positionFromLattice, positionFromAutomaton, tolerance) or wrapsOntoLowerWall;
        assertion(
          positionsAgree,
          childCellIndex,
          childCell.level,
          vertexNumber,
          axis,
          positionFromLattice,
          positionFromAutomaton
        );
      }
    }
#endif

    const bool isRefined = tree.isRefined(childCellIndex);
    const bool isLocal   = childCell.ownerTreeId == _id;

    peano4::grid::GridTraversalEvent enterNodeTraversalEvent;
    enterNodeTraversalEvent.level = fineGridState.level;
    enterNodeTraversalEvent.setX(fineGridState.x + fineGridState.h * 0.5);
    enterNodeTraversalEvent.setH(fineGridState.h);
    enterNodeTraversalEvent.setRelativePositionToFather(child.positionWithinParent());

    // A creational sweep builds the whole grid, so nothing existed before it
    // while the vertex states already tell what will exist below. On a
    // transition sweep the masks come from the old and the new structure's
    // states, so a refining cell reads as unrefined before and refined after
    // while an erasing one reads the other way round; every other sweep sees
    // a settled structure, where the two masks are the same set of refined
    // vertices.
    const auto regridRefinementMask = [&](const std::vector<char>& refinedFlags) {
      std::uint8_t mask = 0;
      for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
        if (
          fineGridVertices[vertexNumber].getState() != peano4::grid::GridVertex::State::HangingVertex
          and refinedFlags[childCell.vertexIndices[vertexNumber]] != 0
        ) {
          mask |= static_cast<std::uint8_t>(1U << vertexNumber);
        }
      }
      return mask;
    };
    const auto refinedVertices =
      static_cast<std::uint8_t>(peano4::grid::GridVertex::getRefinedVertices(fineGridVertices).to_ulong());
    const auto refinedVerticesBefore = scope.regridOverrides != nullptr
      ? regridRefinementMask(scope.regridOverrides->vertexRefinedBefore)
      : refinedVertices;
    enterNodeTraversalEvent.setHasBeenRefined(isCreational ? 0 : refinedVerticesBefore);
    enterNodeTraversalEvent.setWillBeRefined(
      scope.regridOverrides != nullptr ? regridRefinementMask(scope.regridOverrides->vertexRefinedAfter)
                                       : refinedVertices
    );

    const auto neighbourhoodOwners = gatherNeighbourhoodOwners(tree, childCell, persistentCornerMask(fineGridVertices));
    enterNodeTraversalEvent.setIsCellLocal(isLocal);
    enterNodeTraversalEvent.setIsFaceLocal(areFacesLocal(childCell, neighbourhoodOwners, _id).to_ulong());
    enterNodeTraversalEvent.setIsVertexLocal(
      areVerticesLocal(tree, childCell, fineGridVertices, neighbourhoodOwners, _id).to_ulong()
    );
    NeighbourhoodUnits neighbourhoodUnits{};
    if (narrowsLocalityToUnit) {
      neighbourhoodUnits = gatherNeighbourhoodUnits(tree, childCell, unitOfCell);
    }
    enterNodeTraversalEvent.setIsParentCellLocal(parentCellLocal);
    enterNodeTraversalEvent.setIsParentVertexLocal(parentVerticesLocal.to_ulong());

    std::uint32_t rankAdjacentCellsLocal = 0;
    std::uint32_t unitAdjacentCellsLocal = 0;
    for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
      const bool vertexHangs =
        fineGridVertices[vertexNumber].getState() == peano4::grid::GridVertex::State::HangingVertex;
      if (vertexHangs) {
        const auto vertexLattice = vertexLatticeOfCorner(tree, childCell, vertexNumber);
        for (int adjacencyEntry = 0; adjacencyEntry < peano4::CellsPerVertex; adjacencyEntry++) {
          if (tree.getAdjacentCellOwnerAt(childCell.level, vertexLattice, adjacencyEntry) == _id) {
            const auto bit = 1U << AdjacentCells.cellInPatch[vertexNumber][adjacencyEntry];
            rankAdjacentCellsLocal |= bit;
            unitAdjacentCellsLocal |= bit;
          }
        }
      } else {
        for (int adjacencyEntry = 0; adjacencyEntry < peano4::CellsPerVertex; adjacencyEntry++) {
          const auto patchPosition = AdjacentCells.cellInPatch[vertexNumber][adjacencyEntry];
          if (neighbourhoodOwners[patchPosition] == _id) {
            rankAdjacentCellsLocal |= 1U << patchPosition;
            if (not narrowsLocalityToUnit or neighbourhoodUnits[patchPosition] == scope.unit) {
              unitAdjacentCellsLocal |= 1U << patchPosition;
            }
          }
        }
      }
    }
    enterNodeTraversalEvent.setIsAdjacentCellLocal(unitAdjacentCellsLocal);

    // A first touch of an entity other tasks or neighbour ranks also hold
    // merges their frozen copies into the toucher's slot before the load
    // moves the slot into the call block. The merges run once the event is
    // complete and are recorded so the replays repeat them.
    struct PendingMerge {
      bool             isVertex{true};
      int              position{0};
      int              entitySlot{-1};
      int              liveSlot{-1};
      std::vector<int> sourceSlots{};
      int              numberOfPartnerSlots{0};
    };
    std::array<PendingMerge, peano4::VerticesPerCell + peano4::FacesPerCell> pendingMerges;
    int                                                                      numberOfPendingMerges = 0;

    // A migration hop can delete a rank boundary before the sweep after the
    // transition merges the partners' copies, so the transition records every
    // surviving rank-boundary entity's merge recipe for a replay once the
    // post-transition receives have landed.
    struct RankBoundaryRecord {
      int                     position{0};
      bool                    isVertex{true};
      const std::vector<int>* frozenSlots{nullptr};
    };
    std::array<RankBoundaryRecord, peano4::VerticesPerCell + peano4::FacesPerCell> rankBoundaryRecords;
    int                                                                            numberOfRankBoundaryRecords = 0;

    // Partner shadow copies hold nothing valid on a transition sweep, while
    // the migrated remote frozen copies stay readable.
    const bool isTransitionSweep = scope.regridOverrides != nullptr;

    // A vertex the rank holds counts its touches at every local cell and at
    // every remote ancestor of local cells down to the spawn level, so a block
    // root another rank owns moves the vertices it holds with the local cells
    // below, which then read the real coarse vertex data; every rank runs
    // the complete first- and last-touch lifecycle on its side of a
    // rank-boundary entity. A remote cell moves no cell or face data, no vertex
    // the rank does not hold, and nothing below the spawn level.
    const auto neighbourhoodTraversal      = gatherNeighbourhoodTraversal(tree, childCell, neighbourhoodOwners, _id);
    const bool remoteCellMovesHeldVertices = not isLocal and childCell.level <= tree.getSpawnLevel();
    bool       vertexIsLastTouch[peano4::VerticesPerCell];
    bool       vertexIsDestroyed[peano4::VerticesPerCell];
    bool       vertexMovesData[peano4::VerticesPerCell];
    for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
      const auto vertexIndex = childCell.vertexIndices[vertexNumber];
      assertion(
        vertexIndex >= 0 and static_cast<std::size_t>(vertexIndex) < _rankTopology.vertexSlotOfTreeVertex.size(),
        vertexIndex
      );
      const auto vertexSlot = _rankTopology.vertexSlotOfTreeVertex[vertexIndex];
      const bool vertexIsHanging =
        fineGridVertices[vertexNumber].getState() == peano4::grid::GridVertex::State::HangingVertex;
      if (isLocal and cellCountsVertex(neighbourhoodOwners, vertexNumber, _id)) {
        if (vertexIsHanging) {
          scope.statistics.numberOfHangingVertices++;
        } else {
          scope.statistics.numberOfVertices++;
        }
      }
      vertexIsDestroyed[vertexNumber] = false;
      vertexMovesData[vertexNumber] =
        isLocal or (remoteCellMovesHeldVertices and vertexSlot >= 0 and not vertexIsHanging);
      enterNodeTraversalEvent.setVertexDataTo(vertexNumber, vertexNumber);
      enterNodeTraversalEvent.setVertexPoolIndex(vertexNumber, vertexSlot);
      enterNodeTraversalEvent.setIsVertexHanging(vertexNumber, vertexIsHanging);
      if (not vertexMovesData[vertexNumber]) {
        vertexIsLastTouch[vertexNumber] = false;
        enterNodeTraversalEvent.setVertexDataFrom(vertexNumber, peano4::grid::GridTraversalObserver::NoData);
        continue;
      }
      assertion(vertexSlot >= 0 and vertexSlot < _rankTopology.vertexSlotSpaceSize, vertexSlot);

      const TaskTouchSlot*     taskTouchSlot   = nullptr;
      const InterfaceSlotPair* unitSlots       = nullptr;
      int                      vertexPoolIndex = vertexSlot;
      int                      vertexInterface = -1;
      if (scope.unit >= 0 and not scope.writesEntitySlotsDirectly) {
        assertion(static_cast<std::size_t>(vertexSlot) < _interfaceTopology.vertexInterfaceOfSlot.size(), vertexSlot);
        vertexInterface = _interfaceTopology.vertexInterfaceOfSlot[vertexSlot];
        if (vertexInterface >= 0) {
          for (const auto& candidate : _interfaceTopology.vertexInterfaceTouchSlots[vertexInterface]) {
            if (candidate.unit == scope.unit) {
              taskTouchSlot = &candidate;
              unitSlots     = &candidate.interfaceSlots;
              break;
            }
          }
        }
        if (unitSlots != nullptr) {
          assertion(
            unitSlots->live >= 0
              and unitSlots->live < _rankTopology.vertexSlotSpaceSize + _rankTopology.numberOfVertexFrozenSlots
                  + _interfaceTopology.numberOfVertexShadowSlots,
            unitSlots->live
          );
          assertion(
            unitSlots->frozen >= 0
              and unitSlots->frozen < _rankTopology.vertexSlotSpaceSize + _rankTopology.numberOfVertexFrozenSlots
                  + _interfaceTopology.numberOfVertexShadowSlots,
            unitSlots->frozen
          );
          vertexPoolIndex = unitSlots->live;
          enterNodeTraversalEvent.setVertexPoolIndex(vertexNumber, vertexPoolIndex);
        }
      }

      // A hanging vertex carries a per-visit copy: Every touching cell
      // creates its content anew and consumes it on leave, so no touch
      // counting, partner merging or exchange applies.
      if (vertexIsHanging) {
        vertexIsLastTouch[vertexNumber] = true;
        enterNodeTraversalEvent.setVertexDataFrom(
          vertexNumber,
          isCreational ? peano4::grid::GridTraversalObserver::CreateOrDestroyGridEntity : FirstOrLastTouch
        );
        continue;
      }

      int numberOfAdjacentCells = 0;
      if (taskTouchSlot != nullptr) {
        numberOfAdjacentCells = static_cast<int>(taskTouchSlot->numberOfAdjacentCells);
      } else {
        for (int adjacencyEntry = 0; adjacencyEntry < peano4::CellsPerVertex; adjacencyEntry++) {
          if (neighbourhoodTraversal[AdjacentCells.cellInPatch[vertexNumber][adjacencyEntry]]) {
            numberOfAdjacentCells++;
          }
        }
      }
      assertion(numberOfAdjacentCells > 0 and numberOfAdjacentCells <= peano4::CellsPerVertex, numberOfAdjacentCells);

      const bool vertexIsCreatedByRegrid = isTransitionSweep and scope.regridOverrides->vertexCreate[vertexIndex] != 0;
      vertexIsDestroyed[vertexNumber]    = isTransitionSweep and scope.regridOverrides->vertexDestroy[vertexIndex] != 0;

      const int touchCounter = taskTouchSlot != nullptr ? taskTouchSlot->counter : vertexSlot;
      if (scope.vertexTouchesOfAllUnits != nullptr) {
        assertion(
          touchCounter >= 0 and static_cast<std::size_t>(touchCounter) < scope.vertexTouchesOfAllUnits->size(),
          touchCounter,
          scope.vertexTouchesOfAllUnits->size()
        );
      } else {
        assertion(
          touchCounter >= 0 and static_cast<std::size_t>(touchCounter) < scope.vertexTouches.size(),
          touchCounter,
          scope.vertexTouches.size()
        );
      }
      auto& touches = scope.vertexTouchesOfAllUnits != nullptr ? (*scope.vertexTouchesOfAllUnits)[touchCounter]
                                                               : scope.vertexTouches[touchCounter];
      assertion(static_cast<int>(touches) < numberOfAdjacentCells, touches, numberOfAdjacentCells);
      touches++;
      const bool isFirstTouch         = touches == 1;
      vertexIsLastTouch[vertexNumber] = touches == numberOfAdjacentCells;

      if (isFirstTouch and scope.transitionRankBoundaryMerges != nullptr and not vertexIsDestroyed[vertexNumber]) {
        const auto* const boundaryFrozenSlots = findRemoteFrozenSlots(_rankTopology.remoteVertexPartners, vertexSlot);
        if (boundaryFrozenSlots != nullptr) {
          assertion(
            numberOfRankBoundaryRecords < static_cast<int>(rankBoundaryRecords.size()),
            numberOfRankBoundaryRecords
          );
          rankBoundaryRecords[numberOfRankBoundaryRecords++] = {vertexNumber, true, boundaryFrozenSlots};
        }
      }
      if ((isCreational or vertexIsCreatedByRegrid) and isFirstTouch) {
        enterNodeTraversalEvent.setVertexDataFrom(
          vertexNumber,
          peano4::grid::GridTraversalObserver::CreateOrDestroyGridEntity
        );
      } else {
        enterNodeTraversalEvent.setVertexDataFrom(vertexNumber, isFirstTouch ? FirstOrLastTouch : RepeatTouch);
      }
      if (not isCreational and not vertexIsCreatedByRegrid and isFirstTouch) {
        assertion(numberOfPendingMerges < static_cast<int>(pendingMerges.size()), numberOfPendingMerges);
        auto& pending = pendingMerges[numberOfPendingMerges];
        pending       = PendingMerge{true, vertexNumber, -1, -1, {}};
        if (unitSlots != nullptr) {
          for (const auto& candidate : _interfaceTopology.vertexInterfaceTouchSlots[vertexInterface]) {
            if (candidate.unit >= 0 and candidate.unit != scope.unit) {
              assertion(
                candidate.interfaceSlots.frozen >= 0
                  and candidate.interfaceSlots.frozen < _rankTopology.vertexSlotSpaceSize
                      + _rankTopology.numberOfVertexFrozenSlots + _interfaceTopology.numberOfVertexShadowSlots,
                candidate.interfaceSlots.frozen
              );
              pending.sourceSlots.push_back(candidate.interfaceSlots.frozen);
            }
          }
        }
        pending.numberOfPartnerSlots  = static_cast<int>(pending.sourceSlots.size());
        const auto* const frozenSlots = findRemoteFrozenSlots(_rankTopology.remoteVertexPartners, vertexSlot);
        if (frozenSlots != nullptr) {
          if (isTransitionSweep) {
            for (const auto frozenSlot : *frozenSlots) {
              assertion(
                frozenSlot >= 0
                  and static_cast<std::size_t>(frozenSlot) < scope.regridOverrides->vertexFrozenSlotValid.size(),
                frozenSlot,
                scope.regridOverrides->vertexFrozenSlotValid.size()
              );
              if (scope.regridOverrides->vertexFrozenSlotValid[frozenSlot] != 0) {
                pending.sourceSlots.push_back(frozenSlot);
              }
            }
          } else {
            pending.sourceSlots.insert(pending.sourceSlots.end(), frozenSlots->begin(), frozenSlots->end());
          }
          if (unitSlots != nullptr) {
            pending.entitySlot = vertexSlot;
            pending.liveSlot   = vertexPoolIndex;
          }
        }
        if (not pending.sourceSlots.empty()) {
          numberOfPendingMerges++;
        }
      }
    }

    bool faceIsLastTouch[peano4::FacesPerCell];
    bool faceIsDestroyed[peano4::FacesPerCell];
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      const auto faceIndex = childCell.faceIndices[faceNumber];
      assertion(
        faceIndex >= 0 and static_cast<std::size_t>(faceIndex) < _rankTopology.faceSlotOfTreeFace.size(),
        faceIndex
      );
      const auto faceSlot      = _rankTopology.faceSlotOfTreeFace[faceIndex];
      const int  normal        = faceNumber % DIMENSIONS;
      const int  faceSide      = faceNumber < DIMENSIONS ? 0 : 1;
      bool       faceIsHanging = true;
      for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
        if (
          ((vertexNumber >> normal) & 1) == faceSide
          and fineGridVertices[vertexNumber].getState() != peano4::grid::GridVertex::State::HangingVertex
        ) {
          faceIsHanging = false;
        }
      }
      faceIsDestroyed[faceNumber] = false;
      enterNodeTraversalEvent.setFaceDataTo(faceNumber, faceNumber);
      enterNodeTraversalEvent.setFacePoolIndex(faceNumber, faceSlot);
      enterNodeTraversalEvent.setIsFaceHanging(faceNumber, faceIsHanging);
      if (not isLocal) {
        faceIsLastTouch[faceNumber] = false;
        enterNodeTraversalEvent.setFaceDataFrom(faceNumber, peano4::grid::GridTraversalObserver::NoData);
        continue;
      }
      assertion(faceSlot >= 0 and faceSlot < _rankTopology.faceSlotSpaceSize, faceSlot);

      const TaskTouchSlot*     taskTouchSlot = nullptr;
      const InterfaceSlotPair* unitSlots     = nullptr;
      int                      facePoolIndex = faceSlot;
      int                      faceInterface = -1;
      if (scope.unit >= 0 and not scope.writesEntitySlotsDirectly) {
        assertion(static_cast<std::size_t>(faceSlot) < _interfaceTopology.faceInterfaceOfSlot.size(), faceSlot);
        faceInterface = _interfaceTopology.faceInterfaceOfSlot[faceSlot];
        if (faceInterface >= 0) {
          for (const auto& candidate : _interfaceTopology.faceInterfaceTouchSlots[faceInterface]) {
            if (candidate.unit == scope.unit) {
              taskTouchSlot = &candidate;
              unitSlots     = &candidate.interfaceSlots;
              break;
            }
          }
        }
        if (unitSlots != nullptr) {
          assertion(
            unitSlots->live >= 0
              and unitSlots->live < _rankTopology.faceSlotSpaceSize + _rankTopology.numberOfFaceFrozenSlots
                  + _interfaceTopology.numberOfFaceShadowSlots,
            unitSlots->live
          );
          assertion(
            unitSlots->frozen >= 0
              and unitSlots->frozen < _rankTopology.faceSlotSpaceSize + _rankTopology.numberOfFaceFrozenSlots
                  + _interfaceTopology.numberOfFaceShadowSlots,
            unitSlots->frozen
          );
          facePoolIndex = unitSlots->live;
          enterNodeTraversalEvent.setFacePoolIndex(faceNumber, facePoolIndex);
        }
      }

      // A hanging face carries a per-visit copy exactly like a hanging
      // vertex, so its lifecycle needs no touch counting or merging.
      if (faceIsHanging) {
        faceIsLastTouch[faceNumber] = true;
        enterNodeTraversalEvent.setFaceDataFrom(
          faceNumber,
          isCreational ? peano4::grid::GridTraversalObserver::CreateOrDestroyGridEntity : FirstOrLastTouch
        );
        continue;
      }

      const int numberOfAdjacentCells = taskTouchSlot != nullptr
        ? static_cast<int>(taskTouchSlot->numberOfAdjacentCells)
        : tree.getNumberOfLocalAdjacentCellsOfFace(childCellIndex, faceNumber);
      assertion(numberOfAdjacentCells > 0 and numberOfAdjacentCells <= 2, numberOfAdjacentCells);

      const bool faceIsCreatedByRegrid = isTransitionSweep and scope.regridOverrides->faceCreate[faceIndex] != 0;
      faceIsDestroyed[faceNumber]      = isTransitionSweep and scope.regridOverrides->faceDestroy[faceIndex] != 0;

      const int touchCounter = taskTouchSlot != nullptr ? taskTouchSlot->counter : faceSlot;
      if (scope.faceTouchesOfAllUnits != nullptr) {
        assertion(
          touchCounter >= 0 and static_cast<std::size_t>(touchCounter) < scope.faceTouchesOfAllUnits->size(),
          touchCounter,
          scope.faceTouchesOfAllUnits->size()
        );
      } else {
        assertion(
          touchCounter >= 0 and static_cast<std::size_t>(touchCounter) < scope.faceTouches.size(),
          touchCounter,
          scope.faceTouches.size()
        );
      }
      auto& touches = scope.faceTouchesOfAllUnits != nullptr ? (*scope.faceTouchesOfAllUnits)[touchCounter]
                                                             : scope.faceTouches[touchCounter];
      assertion(static_cast<int>(touches) < numberOfAdjacentCells, touches, numberOfAdjacentCells);
      touches++;
      const bool isFirstTouch     = touches == 1;
      faceIsLastTouch[faceNumber] = touches == numberOfAdjacentCells;

      if (isFirstTouch and scope.transitionRankBoundaryMerges != nullptr and not faceIsDestroyed[faceNumber]) {
        const auto* const boundaryFrozenSlots = findRemoteFrozenSlots(_rankTopology.remoteFacePartners, faceSlot);
        if (boundaryFrozenSlots != nullptr) {
          assertion(
            numberOfRankBoundaryRecords < static_cast<int>(rankBoundaryRecords.size()),
            numberOfRankBoundaryRecords
          );
          rankBoundaryRecords[numberOfRankBoundaryRecords++] = {faceNumber, false, boundaryFrozenSlots};
        }
      }
      if ((isCreational or faceIsCreatedByRegrid) and isFirstTouch) {
        enterNodeTraversalEvent.setFaceDataFrom(
          faceNumber,
          peano4::grid::GridTraversalObserver::CreateOrDestroyGridEntity
        );
      } else {
        enterNodeTraversalEvent.setFaceDataFrom(faceNumber, isFirstTouch ? FirstOrLastTouch : RepeatTouch);
      }
      if (not isCreational and not faceIsCreatedByRegrid and isFirstTouch) {
        assertion(numberOfPendingMerges < static_cast<int>(pendingMerges.size()), numberOfPendingMerges);
        auto& pending = pendingMerges[numberOfPendingMerges];
        pending       = PendingMerge{false, faceNumber, -1, -1, {}};
        if (unitSlots != nullptr) {
          for (const auto& candidate : _interfaceTopology.faceInterfaceTouchSlots[faceInterface]) {
            if (candidate.unit >= 0 and candidate.unit != scope.unit) {
              assertion(
                candidate.interfaceSlots.frozen >= 0
                  and candidate.interfaceSlots.frozen < _rankTopology.faceSlotSpaceSize
                      + _rankTopology.numberOfFaceFrozenSlots + _interfaceTopology.numberOfFaceShadowSlots,
                candidate.interfaceSlots.frozen
              );
              pending.sourceSlots.push_back(candidate.interfaceSlots.frozen);
            }
          }
        }
        pending.numberOfPartnerSlots  = static_cast<int>(pending.sourceSlots.size());
        const auto* const frozenSlots = findRemoteFrozenSlots(_rankTopology.remoteFacePartners, faceSlot);
        if (frozenSlots != nullptr) {
          if (isTransitionSweep) {
            for (const auto frozenSlot : *frozenSlots) {
              assertion(
                frozenSlot >= 0
                  and static_cast<std::size_t>(frozenSlot) < scope.regridOverrides->faceFrozenSlotValid.size(),
                frozenSlot,
                scope.regridOverrides->faceFrozenSlotValid.size()
              );
              if (scope.regridOverrides->faceFrozenSlotValid[frozenSlot] != 0) {
                pending.sourceSlots.push_back(frozenSlot);
              }
            }
          } else {
            pending.sourceSlots.insert(pending.sourceSlots.end(), frozenSlots->begin(), frozenSlots->end());
          }
          if (unitSlots != nullptr) {
            pending.entitySlot = faceSlot;
            pending.liveSlot   = facePoolIndex;
          }
        }
        if (not pending.sourceSlots.empty()) {
          numberOfPendingMerges++;
        }
      }
    }

    const bool cellIsCreatedByRegrid   = isTransitionSweep and scope.regridOverrides->cellCreate[childCellIndex] != 0;
    const bool cellIsDestroyedByRegrid = isTransitionSweep and scope.regridOverrides->cellDestroy[childCellIndex] != 0;
    const auto cellSlot                = _rankTopology.cellSlotOfTreeCell[childCellIndex];
    assertion(not isLocal or (cellSlot >= 0 and cellSlot < _rankTopology.cellSlotSpaceSize), cellSlot);
    enterNodeTraversalEvent.setCellPoolIndex(cellSlot);
    if (not isLocal) {
      enterNodeTraversalEvent.setCellData(peano4::grid::GridTraversalObserver::NoData);
    } else if (isCreational or cellIsCreatedByRegrid) {
      enterNodeTraversalEvent.setCellData(peano4::grid::GridTraversalObserver::CreateOrDestroyGridEntity);
    } else {
      enterNodeTraversalEvent.setCellData(FirstOrLastTouch);
    }

    int numberOfRecordedMerges = 0;
    for (int mergeNumber = 0; mergeNumber < numberOfPendingMerges; mergeNumber++) {
      auto& pending = pendingMerges[mergeNumber];
      for (std::size_t sourceNumber = 0; sourceNumber < pending.sourceSlots.size(); sourceNumber++) {
        observer.receiveNodeData(
          pending.isVertex ? peano4::grid::GridTraversalObserver::NodeData::Vertex
                           : peano4::grid::GridTraversalObserver::NodeData::Face,
          pending.position,
          pending.sourceSlots[sourceNumber],
          sourceNumber < static_cast<std::size_t>(pending.numberOfPartnerSlots)
            ? peano4::grid::SendReceiveContext::IntraRank
            : peano4::grid::SendReceiveContext::InterRank,
          enterNodeTraversalEvent
        );
      }
      if (scope.consolidationSeeds != nullptr and pending.entitySlot >= 0) {
        // The consolidation rebuilds the rank's copy of the entity from the
        // tasks' copies, so its merges see the rank-wide particle locality.
        auto seedEvent = enterNodeTraversalEvent;
        seedEvent.setIsAdjacentCellLocal(rankAdjacentCellsLocal);
        if (pending.isVertex) {
          seedEvent.setVertexPoolIndex(pending.position, pending.entitySlot);
        } else {
          seedEvent.setFacePoolIndex(pending.position, pending.entitySlot);
        }
        scope.consolidationSeeds->push_back(
          {seedEvent, pending.position, pending.isVertex, pending.entitySlot, pending.liveSlot}
        );
      }
      if (scope.inlineMerges != nullptr) {
        scope.inlineMerges->push_back(
          {enterNodeTraversalEvent,
           pending.position,
           pending.isVertex,
           std::move(pending.sourceSlots),
           pending.numberOfPartnerSlots}
        );
        numberOfRecordedMerges++;
      }
    }
    for (int recordNumber = 0; recordNumber < numberOfRankBoundaryRecords; recordNumber++) {
      const auto& record = rankBoundaryRecords[recordNumber];
      // The recorded merges replay on the post-regrid structure, so the merge
      // predicates must see the new refinement state.
      auto boundaryEvent = enterNodeTraversalEvent;
      boundaryEvent.setHasBeenRefined(boundaryEvent.getWillBeRefined());
      scope.transitionRankBoundaryMerges->push_back(
        {boundaryEvent, record.position, record.isVertex, *record.frozenSlots, 0}
      );
    }

    observer.enterNode(enterNodeTraversalEvent);
    if (scope.entries != nullptr) {
      scope.entries->push_back({enterNodeTraversalEvent, numberOfRecordedMerges, true});
    }

    // A transition sweep traverses the union structure, but the statistics
    // describe the grid the sweep leaves behind: Destroyed cells do not
    // count, and the after-masks decide which cells count as refined.
    if (not cellIsDestroyedByRegrid) {
      scope.statistics.minH      = tarch::la::min(scope.statistics.minH, 1.0 / SubdivisionFactor * state.h);
      const bool countsAsRefined = isTransitionSweep ? enterNodeTraversalEvent.getWillBeRefined() != 0 : isRefined;
      if (countsAsRefined) {
        if (isLocal) {
          scope.statistics.numberOfLocalRefinedCells++;
        } else {
          scope.statistics.numberOfRemoteRefinedCells++;
        }
      } else {
        if (isLocal) {
          scope.statistics.numberOfLocalUnrefinedCells++;
          scope.statistics.minDepth = std::min(scope.statistics.minDepth, fineGridState.level);
          scope.statistics.maxDepth = std::max(scope.statistics.maxDepth, fineGridState.level);
        } else {
          scope.statistics.numberOfRemoteUnrefinedCells++;
        }
      }
    }
    // The master and the subtree tasks stop at every block root: The root's
    // own events belong to the task above it, its subtree to the root's
    // task. The serial whole-domain traversal descends everywhere.
    if (isRefined and (not scope.skipsBlockRoots or _cellIsBlockRoot[childCellIndex] == 0)) {
      descendWithPools(tree, fineGridState, childCellIndex, fineGridVertices, observer, scope);
    }

    auto leaveNodeTraversalEvent = enterNodeTraversalEvent;
    for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
      leaveNodeTraversalEvent.setVertexDataFrom(vertexNumber, vertexNumber);
      int vertexDataTo = RepeatTouch;
      if (not vertexMovesData[vertexNumber]) {
        vertexDataTo = peano4::grid::GridTraversalObserver::NoData;
      } else if (vertexIsLastTouch[vertexNumber]) {
        vertexDataTo = vertexIsDestroyed[vertexNumber] ? peano4::grid::GridTraversalObserver::CreateOrDestroyGridEntity
                                                       : FirstOrLastTouch;
      }
      leaveNodeTraversalEvent.setVertexDataTo(vertexNumber, vertexDataTo);
    }
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      leaveNodeTraversalEvent.setFaceDataFrom(faceNumber, faceNumber);
      int faceDataTo = RepeatTouch;
      if (not isLocal) {
        faceDataTo = peano4::grid::GridTraversalObserver::NoData;
      } else if (faceIsLastTouch[faceNumber]) {
        faceDataTo = faceIsDestroyed[faceNumber] ? peano4::grid::GridTraversalObserver::CreateOrDestroyGridEntity
                                                 : FirstOrLastTouch;
      }
      leaveNodeTraversalEvent.setFaceDataTo(faceNumber, faceDataTo);
    }
    int cellDataTo = FirstOrLastTouch;
    if (not isLocal) {
      cellDataTo = peano4::grid::GridTraversalObserver::NoData;
    } else if (cellIsDestroyedByRegrid) {
      cellDataTo = peano4::grid::GridTraversalObserver::CreateOrDestroyGridEntity;
    }
    leaveNodeTraversalEvent.setCellData(cellDataTo);

    observer.leaveNode(leaveNodeTraversalEvent);
    if (scope.entries != nullptr) {
      scope.entries->push_back({leaveNodeTraversalEvent, 0, false});
    }
  }
}
