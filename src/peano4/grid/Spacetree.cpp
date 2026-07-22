// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Spacetree.h"

#include "peano4/Loop.h"
#include "peano4/grid/GridControlEvent.h"
#include "peano4/grid/GridTraversalEvent.h"
#include "peano4/grid/GridTraversalObserver.h"
#include "peano4/grid/PeanoCurve.h"
#include "peano4/grid/SpacetreeHelper.h"
#include "peano4/grid/StackTopology.h"
#include "peano4/grid/VertexType.h"
#include "tarch/Termination.h"
#include "tarch/instrumentation/Log.h"

namespace {
  static constexpr auto RankOfPeriodicBoundaryCondition =
    peano4::grid::SpacetreeHelper::RankOfPeriodicBoundaryCondition;
  static constexpr auto NumberOfStationarySweepsToWaitAtLeastTillJoin =
    peano4::grid::SpacetreeHelper::NumberOfStationarySweepsToWaitAtLeastTillJoin;

  inline static bool hasPeriodicBoundaryConditions(const std::array<int, TWO_POWER_D>& adjacentRanks) {
    for (int adjacencyIndex = 0; adjacencyIndex < TWO_POWER_D; ++adjacencyIndex) {
      if (adjacentRanks[adjacencyIndex] == RankOfPeriodicBoundaryCondition) {
        return true;
      }
    }
    return false;
  }
} // namespace

/**
 * Construct a new, self-contained root tree. The global root is tree 0;
 * further root trees (other ids) host independent domains.
 */
peano4::grid::Spacetree::Spacetree(
  int                                          newRootId,
  const tarch::la::Vector<DIMENSIONS, double>& offset,
  const tarch::la::Vector<DIMENSIONS, double>& width,
  const std::bitset<DIMENSIONS>&               periodicBC
) :
  _id(newRootId),
  _spacetreeState(SpacetreeState::NewRoot),
  _root{},
  _statistics{},
  _masterId(-1),
  _periodicBC(periodicBC),
  _splitTriggered(),
  _nextSplitInstruction(_splitTriggered.end()),
  _gridTraversalEventGenerator(newRootId) {
  _root.x = offset;
  _root.h = width;
  _root.setInverted(false);

  for (int accessNumberIndex = 0; accessNumberIndex < TWO_TIMES_D; accessNumberIndex++) {
    _root.accessNumber[accessNumberIndex] = 0;
  }

  _statistics.numberOfStationarySweeps = 0;
  _statistics.clear(_id == 0);

  _noParallelTopologyChanges = true;

  logDebug(
    "Created root spacetree " << _id << " with " << offset << "x" << width << " and periodic BCs=" << _periodicBC
  );
}

/**
 * Every constructor takes the periodic configuration explicitly: a worker
 * that does not know about the periodic axes cannot detect its periodic
 * boundaries, so a forgotten propagation must fail to compile rather than
 * silently default to non-periodic.
 */
peano4::grid::Spacetree::Spacetree(
  int                                          newId,
  int                                          masterId,
  const tarch::la::Vector<DIMENSIONS, double>& offset,
  const tarch::la::Vector<DIMENSIONS, double>& width,
  bool                                         traversalInverted,
  const std::bitset<DIMENSIONS>&               periodicBC
) :
  _id(newId),
  _spacetreeState(SpacetreeState::EmptyRun),
  _root{},
  _statistics{},
  _masterId(masterId),
  _periodicBC(periodicBC),
  _splitTriggered(),
  _nextSplitInstruction(_splitTriggered.end()),
  _splitting(),
  _gridTraversalEventGenerator(newId) {
  _root.level = 0;
  _root.x     = offset;
  _root.h     = width;
  _root.setInverted(traversalInverted);

  for (int accessNumberIndex = 0; accessNumberIndex < TWO_TIMES_D; accessNumberIndex++) {
    _root.accessNumber[accessNumberIndex] = 0;
  }

  _statistics.numberOfStationarySweeps = -NumberOfStationarySweepsToWaitAtLeastTillJoin - 2;
  _statistics.clear(_id == 0);

  _noParallelTopologyChanges = true;

  logDebug("Created spacetree " << _id << " with master tree " << masterId << ": " << _root.toString());
}

int peano4::grid::Spacetree::getOrCreateNeighbourSlot(int neighbourTreeId) {
  assertion(neighbourTreeId >= 0, neighbourTreeId);
  for (std::size_t slotIndex = 0; slotIndex < _neighbourTreeIds.size(); ++slotIndex) {
    if (_neighbourTreeIds[slotIndex] == neighbourTreeId) {
      return static_cast<int>(slotIndex);
    }
  }
  const int neighbourSlot = static_cast<int>(_neighbourTreeIds.size());
  _neighbourTreeIds.push_back(neighbourTreeId);
  _parallelExchangeVertexStacks.emplace_back();
  return neighbourSlot;
}

int peano4::grid::Spacetree::getNeighbourSlot(int neighbourTreeId) const {
  assertion(neighbourTreeId >= 0, neighbourTreeId);
  for (std::size_t slotIndex = 0; slotIndex < _neighbourTreeIds.size(); ++slotIndex) {
    if (_neighbourTreeIds[slotIndex] == neighbourTreeId) {
      return static_cast<int>(slotIndex);
    }
  }
  return -1;
}

peano4::stacks::Stack<peano4::grid::GridVertex>&
peano4::grid::Spacetree::getLocalExchangeVertexStack(int localStackId) {
  assertion(
    localStackId >= 0 and localStackId < StackTopology::TotalNumberOfLocalExchangeStacks,
    localStackId,
    StackTopology::TotalNumberOfLocalExchangeStacks
  );
  return _localExchangeVertexStacks[localStackId];
}

/**
 * Stamp list running in tandem with the periodic boundary entries of the
 * local exchange vertex stack localStackId. SpacetreeSet uses it to route
 * grid vertices across the wrap by identity.
 */
std::vector<peano4::grid::PeriodicStamp>& peano4::grid::Spacetree::getLocalExchangeVertexStamps(int localStackId) {
  assertion(
    localStackId >= 0 and localStackId < StackTopology::TotalNumberOfLocalExchangeStacks,
    localStackId,
    StackTopology::TotalNumberOfLocalExchangeStacks
  );
  return _localExchangeVertexStackStamps[localStackId];
}

const peano4::stacks::Stack<peano4::grid::GridVertex>&
peano4::grid::Spacetree::getLocalExchangeVertexStack(int localStackId) const {
  assertion(
    localStackId >= 0 and localStackId < StackTopology::TotalNumberOfLocalExchangeStacks,
    localStackId,
    StackTopology::TotalNumberOfLocalExchangeStacks
  );
  return _localExchangeVertexStacks[localStackId];
}

peano4::stacks::Stack<peano4::grid::GridVertex>& peano4::grid::Spacetree::getParallelExchangeVertexStack(
  int                                  neighbourSlot,
  StackTopology::ParallelExchangeStack exchangeStack
) {
  assertion(
    neighbourSlot >= 0 and neighbourSlot < static_cast<int>(_parallelExchangeVertexStacks.size()),
    neighbourSlot,
    static_cast<int>(_parallelExchangeVertexStacks.size())
  );
  return _parallelExchangeVertexStacks[neighbourSlot][StackTopology::toIndex(exchangeStack)];
}

const peano4::stacks::Stack<peano4::grid::GridVertex>& peano4::grid::Spacetree::getParallelExchangeVertexStack(
  int                                  neighbourSlot,
  StackTopology::ParallelExchangeStack exchangeStack
) const {
  assertion(
    neighbourSlot >= 0 and neighbourSlot < static_cast<int>(_parallelExchangeVertexStacks.size()),
    neighbourSlot,
    static_cast<int>(_parallelExchangeVertexStacks.size())
  );
  return _parallelExchangeVertexStacks[neighbourSlot][StackTopology::toIndex(exchangeStack)];
}

int peano4::grid::Spacetree::getNumberOfNeighbourTrees() const { return static_cast<int>(_neighbourTreeIds.size()); }

int peano4::grid::Spacetree::getNeighbourTreeIdForSlot(int neighbourSlot) const {
  assertion(neighbourSlot >= 0 and neighbourSlot < static_cast<int>(_neighbourTreeIds.size()));
  return _neighbourTreeIds[neighbourSlot];
}

/**
 * Traverse the tree and describe each grid action to the observer through a
 * GridTraversalEvent.
 */
void peano4::grid::Spacetree::traverse(GridTraversalObserver& observer) {
  traceEnter();

  for (int accessNumberIndex = 0; accessNumberIndex < TWO_TIMES_D; accessNumberIndex++) {
    assertion(_root.accessNumber[accessNumberIndex] > -2 * DIMENSIONS, accessNumberIndex, _root.toString());
    assertion(_root.accessNumber[accessNumberIndex] < 2 * DIMENSIONS, accessNumberIndex, _root.toString());
  }

  assertion(_joinTriggered.empty() or _splitTriggered.empty());
  assertion(_joinTriggered.empty() or _splitting.empty());
  assertion(_joining.empty() or _splitTriggered.empty());
  assertion(_joining.empty() or _splitting.empty());
  logDebug(
    "Tree " << _id << " has " << _splitTriggered.size() << " triggered splits and " << _splitting.size()
            << " splitting trees."
  );

  _statistics.clear(_id == 0);

  _periodicProducerDescriptors.clear();
  _periodicConsumerDescriptors.clear();

  if (_spacetreeState == SpacetreeState::EmptyRun or _spacetreeState == SpacetreeState::NewFromSplit
      or _spacetreeState == SpacetreeState::Joining) {
    _gridControlEvents.clear();
  } else {
    _gridControlEvents = observer.getGridControlEvents();
    // An observer may hand out any set of events, so we fuse it here and every cell then
    // tests against as few boxes as possible. The fusion changes no cell's verdict.
    GridControlEvent::merge(_gridControlEvents);
    if (not _gridControlEvents.empty()) {
      logDebug("Spacetree " << _id << " got " << _gridControlEvents.size() << " grid control events");
    }
  }
  _gridControlEventsContainErase =
    std::any_of(_gridControlEvents.begin(), _gridControlEvents.end(), [](const GridControlEvent& event) {
      return event.getRefinementControl() == GridControlEvent::RefinementControl::Erase;
    });

  _splittedCells.clear();
  resetSplittingTreeCursor();

  _noParallelTopologyChanges =
    _splitTriggered.empty() and _splitting.empty() and _joinTriggered.empty() and _joining.empty();

  const bool isFirstTraversal = _spacetreeState == SpacetreeState::NewRoot;

  GridVertex vertices[TWO_POWER_D];
  peano4::dForEachCorner([&](const tarch::la::Vector<DIMENSIONS, int>&, int cornerIndex) {
    std::array<int, TWO_POWER_D> adjacentRanks;
    adjacentRanks.fill(-1);
    // A root tree owns its level-0 cell; on a worker the cell belongs to the
    // master's chain, which keeps every coarse level remote there.
    adjacentRanks[TWO_POWER_D - 1 - cornerIndex] = _masterId >= 0 ? _masterId : _id;
    vertices[cornerIndex]                        = GridVertex::create(
      0,
      isFirstTraversal ? GridVertex::State::Refining : GridVertex::State::Refined,
      _root.x,
      _root.h,
      cornerIndex,
      adjacentRanks,
      false
    );
    vertices[cornerIndex].setFlag(GridVertex::HasBeenParentOfSubtreeVertexInPreviousTreeSweep, true);
    logDebug("Create " << vertices[cornerIndex].toString() << " for tree " << _id);
  });

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    if (_periodicBC[axis]) {
      peano4::dForEachCorner([&](const tarch::la::Vector<DIMENSIONS, int>& vertexPosition, int vertexIndex) {
        peano4::dForEachCorner([&](const tarch::la::Vector<DIMENSIONS, int>& adjacencyPosition, int) {
          auto adjacencyEntry = adjacencyPosition;
          if (vertexPosition(axis) == 0) {
            adjacencyEntry(axis) = 0;
          } else {
            adjacencyEntry(axis) = 1;
          }
          vertices[vertexIndex].adjacentRanks[peano4::dLinearised(adjacencyEntry, 2)] = RankOfPeriodicBoundaryCondition;
          vertices[vertexIndex].backupOfAdjacentRanks[peano4::dLinearised(adjacencyEntry, 2)] =
            RankOfPeriodicBoundaryCondition;
        });
        logDebug("Set periodic boundary conditions: " << vertices[vertexIndex].toString());
      });
    }
  }

  observer.beginTraversal(_root.x, _root.h);
  descend(_root, tarch::la::Vector<DIMENSIONS, int>(0), vertices, observer);

  _hasSplit.clear();
  for (const auto splittingTreeId : _splitting) {
    _hasSplit.insert(splittingTreeId);
  }
  _splitting.clear();
  for (const auto& splitEntry : _splitTriggered) {
    if (splitEntry.second.numberOfFineGridCells > 0) {
      std::ostringstream errorMessage;
      errorMessage << "Tree " << _id << " cannot assign " << splitEntry.second.numberOfFineGridCells
                   << " remaining cells to new tree " << splitEntry.first
                   << ". Exact split requests must not be completed partially.";
      tarch::terminateWithError(errorMessage.str());
    }
    _splitting.insert(splitEntry.first);
    static_cast<void>(getOrCreateNeighbourSlot(splitEntry.first));
  }
  _splitTriggered.clear();
  resetSplittingTreeCursor();

  _joining.clear();
  _joining.insert(_joinTriggered.begin(), _joinTriggered.end());
  _joinTriggered.clear();

  observer.endTraversal(_root.x, _root.h);

  {
    std::size_t numberOfResidentVertices = 0;
    for (const auto& stack : _localExchangeVertexStacks) {
      numberOfResidentVertices += stack.size();
    }
    for (const auto& neighbourStacks : _parallelExchangeVertexStacks) {
      for (const auto& stack : neighbourStacks) {
        numberOfResidentVertices += stack.size();
      }
    }
    _statistics.numberOfVertices = static_cast<int>(numberOfResidentVertices);
  }

  _root.setInverted(not _root.getInverted());

  [[maybe_unused]] const auto previousState = _spacetreeState;
  switch (_spacetreeState) {
  case SpacetreeState::EmptyRun:
    _spacetreeState = SpacetreeState::NewFromSplit;
    break;
  case SpacetreeState::NewRoot:
  case SpacetreeState::NewFromSplit:
    _spacetreeState = SpacetreeState::Running;
    break;
  case SpacetreeState::JoinTriggered:
    _spacetreeState = SpacetreeState::Joining;
    break;
  case SpacetreeState::Joining:
    _spacetreeState = SpacetreeState::Joined;
    break;
  case SpacetreeState::Running:
  case SpacetreeState::Joined:
    break;
  }
  logDebug(
    "Switched tree " << _id << " from " << grid::toString(previousState) << " into " << grid::toString(_spacetreeState)
  );

  traceLeave();
}

/**
 * Merge joining-worker adjacency after horizontal boundary data. Only the
 * live adjacency changes because outgoing user-data routing still needs the
 * backup from the start of the traversal.
 *
 * @see receiveAndMergeGridVertexAtHorizontalBoundary()
 */
void peano4::grid::Spacetree::receiveAndMergeGridVertexAtVerticalBoundary(GridVertex& vertex) {
  for (const auto joiningTreeId : _joining) {
    const auto& adjacentRanks = vertex.adjacentRanks;
    if (std::find(std::begin(adjacentRanks), std::end(adjacentRanks), joiningTreeId) != std::end(adjacentRanks)) {
      const int neighbourSlot = getOrCreateNeighbourSlot(joiningTreeId);
      logDebug(
        "Merge vertex " << vertex.toString() << " with the replica from joining worker " << joiningTreeId << "."
      );

      auto& inputStack =
        _parallelExchangeVertexStacks[neighbourSlot]
                                     [StackTopology::toIndex(StackTopology::ParallelExchangeStack::VerticalInput)];
      assertion(not inputStack.empty(), _id, joiningTreeId, vertex.toString());
      GridVertex incomingVertex = inputStack.pop();

#if PEANO_DEBUG
      assertion(
        tarch::la::equals(vertex.getX(), incomingVertex.getX()),
        vertex.getX(),
        incomingVertex.getX(),
        vertex.toString(),
        incomingVertex.toString(),
        _id
      );
#endif

      vertex.adjacentRanks = incomingVertex.adjacentRanks;
      logDebug("Updated adjacency lists: " << vertex.toString());
    }
  }
}

/**
 * Merge replicas before advancing refinement states. This order lets remote
 * vetoes prevent a local erase in the same traversal.
 * This routine handles vertices loaded from the input stream; newly created
 * vertices bypass it and reach updateVertexBeforeStore() directly.
 */
void peano4::grid::Spacetree::updateVertexAfterLoad(
  GridVertex&                               vertex,
  const tarch::la::Vector<DIMENSIONS, int>& vertexIndex
) {
  const bool vertexIsAdjacentToLocalTree = SpacetreeHelper::isVertexAdjacentToLocalSpacetree(
    vertex,
    _id,
    _splitTriggered,
    _splitting,
    _joinTriggered,
    _joining,
    true,
    true,
    false
  );

  if (_spacetreeState != SpacetreeState::NewFromSplit and _spacetreeState != SpacetreeState::EmptyRun) {
    // The horizontal merges below accumulate the neighbour trees' refined-cell counts of
    // the finished sweep into this field, so it starts from zero on every load.
    vertex.numberOfAdjacentRefinedRemoteCells = 0;
    receiveAndMergeGridVertexAtHorizontalBoundary(vertex, vertexIndex, vertexIsAdjacentToLocalTree);
    receiveAndMergeGridVertexAtVerticalBoundary(vertex);

    // Transitional AMR states require conservative reconstruction as hanging
    // because fine-grid data at their refinement boundary may be unstable.
    // EraseTriggered/Erasing are excluded on purpose: a vertex mid-erase still carries
    // refined structure until it reaches Unrefined, so it is not hanging-like.
    const auto vertexState                        = vertex.getState();
    const bool previousCoarseVertexTypeWasHanging = vertexState == GridVertex::State::RefinementTriggered
      or vertexState == GridVertex::State::Delete or vertexState == GridVertex::State::New
      or vertexState == GridVertex::State::Unrefined or vertexState == GridVertex::State::HangingVertex;
    vertex.setFlag(GridVertex::HasBeenVertexTypeHangingInPreviousTreeSweep, previousCoarseVertexTypeWasHanging);

    vertex.setFlag(
      GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep,
      vertex.getFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep)
    );
    vertex.setFlag(
      GridVertex::HasBeenParentOfSubtreeVertexInPreviousTreeSweep,
      vertex.getFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep)
    );

    vertex.clearFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep);
    vertex.clearFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep);

    vertex.numberOfAdjacentRefinedLocalCells = 0;

    if (vertex.getState() == GridVertex::State::RefinementTriggered) {
      if (vertexIsAdjacentToLocalTree) {
        logDebug("Switch vertex to refining on tree " << _id << ": " << vertex.toString());
        vertex.setState(GridVertex::State::Refining);
        _statistics.numberOfStationarySweeps = 0;
      } else {
        vertex.setState(GridVertex::State::Unrefined);
        // Keep the stationary counter untouched here; resetting it could stop
        // the tree from merging into its parent.
      }
    } else if (vertex.getState() == GridVertex::State::EraseTriggered) {
      if (vertex.getFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep)) {
        logDebug(
          "Vertex " << vertex.toString() << " may not be erased on tree " << _id
                    << " as refined structure above or beside it still needs it. Keep the refined state."
        );
        vertex.setState(GridVertex::State::Refined);
        _statistics.setCoarseningHasBeenVetoed(true);
        _statistics.numberOfStationarySweeps = 0;
      } else {
        logDebug("Erase vertex " << vertex.toString() << " outside of domain on tree " << _id);
        vertex.setState(GridVertex::State::Erasing);
        _statistics.numberOfStationarySweeps = 0;
      }
    }
  }

  // A new worker receives a coherent snapshot and can start pruning
  // remote-only replicas inherited from its master during the NewFromSplit
  // traversal. EmptyRun only restores stack order and must not alter states.
  if (_spacetreeState != SpacetreeState::EmptyRun and not vertexIsAdjacentToLocalTree
      and not vertex.getFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep)
      and vertex.getState() == GridVertex::State::Refined) {
    logDebug("Mark " << vertex.toString() << " for erasure in spacetree " << _id);
    vertex.setState(GridVertex::State::EraseTriggered);
  }

  if (_spacetreeState == SpacetreeState::Joining and vertexIsAdjacentToLocalTree) {
    const int masterSlot = getOrCreateNeighbourSlot(_masterId);
    logDebug(
      "Stream vertex " << vertex.toString() << " from tree " << _id << " to master " << _masterId
                       << " through vertical exchange channel"
    );
    _parallelExchangeVertexStacks[masterSlot]
                                 [StackTopology::toIndex(StackTopology::ParallelExchangeStack::VerticalOutput)]
                                   .push(std::move(vertex));
  }
}

bool peano4::grid::Spacetree::restrictToCoarseGrid(
  const tarch::la::Vector<DIMENSIONS, int>& coarseVertexPosition,
  const tarch::la::Vector<DIMENSIONS, int>& fineVertexPosition
) {
  bool result = true;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    assertion(coarseVertexPosition(axis) >= 0, coarseVertexPosition, fineVertexPosition);
    assertion(coarseVertexPosition(axis) <= 1, coarseVertexPosition, fineVertexPosition);

    assertion(fineVertexPosition(axis) >= 0, coarseVertexPosition, fineVertexPosition);
    assertion(fineVertexPosition(axis) <= 3, coarseVertexPosition, fineVertexPosition);

    result = result
      and ((coarseVertexPosition(axis) == 1 and fineVertexPosition(axis) > 0)
           or (coarseVertexPosition(axis) == 0 and fineVertexPosition(axis) < 3));
  }

  return result;
}

/**
 * Restrict the refinement veto to coarse vertices so each tree removes at
 * most one level per sweep. The rolled-over veto keeps replicas at a domain
 * boundary from making conflicting erase decisions.
 *
 * Remote adjacency is cleared only after every descendant is remote because
 * joins still need the intermediate tree IDs. A vertex whose existing
 * adjacent cells are all refined is refined to avoid a hanging vertex.
 *
 * @param fineVertexPositionWithinPatch Position of the vertex within its patch.
 *
 * @see updateVertexAfterLoad()
 * @see mergeCellFromWorkerWithMasterThroughoutJoin()
 */
void peano4::grid::Spacetree::updateVertexBeforeStore(
  GridVertex&                               vertex,
  GridVertex                                coarseGridVertices[TWO_POWER_D],
  const tarch::la::Vector<DIMENSIONS, int>& fineVertexPositionWithinPatch,
  const tarch::la::Vector<DIMENSIONS, int>& vertexIndex
) {
  if (_spacetreeState != SpacetreeState::EmptyRun) {
    if (vertex.getState() == GridVertex::State::New) {
      vertex.setState(GridVertex::State::Unrefined);
    }

    // A vertex surrounded by refined cells refines, so no hanging cluster survives within
    // the domain. The threshold is the number of existing adjacent cells: along the domain
    // boundary the outside slots hold no cell and can never refine, so a wall vertex is
    // already surrounded when every existing cell is refined. A cell dragged into
    // refinement through a vertex it shares with a refining neighbour relies on this rule
    // to refine its remaining wall vertices; without it the wall faces below the cell stay
    // hanging forever and never receive boundary data. The local count is exact and
    // triggers at once. The remote count merged in from neighbouring trees is one sweep
    // old, so it may overcount while cells beyond the tree boundary coarsen; requiring the
    // surrounded state to persist over two sweeps filters every such one-sweep transient
    // without touching single-tree timing.
    int numberOfExistingAdjacentCells = 0;
    for (int i = 0; i < TWO_POWER_D; i++) {
      if (vertex.adjacentRanks[i] != -1) {
        numberOfExistingAdjacentCells++;
      }
    }
    const bool isSurroundedByRefinedCells = _spacetreeState == SpacetreeState::Running
      and vertex.getState() == GridVertex::State::Unrefined and numberOfExistingAdjacentCells > 0
      and vertex.numberOfAdjacentRefinedLocalCells + vertex.numberOfAdjacentRefinedRemoteCells
        == numberOfExistingAdjacentCells;
    if (isSurroundedByRefinedCells
        and (vertex.numberOfAdjacentRefinedLocalCells == numberOfExistingAdjacentCells or vertex.getFlag(GridVertex::HasBeenSurroundedByRefinedCellsInPreviousTreeSweep))) {
      vertex.setState(GridVertex::State::RefinementTriggered);
      _statistics.numberOfStationarySweeps = 0;
      logDebug(
        "Refine vertex " << vertex.toString() << " because all adjacent cells are refined on tree " << _id
                         << " in state " << peano4::grid::toString(_spacetreeState) << "."
      );
    } else if (isSurroundedByRefinedCells) {
      // The two-sweep confirmation is still pending, so the grid must not report
      // itself stationary: otherwise the mesh-refinement drain ends between the
      // two sweeps and the post-refinement fires one time step later than on a
      // single tree.
      _statistics.numberOfStationarySweeps = 0;
    }
    vertex.setFlag(GridVertex::HasBeenSurroundedByRefinedCellsInPreviousTreeSweep, isSurroundedByRefinedCells);

    bool shouldRestrictRefinementVeto = vertex.getFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep);

    if (vertex.getState() == GridVertex::State::RefinementTriggered) {
      shouldRestrictRefinementVeto = true;
    } else if (vertex.getState() == GridVertex::State::Refining) {
      vertex.setState(GridVertex::State::Refined);
      shouldRestrictRefinementVeto = true;
    } else if (vertex.getState() == GridVertex::State::Refined) {
      shouldRestrictRefinementVeto = true;
    } else if (vertex.getState() == GridVertex::State::EraseTriggered) {
      shouldRestrictRefinementVeto = true;
    } else if (vertex.getState() == GridVertex::State::Erasing) {
      vertex.setState(GridVertex::State::Unrefined);
      // A vertex mid-erase still carries refined structure this sweep, so it restricts the
      // antecessor flag like the refined branches above. Without this restriction the
      // coarse level loses the coarsening veto during the multi-sweep erase transition and
      // multi-tree runs coarsen differently from a single tree.
      shouldRestrictRefinementVeto = true;
    }

    sendGridVertex(vertex, vertexIndex);

    if (shouldRestrictRefinementVeto) {
      peano4::dForEachCorner([&](const tarch::la::Vector<DIMENSIONS, int>& coarseVertexPosition, int cornerIndex) {
        if (restrictToCoarseGrid(coarseVertexPosition, fineVertexPositionWithinPatch)) {
          logDebug(
            "Set antecessor flag (veto coarsening) on vertex " << coarseGridVertices[cornerIndex].toString()
                                                               << " due to vertex " << vertex.toString()
          );
          coarseGridVertices[cornerIndex].setFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep, true);
          coarseGridVertices[cornerIndex].setFlag(
            GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep,
            coarseGridVertices[cornerIndex].getFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep)
              or vertex.getFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep)
          );
        }
      });
    }

    if (shouldEraseAdjacencyInformation(vertex, coarseGridVertices, fineVertexPositionWithinPatch)) {
      vertex.adjacentRanks.fill(-1);
    }
  }
}

/**
 * Keep refined and transitional vertices until their erase completes because
 * joins and forks still use their adjacency.
 *
 * @see updateVertexAfterLoad Erases refined vertices outside of local
 *                            domain.
 */
bool peano4::grid::Spacetree::shouldEraseAdjacencyInformation(
  const GridVertex&                  vertex,
  GridVertex                         coarseGridVertices[TWO_POWER_D],
  tarch::la::Vector<DIMENSIONS, int> fineVertexPositionWithinPatch
) const {
  const bool vertexIsAdjacentToLocalTree = SpacetreeHelper::isVertexAdjacentToLocalSpacetree(
    vertex,
    _id,
    _splitTriggered,
    _splitting,
    _joinTriggered,
    _joining,
    true,
    true,
    false
  );
  if (vertexIsAdjacentToLocalTree or vertex.getState() != GridVertex::State::Unrefined) {
    return false;
  }

  auto isOutsideRecursively = [&](const auto& self, tarch::la::Vector<DIMENSIONS, int> position) -> bool {
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      if (position(axis) != 0 and position(axis) != 3) {
        auto lowerBoundaryPosition = position;
        auto upperBoundaryPosition = position;

        lowerBoundaryPosition(axis) = 0;
        upperBoundaryPosition(axis) = 3;

        return self(self, lowerBoundaryPosition) and self(self, upperBoundaryPosition);
      }
    }

    position = position / 3;

    const auto& adjacentRanks = coarseGridVertices[peano4::dLinearised(position, 2)].adjacentRanks;
    return std::find(std::begin(adjacentRanks), std::end(adjacentRanks), _id) == std::end(adjacentRanks);
  };

  return isOutsideRecursively(isOutsideRecursively, fineVertexPositionWithinPatch);
}

std::array<int, TWO_POWER_D> peano4::grid::Spacetree::getAdjacentRanksForNewVertex(
  GridVertex                                coarseGridVertices[TWO_POWER_D],
  const tarch::la::Vector<DIMENSIONS, int>& vertexPositionWithin3x3Patch
) const {
  std::array<int, TWO_POWER_D> adjacentRanks;
  adjacentRanks.fill(-1);

  peano4::dForEachCorner([&](const tarch::la::Vector<DIMENSIONS, int>& corner, int cornerIndex) {
    std::bitset<DIMENSIONS> vertexToInherit;
    std::bitset<DIMENSIONS> indexToInherit;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      if (vertexPositionWithin3x3Patch(axis) <= 1) {
        vertexToInherit.set(axis, false);
        indexToInherit.set(axis, corner(axis) > 0 or vertexPositionWithin3x3Patch(axis) > 0);
      } else {
        vertexToInherit.set(axis, true);
        indexToInherit.set(axis, not(corner(axis) < 1 or vertexPositionWithin3x3Patch(axis) < 3));
      }
    }
    logDebug(
      "Inherit " << indexToInherit.to_ulong() << "th index from coarse vertex " << vertexToInherit
                 << " into local index " << cornerIndex
    );
    adjacentRanks[cornerIndex] =
      coarseGridVertices[vertexToInherit.to_ulong()].adjacentRanks[indexToInherit.to_ulong()];
  });

  return adjacentRanks;
}

/**
 * Load cell vertices in Peano space-filling-curve order. Hanging vertices do
 * not use persistent stacks because their number of adjacent cells is unknown.
 */
void peano4::grid::Spacetree::loadVertices(
  const AutomatonState&                     fineGridState,
  const tarch::la::Vector<DIMENSIONS, int>& cellIndex,
  GridVertex                                coarseGridVertices[TWO_POWER_D],
  GridVertex                                fineGridVertices[TWO_POWER_D],
  const VertexType                          vertexTypes[TWO_POWER_D],
  const tarch::la::Vector<DIMENSIONS, int>& cellPositionWithin3x3Patch
) {
  _vertexLookupTables.push_back(&createVertexLookupTable(fineGridState, cellPositionWithin3x3Patch));
  const auto& lookupTable = *_vertexLookupTables.back();

  int                        cachedStackNumber = -1;
  stacks::Stack<GridVertex>* cachedStack       = nullptr;

  for (int traversalIndex = 0; traversalIndex < TWO_POWER_D; traversalIndex++) {
    const int  vertexIndex          = lookupTable.vIdx[traversalIndex];
    auto&      vertex               = fineGridVertices[vertexIndex];
    const auto vertexPosition       = lookupTable.vPos[traversalIndex];
    VertexType vertexType           = vertexTypes[vertexIndex];
    const int  stackNumber          = lookupTable.readStack[traversalIndex];
    const bool usesInputOutputStack = lookupTable.inOutRead[traversalIndex];

    if (not usesInputOutputStack and vertexType == VertexType::New) {
      vertexType = VertexType::Persistent;
    }

    switch (vertexType) {
    case VertexType::New:
      assertion(usesInputOutputStack);
      vertex = GridVertex::create(
        fineGridState.level,
        GridVertex::State::New,
        fineGridState.x,
        fineGridState.h,
        vertexIndex,
        getAdjacentRanksForNewVertex(coarseGridVertices, vertexPosition),
        true
      );
      break;
    case VertexType::Hanging:
      vertex = GridVertex::create(
        fineGridState.level,
        GridVertex::State::HangingVertex,
        fineGridState.x,
        fineGridState.h,
        vertexIndex,
        getAdjacentRanksForNewVertex(coarseGridVertices, vertexPosition),
        true
      );
      break;
    case VertexType::Persistent: {
      if (stackNumber != cachedStackNumber) {
        assertion(stackNumber >= 0 and stackNumber < StackTopology::TotalNumberOfLocalExchangeStacks, stackNumber, _id);
        cachedStack       = &_localExchangeVertexStacks[stackNumber];
        cachedStackNumber = stackNumber;
      }
      assertion(not cachedStack->empty(), _id, stackNumber, vertexIndex);
      vertex = cachedStack->pop();
      if (usesInputOutputStack) {
        updateVertexAfterLoad(vertex, vertexIndexOfCellVertex(cellIndex, vertexIndex));
      }
    } break;
    case VertexType::Delete: {
      if (stackNumber != cachedStackNumber) {
        assertion(stackNumber >= 0 and stackNumber < StackTopology::TotalNumberOfLocalExchangeStacks, stackNumber, _id);
        cachedStack       = &_localExchangeVertexStacks[stackNumber];
        cachedStackNumber = stackNumber;
      }
      assertion(not cachedStack->empty(), stackNumber, _id);
      vertex = cachedStack->pop();
      if (usesInputOutputStack) {
        updateVertexAfterLoad(vertex, vertexIndexOfCellVertex(cellIndex, vertexIndex));
        vertex.setState(GridVertex::State::Delete);
      }
    } break;
    }

#if PEANO_DEBUG
    const auto expectedPosition = GridVertex::getVertexCoordinate(fineGridState.x, fineGridState.h, vertexIndex);
    assertion(
      tarch::la::equals(vertex.getX(), expectedPosition),
      vertex.getX(),
      expectedPosition,
      vertex.toString(),
      fineGridState.toString(),
      vertexIndex,
      toString(),
      _id
    );
    vertex.setX(expectedPosition);
#endif

#if PEANO_DEBUG
    logDebug(
      "Handled " << peano4::grid::toString(vertexType) << " vertex " << vertexIndex << " at " << vertexPosition << ": "
                 << vertex.toString()
    );
    assertion(
      vertex.level == fineGridState.level,
      vertex.level,
      fineGridState.level,
      vertex.toString(),
      fineGridState.toString(),
      vertexIndex,
      toString(),
      _id
    );
#endif
  }
}

void peano4::grid::Spacetree::storeVertices(
  const tarch::la::Vector<DIMENSIONS, int>& cellIndex,
  GridVertex                                coarseGridVertices[TWO_POWER_D],
  GridVertex                                fineGridVertices[TWO_POWER_D],
  const VertexType                          vertexTypes[TWO_POWER_D],
  const tarch::la::Vector<DIMENSIONS, int>& cellPositionWithin3x3Patch
) {
  assertion(not _vertexLookupTables.empty());
  const auto& lookupTable = *_vertexLookupTables.back();

  int                        cachedStackNumber = -1;
  stacks::Stack<GridVertex>* cachedStack       = nullptr;

  for (int traversalIndex = 0; traversalIndex < TWO_POWER_D; traversalIndex++) {
    const int  vertexIndex          = lookupTable.vIdx[traversalIndex];
    auto&      vertex               = fineGridVertices[vertexIndex];
    VertexType vertexType           = vertexTypes[vertexIndex];
    const int  stackNumber          = lookupTable.writeStack[traversalIndex];
    const bool usesInputOutputStack = lookupTable.inOutWrite[traversalIndex];

    if (not usesInputOutputStack and vertexType == VertexType::Delete) {
      vertexType = VertexType::Persistent;
    }

    if (vertexType == VertexType::Hanging) {
      _statistics.numberOfHangingVertices++;
      continue;
    }
    if (vertexType == VertexType::Delete) {
      assertion(usesInputOutputStack);
      continue;
    }

    if (usesInputOutputStack) {
      updateVertexBeforeStore(
        vertex,
        coarseGridVertices,
        lookupTable.vPos[traversalIndex],
        vertexIndexOfCellVertex(cellIndex, vertexIndex)
      );
    }
    if (stackNumber != cachedStackNumber) {
      assertion(stackNumber >= 0 and stackNumber < StackTopology::TotalNumberOfLocalExchangeStacks, stackNumber, _id);
      cachedStack       = &_localExchangeVertexStacks[stackNumber];
      cachedStackNumber = stackNumber;
    }
    cachedStack->push(std::move(vertex));
  }

  _vertexLookupTables.pop_back();
}

bool peano4::grid::Spacetree::isFaceAlongPeriodicBoundaryCondition(
  GridVertex fineGridVertices[TWO_POWER_D],
  int        faceNumber,
  bool       calledByReceivingProcess
) const {
  int adjacentRanksOfFace[TWO_POWER_D];
  SpacetreeHelper::getAdjacentRanksOfFace(fineGridVertices, faceNumber, calledByReceivingProcess, adjacentRanksOfFace);

  bool isAdjacentToLocalTree              = false;
  bool holdsPeriodicBoundaryConditionFlag = false;
  for (int adjacentRankIndex = 0; adjacentRankIndex < TWO_POWER_D; adjacentRankIndex++) {
    isAdjacentToLocalTree = isAdjacentToLocalTree or adjacentRanksOfFace[adjacentRankIndex] == _id;
    holdsPeriodicBoundaryConditionFlag =
      holdsPeriodicBoundaryConditionFlag or adjacentRanksOfFace[adjacentRankIndex] == RankOfPeriodicBoundaryCondition;
  }

  holdsPeriodicBoundaryConditionFlag = holdsPeriodicBoundaryConditionFlag and isAdjacentToLocalTree;

  return holdsPeriodicBoundaryConditionFlag;
}

void peano4::grid::Spacetree::mergeGridVertexAdjacencyListsAtHorizontalDomainBoundary(
  GridVertex&       vertex,
  const GridVertex& incomingVertex,
  int               neighbourTreeId
) {
  for (int adjacencyIndex = 0; adjacencyIndex < TWO_POWER_D; adjacencyIndex++) {
    // The ownership decision must be based on the pre-merge adjacency list.
    // Otherwise, earlier merges within the same receive can change
    // vertex.adjacentRanks and thereby change ownership for later neighbours.
    if (vertex.backupOfAdjacentRanks[adjacencyIndex] == neighbourTreeId and neighbourTreeId != -1) {
      if (incomingVertex.adjacentRanks[adjacencyIndex] != vertex.adjacentRanks[adjacencyIndex]) {
        logDebug(
          "Update " << adjacencyIndex << "th entry of adjacency data of vertex " << vertex.toString()
                    << " with neighbouring vertex " << incomingVertex.toString() << " on tree " << _id
        );
      }
      vertex.adjacentRanks[adjacencyIndex] = incomingVertex.adjacentRanks[adjacencyIndex];
    }
  }
}

/**
 * Merge refinement flags without allowing either replica to erase data that
 * the other replica still refines.
 */
void peano4::grid::Spacetree::mergeGridVertexRefinementStateAtHorizontalDomainBoundary(
  GridVertex&       vertex,
  const GridVertex& incomingVertex,
  int               neighbourTreeId
) {
  assertion(
    vertex.getState() != GridVertex::State::HangingVertex,
    incomingVertex.toString(),
    vertex.toString(),
    _id,
    neighbourTreeId
  );
  assertion(
    incomingVertex.getState() != GridVertex::State::HangingVertex,
    incomingVertex.toString(),
    vertex.toString(),
    _id,
    neighbourTreeId
  );
#if PEANO_DEBUG
  assertion(
    tarch::la::equals(vertex.getX(), incomingVertex.getX())
      or (std::find(
            std::begin(vertex.adjacentRanks),
            std::end(vertex.adjacentRanks),
            RankOfPeriodicBoundaryCondition
          ) != std::end(vertex.adjacentRanks)
          and (_periodicBC.count() > 1
               or tarch::la::countEqualEntries(vertex.getX(), incomingVertex.getX())
                    == DIMENSIONS - static_cast<int>(_periodicBC.count()))),
    incomingVertex.toString(),
    vertex.toString(),
    _id,
    neighbourTreeId,
    tarch::la::countEqualEntries(vertex.getX(), incomingVertex.getX()),
    _periodicBC
  );
#endif
  assertion(
    vertex.level == incomingVertex.level,
    vertex.level,
    incomingVertex.level,
    incomingVertex.toString(),
    vertex.toString(),
    _id,
    neighbourTreeId
  );
  assertion(
    not(vertex.getState() == GridVertex::State::Refined and incomingVertex.getState() == GridVertex::State::Erasing),
    incomingVertex.toString(),
    vertex.toString(),
    _id,
    neighbourTreeId
  );
  assertion(
    not(vertex.getState() == GridVertex::State::Refined and incomingVertex.getState() == GridVertex::State::Unrefined),
    incomingVertex.toString(),
    vertex.toString(),
    _id,
    neighbourTreeId
  );

  vertex.setFlag(
    GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep,
    vertex.getFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep)
      or incomingVertex.getFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep)
  );

  // Every adjacent refined cell is local to exactly one tree, so summing the neighbour
  // copies' local counts yields the global count of refined cells around this vertex.
  // The surrounded-vertex refinement in updateVertexBeforeStore() reads this sum; with
  // the local count alone a vertex on a tree boundary could never see all existing adjacent cells.
  // Splits and joins move cells between trees mid-sweep, which would double-count, so
  // the accumulation pauses while any of them is in flight.
  if (_splitTriggered.empty() and _splitting.empty() and _joinTriggered.empty() and _joining.empty()) {
    vertex.numberOfAdjacentRefinedRemoteCells += incomingVertex.numberOfAdjacentRefinedLocalCells;
  }

  // The subtree-parent flag has to be merged like the antecessor flag: a copy of this
  // vertex on a neighbouring tree may sit above a forked subtree the local tree cannot
  // see, and dropping that veto would let the local tree coarsen over the worker's domain.
  vertex.setFlag(
    GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep,
    vertex.getFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep)
      or incomingVertex.getFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep)
  );

  // The copies compute the surrounded state with counts offset by one sweep, so one side
  // may see it a sweep earlier; merging keeps the two-sweep confirmation in step.
  vertex.setFlag(
    GridVertex::HasBeenSurroundedByRefinedCellsInPreviousTreeSweep,
    vertex.getFlag(GridVertex::HasBeenSurroundedByRefinedCellsInPreviousTreeSweep)
      or incomingVertex.getFlag(GridVertex::HasBeenSurroundedByRefinedCellsInPreviousTreeSweep)
  );

  assertion(vertex.getState() != GridVertex::State::Erasing, vertex.toString(), incomingVertex.toString(), _id);
  assertion(vertex.getState() != GridVertex::State::Refining, vertex.toString(), incomingVertex.toString(), _id);

  const bool onlyNeighbourHasTriggeredRefinement = incomingVertex.getState() == GridVertex::State::RefinementTriggered
    and (vertex.getState() == GridVertex::State::Unrefined or vertex.getState() == GridVertex::State::New
         or vertex.getState() == GridVertex::State::Delete);
  const bool onlyLocalHasTriggeredRefinement = vertex.getState() == GridVertex::State::RefinementTriggered
    and (incomingVertex.getState() == GridVertex::State::Unrefined
         or incomingVertex.getState() == GridVertex::State::New
         or incomingVertex.getState() == GridVertex::State::Delete);
  const bool onlyNeighbourHasTriggeredErase =
    incomingVertex.getState() == GridVertex::State::EraseTriggered and vertex.getState() == GridVertex::State::Refined;
  const bool onlyLocalHasTriggeredErase =
    vertex.getState() == GridVertex::State::EraseTriggered and incomingVertex.getState() == GridVertex::State::Refined;

  if (onlyNeighbourHasTriggeredRefinement or onlyLocalHasTriggeredRefinement) {
    vertex.setState(GridVertex::State::RefinementTriggered);
    _statistics.numberOfStationarySweeps = 0;
    logDebug("Set state to " << vertex.toString() << " on tree " << _id << " due to merge with neighbour");
  } else if (onlyNeighbourHasTriggeredErase or onlyLocalHasTriggeredErase) {
    vertex.setState(GridVertex::State::EraseTriggered);
    _statistics.numberOfStationarySweeps = 0;
    logDebug("Set state to " << vertex.toString() << " on tree " << _id << " due to merge with neighbour");
  } else {
    assertion(
      vertex.getState() == incomingVertex.getState(),
      incomingVertex.toString(),
      vertex.toString(),
      _id,
      neighbourTreeId
    );
  }
}

/**
 * Merge boundary replicas before refinement-state transitions.
 *
 * The backup preserves routing from the start of the traversal while live
 * adjacency is merged. Periodic replicas use identity stamps because one
 * wrapped vertex can be held by several trees. Horizontal data is merged
 * before joining-worker data so the latter can update outgoing adjacency.
 */
void peano4::grid::Spacetree::receiveAndMergeGridVertexAtHorizontalBoundary(
  GridVertex&                               vertex,
  const tarch::la::Vector<DIMENSIONS, int>& vertexIndex,
  bool                                      vertexIsAdjacentToLocalTree
) {
  assertion(_spacetreeState != SpacetreeState::NewFromSplit and _spacetreeState != SpacetreeState::EmptyRun);

  vertex.backupOfAdjacentRanks = vertex.adjacentRanks;

  const auto receivingNeighbourTrees = SpacetreeHelper::getNeighbourTreesAlongVertex(
    vertex,
    _id,
    _splitTriggered,
    _splitting,
    _joining,
    true,
    _spacetreeState,
    _noParallelTopologyChanges
  );
  for (int neighbourIndex = 0; neighbourIndex < receivingNeighbourTrees.count; ++neighbourIndex) {
    const int neighbourTreeId = receivingNeighbourTrees.ids[neighbourIndex];
    assertion(neighbourTreeId >= 0, neighbourTreeId);
    const int neighbourSlot = getNeighbourSlot(neighbourTreeId);
    assertion(neighbourSlot >= 0, _id, neighbourTreeId, vertex.toString(), toString());
    auto& inputStack =
      _parallelExchangeVertexStacks[neighbourSlot]
                                   [StackTopology::toIndex(StackTopology::ParallelExchangeStack::HorizontalInput)];
    assertion(not inputStack.empty(), _id, neighbourTreeId, vertex.toString(), inputStack.toString());
    GridVertex incomingVertex = inputStack.pop();
    mergeGridVertexRefinementStateAtHorizontalDomainBoundary(vertex, incomingVertex, neighbourTreeId);
    mergeGridVertexAdjacencyListsAtHorizontalDomainBoundary(vertex, incomingVertex, neighbourTreeId);
  }

  if (vertexIsAdjacentToLocalTree and hasPeriodicBoundaryConditions(vertex.adjacentRanks)) {
    const auto outputStacks = StackTopology::getOutputStacksForPeriodicBoundaryExchange(vertex.adjacentRanks);
    for (const auto& outputStack : outputStacks) {
      assertion(
        StackTopology::isPeriodicBoundaryExchangeOutputStackNumber(outputStack.first),
        _id,
        outputStack.first,
        outputStack.second,
        vertex.toString()
      );
      const auto stamp = makeVertexStamp(vertex.level, vertexIndex, std::bitset<DIMENSIONS>(outputStack.second));
      _periodicConsumerDescriptors.push_back({stamp, _id});

      const int inputStackNumber =
        StackTopology::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(outputStack.first);
      assertion(
        inputStackNumber >= 0 and inputStackNumber < StackTopology::TotalNumberOfLocalExchangeStacks,
        inputStackNumber,
        outputStack.first,
        StackTopology::TotalNumberOfLocalExchangeStacks,
        _id
      );
      // One copy arrives per tree holding the wrapped image; merge them all,
      // like the horizontal merge handles one copy per neighbour tree. On the
      // first sweep after a regrid the partner may not have produced yet, so
      // finding nothing is legitimate then.
      while (auto incomingVertex = popPeriodicEntryMatching(
               _localExchangeVertexStacks[inputStackNumber],
               _localExchangeVertexStackStamps[inputStackNumber],
               stamp
             )) {
        logDebug("Merge periodic image " << stamp.toString() << " into " << vertex.toString() << " on tree " << _id);
        mergeGridVertexRefinementStateAtHorizontalDomainBoundary(vertex, *incomingVertex, _id);
      }
    }
  }
}

/**
 * Send each vertex once per sweep. The call must coincide with storage on an
 * input/output stack because hanging vertices have no persistent stack entry.
 * The neighbour set is unique, so no tree receives duplicate copies.
 *
 * A local vertex along a periodic boundary additionally pushes one stamped
 * copy per wrapped-axis subset onto the periodic output stacks, so the
 * exchange can route it to whichever trees hold the wrapped images.
 */
void peano4::grid::Spacetree::sendGridVertex(
  const GridVertex&                         vertex,
  const tarch::la::Vector<DIMENSIONS, int>& vertexIndex
) {
  assertion(_spacetreeState != SpacetreeState::EmptyRun, _id, peano4::grid::toString(_spacetreeState));

  const auto outgoingNeighbourTrees = SpacetreeHelper::getNeighbourTreesAlongVertex(
    vertex,
    _id,
    _splitTriggered,
    _splitting,
    _joining,
    false,
    _spacetreeState,
    _noParallelTopologyChanges
  );

  for (int neighbourIndex = 0; neighbourIndex < outgoingNeighbourTrees.count; ++neighbourIndex) {
    const int neighbourTreeId = outgoingNeighbourTrees.ids[neighbourIndex];
    const int neighbourSlot   = getOrCreateNeighbourSlot(neighbourTreeId);

    GridVertex vertexCopy = vertex;
    for (int adjacencyIndex = 0; adjacencyIndex < TWO_POWER_D; adjacencyIndex++) {
      const int targetTreeId = vertexCopy.adjacentRanks[adjacencyIndex];
      if (_spacetreeState == SpacetreeState::JoinTriggered and targetTreeId == _id and neighbourTreeId != _masterId) {
        vertexCopy.adjacentRanks[adjacencyIndex] = _masterId;
      }
    }

    _parallelExchangeVertexStacks[neighbourSlot]
                                 [StackTopology::toIndex(StackTopology::ParallelExchangeStack::HorizontalOutput)]
                                   .push(std::move(vertexCopy));
  }

  // The marker test comes first, so runs without periodic boundary conditions
  // skip the locality evaluation altogether.
  if (hasPeriodicBoundaryConditions(vertex.adjacentRanks)
      and SpacetreeHelper::isVertexAdjacentToLocalSpacetree(
        vertex,
        _id,
        _splitTriggered,
        _splitting,
        _joinTriggered,
        _joining,
        true,
        true,
        false
      )) {
    const auto outputStacks = StackTopology::getOutputStacksForPeriodicBoundaryExchange(vertex.adjacentRanks);
    for (const auto& outputStack : outputStacks) {
      assertion(
        outputStack.first >= 0 and outputStack.first < StackTopology::TotalNumberOfLocalExchangeStacks,
        outputStack.first,
        _id
      );
      const auto stamp = makeVertexStamp(vertex.level, vertexIndex, std::bitset<DIMENSIONS>(outputStack.second));
      _periodicProducerDescriptors.push_back({stamp, _id});
      _localExchangeVertexStacks[outputStack.first].push(vertex);
      _localExchangeVertexStackStamps[outputStack.first].push_back(stamp);
      logDebug(
        "Vertex " << vertex.toString() << " on tree " << _id << " goes to stack " << outputStack.first
                  << " as periodic image " << stamp.toString()
      );
    }
  }
}

/**
 * Count this refined cell on each of its vertices.
 *
 * updateVertexBeforeStore() compares the total against the number of adjacent cells that
 * exist, so a vertex refines once all of them are refined and no hanging cluster survives,
 * inside the domain and along its walls alike. The counter holds the local tree's share;
 * neighbouring trees contribute theirs through the horizontal vertex merge.
 */
void peano4::grid::Spacetree::incrementNumberOfAdjacentRefinedLocalCells(GridVertex vertices[TWO_POWER_D]) {
  peano4::dForEachCorner([&](const tarch::la::Vector<DIMENSIONS, int>&, int cornerIndex) {
    vertices[cornerIndex].numberOfAdjacentRefinedLocalCells += 1;
  });
}

/**
 * Update the ownership entries of a local cell whose parent is local. Joining
 * workers retain their adjacency because it identifies the local tree during
 * the merge.
 */
void peano4::grid::Spacetree::updateVertexRanksWithinCell(GridVertex fineGridVertices[TWO_POWER_D], int newTreeId) {
  peano4::dForEachCorner([&](const tarch::la::Vector<DIMENSIONS, int>&, int cornerIndex) {
    fineGridVertices[cornerIndex].adjacentRanks[TWO_POWER_D - cornerIndex - 1] = newTreeId;
  });
}

void peano4::grid::Spacetree::evaluateGridControlEvents(
  const AutomatonState& fineGridState,
  GridVertex            coarseGridVertices[TWO_POWER_D],
  GridVertex            fineGridVertices[TWO_POWER_D]
) {
  if (_gridControlEvents.empty()) {
    return;
  }
  assertion(
    _spacetreeState != SpacetreeState::EmptyRun and _spacetreeState != SpacetreeState::NewFromSplit
      and _spacetreeState != SpacetreeState::Joining,
    toString()
  );

  const auto stateMin = fineGridState.x;
  const auto stateMax = fineGridState.x + fineGridState.h;

  bool wantsRefine = false;
  bool wantsErase  = false;

  for (const auto& event : _gridControlEvents) {
    const auto eventMin = event.offset;
    const auto eventMax = event.offset + event.width;

    if (event.getRefinementControl() == GridControlEvent::RefinementControl::Refine) {
      // Determine if the event overlaps the state with positive volume
      //
      // event:  [eventMin --------------------- eventMax]
      // state:       [stateMin ---- stateMax]
      //               ^ overlap because the interiors intersect
      //
      // The overlap is strict (open): a cell that only touches a refine event's boundary
      // (stateMax == eventMin, say) lies outside the event and is not refined. A closed
      // test would refine one extra cell layer around every refine region. The erase
      // containment test below deliberately stays closed.
      //
      // These comparisons inherit tarch::la's default NUMERICAL_ZERO_DIFFERENCE, an
      // absolute 1e-8 dead band. Event boxes and cell bounds are exact multiples of the
      // mesh size, so the two sides either coincide or differ by a whole cell width, and
      // the band decides nothing until h itself approaches 1e-8.
      const bool overlaps = tarch::la::allGreater(stateMax, eventMin) and tarch::la::allSmaller(stateMin, eventMax);

      const bool refineToTargetResolution = tarch::la::allSmaller(event.targetH, fineGridState.h);

      if (refineToTargetResolution and overlaps) {
        // Refinement takes precedence when refinement and erasure regions overlap.
        wantsRefine = true;
        break;
      }
    } else if (event.getRefinementControl() == GridControlEvent::RefinementControl::Erase) {
      const bool contains =
        tarch::la::allGreaterEquals(stateMin, eventMin) and tarch::la::allSmallerEquals(stateMax, eventMax);

      const bool coarsenToTargetResolution = tarch::la::allGreaterEquals(event.targetH, fineGridState.h)
        // Erase events are computed on leaves, but have to be applied on the
        // direct parents of these leaves. Therefore, we coarsen only if the
        // current cell's size equals the target resolution, i.e. the cell is not
        // finer than one refinement level below the event's target resolution.
        and not tarch::la::allSmallerEquals(3.0 * fineGridState.h, event.targetH);

      if (coarsenToTargetResolution and contains) {
        wantsErase = true;
      }
    }
  }

  // A refined cell that no event covers keeps its vertices. Its marking below is what makes
  // an erase a consensus decision: a vertex only coarsens once every refined cell around it
  // asks for it, so an erase region never takes cells it does not cover through shared
  // vertices, whichever order the trees traverse them in.
  const bool wantsKeep = _gridControlEventsContainErase and not wantsRefine and not wantsErase
    and SpacetreeHelper::isSpacetreeNodeRefined(fineGridVertices);

  if (not wantsRefine and not wantsErase and not wantsKeep) {
    return;
  }

  std::array<bool, TWO_POWER_D> isLocalFineVertex{};
  UNROLL(DIMENSIONS)
  for (int vertexIndex = 0; vertexIndex < TWO_POWER_D; ++vertexIndex) {
    isLocalFineVertex[vertexIndex] = SpacetreeHelper::isVertexAdjacentToLocalSpacetree(
      fineGridVertices[vertexIndex],
      _id,
      _splitTriggered,
      _splitting,
      _joinTriggered,
      _joining,
      true,
      true,
      false
    );
  }

  const auto isTopologyStable = [](GridVertex::State vertexState) {
    switch (vertexState) {
    case GridVertex::State::HangingVertex:
    case GridVertex::State::Unrefined:
    case GridVertex::State::Refined:
    case GridVertex::State::EraseTriggered:
    case GridVertex::State::RefinementTriggered:
    case GridVertex::State::Refining:
      return true;
    default:
      return false;
    }
  };

  // Transitional coarse vertices cannot provide a stable topology for refinement.
  bool mayChangeGrid = true;
  UNROLL(DIMENSIONS)
  for (int vertexIndex = 0; vertexIndex < TWO_POWER_D; vertexIndex++) {
    if (not isTopologyStable(coarseGridVertices[vertexIndex].getState())) {
      mayChangeGrid = false;
      break;
    }
  }

  if (wantsRefine) {
    UNROLL(DIMENSIONS)
    for (int vertexIndex = 0; vertexIndex < TWO_POWER_D; ++vertexIndex) {
      auto& vertex = fineGridVertices[vertexIndex];
      if (not isLocalFineVertex[vertexIndex]) {
        continue;
      }

      vertex.setFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep, true);

      const auto vertexState = vertex.getState();
      if (vertexState == GridVertex::State::EraseTriggered) {
        // Cancel pending erase if an adjacent cell wants to keep/refine.
        vertex.setState(GridVertex::State::Refined);
      } else if (vertexState == GridVertex::State::Unrefined) {
        if (mayChangeGrid) {
          vertex.setState(GridVertex::State::RefinementTriggered);
        } else {
          // The requested refinement is deferred while neighbouring transitions are in
          // flight; the count tells the event source to deliver the request again.
          _statistics.numberOfDeferredGridControlChanges++;
        }
        _statistics.numberOfStationarySweeps = 0;
      }
    }
  } else if (wantsErase) {
    UNROLL(DIMENSIONS)
    for (int vertexIndex = 0; vertexIndex < TWO_POWER_D; ++vertexIndex) {
      auto& vertex = fineGridVertices[vertexIndex];
      if (not isLocalFineVertex[vertexIndex]) {
        continue;
      }

      if (vertex.getState() == GridVertex::State::Refined
          and not vertex.getFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep)) {
        if (mayChangeGrid) {
          vertex.setState(GridVertex::State::EraseTriggered);
        } else {
          // The requested erase is deferred while neighbouring transitions are in flight;
          // the count tells the event source to deliver the request again.
          _statistics.numberOfDeferredGridControlChanges++;
        }
        _statistics.numberOfStationarySweeps = 0;
      }
    }
  } else {
    UNROLL(DIMENSIONS)
    for (int i = 0; i < TWO_POWER_D; ++i) {
      auto& v = fineGridVertices[i];
      if (not isLocalFineVertex[i] or v.getState() == GridVertex::State::HangingVertex) {
        continue;
      }

      // The flag rolls over at the next load, where it unrolls an erase a neighbouring cell
      // triggered on a shared vertex.
      v.setFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep, true);
    }
  }
}

/**
 * Traverse a refined cell. Fork boundaries veto coarsening on both trees so
 * the bottom-up flag propagation preserves the worker domain.
 */
void peano4::grid::Spacetree::descend(
  const AutomatonState&                     state,
  const tarch::la::Vector<DIMENSIONS, int>& cellIndex,
  GridVertex                                vertices[TWO_POWER_D],
  GridTraversalObserver&                    observer
) {
  assertion(SpacetreeHelper::isSpacetreeNodeRefined(vertices));
#if PEANO_DEBUG
  // The exact integer index carried through the recursion must agree with the
  // index derived from the automaton's floating-point box.
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    assertion(cellIndexFromState(state, _root.x)(axis) == cellIndex(axis), axis, cellIndex, state.toString(), _id);
  }
#endif

  const bool hasJoiningTransitions   = not _joining.empty();
  const bool hasGridControlEvents    = not _gridControlEvents.empty();
  const bool hasTriggeredSplits      = not _splitTriggered.empty();
  const bool spacetreeStateIsRunning = _spacetreeState == SpacetreeState::Running;
  const bool canReceiveUserData      = _spacetreeState != SpacetreeState::EmptyRun
    and _spacetreeState != SpacetreeState::NewFromSplit and _spacetreeState != SpacetreeState::Joined;
  const bool canSendUserData =
    _spacetreeState != SpacetreeState::EmptyRun and _spacetreeState != SpacetreeState::Joined;

  const bool isCoarseLocal = SpacetreeHelper::isSpacetreeNodeLocal(
    vertices,
    _id,
    _splitTriggered,
    _splitting,
    _joinTriggered,
    _joining,
    false,
    false,
    false
  );

  AutomatonState fineGridStates[THREE_POWER_D];
  AutomatonState::refineState(state, fineGridStates);

  const auto loopDirection = PeanoCurve::getLoopDirection(state);
  peano4::zForEach3(loopDirection, [&](const tarch::la::Vector<DIMENSIONS, int>& cellPositionWithinPatch) {
    traceEnter("Grid bookkeeping before enter");
    const auto cellPositionLinearised = peano4::dLinearised(cellPositionWithinPatch, 3);
    const auto fineGridCellIndex      = childCellIndex(cellIndex, cellPositionWithinPatch);

    VertexType vertexTypes[TWO_POWER_D];
    getVertexTypes(vertices, cellPositionWithinPatch, vertexTypes);

    GridVertex fineGridVertices[TWO_POWER_D];
    loadVertices(
      fineGridStates[cellPositionLinearised],
      fineGridCellIndex,
      vertices,
      fineGridVertices,
      vertexTypes,
      cellPositionWithinPatch
    );

    const auto& lookupTable = *_vertexLookupTables.back();

    FaceType faceTypes[TWO_TIMES_D];
    for (int faceIndex = 0; faceIndex < TWO_TIMES_D; ++faceIndex) {
      faceTypes[faceIndex] = getFaceType(vertexTypes, lookupTable.faceIndex[faceIndex]);
    }

    if (hasJoiningTransitions) {
      mergeCellFromWorkerWithMasterThroughoutJoin(vertices, fineGridVertices);
    }

    const bool isRefined   = SpacetreeHelper::isSpacetreeNodeRefined(fineGridVertices);
    const bool isFineLocal = SpacetreeHelper::isSpacetreeNodeLocal(
      fineGridVertices,
      _id,
      _splitTriggered,
      _splitting,
      _joinTriggered,
      _joining,
      false,
      false,
      false
    );

    if (isFineLocal != isCoarseLocal) {
      markVerticesAroundForkedCell(vertices, fineGridVertices);
    }

    if (hasGridControlEvents and isFineLocal) {
      evaluateGridControlEvents(fineGridStates[cellPositionLinearised], vertices, fineGridVertices);
    }

    if (hasTriggeredSplits) {
      splitCellTopDown(vertices, fineGridVertices);
    }

    auto enterNodeGridTraversalEvent = _gridTraversalEventGenerator.createEnterNodeTraversalEvent(
      vertices,
      fineGridVertices,
      vertexTypes,
      faceTypes,
      fineGridStates[cellPositionLinearised],
      _splitTriggered,
      _splitting,
      _joinTriggered,
      _joining,
      _hasSplit,
      cellPositionWithinPatch,
      lookupTable,
      _noParallelTopologyChanges,
      spacetreeStateIsRunning
    );
    traceLeave("Grid bookkeeping before enter");

    observer.loadNodeData(enterNodeGridTraversalEvent);

    if (canReceiveUserData) {
      traceEnter("Observer data receive");
      receiveAndMergeUserData(
        fineGridStates[cellPositionLinearised],
        fineGridCellIndex,
        observer,
        enterNodeGridTraversalEvent,
        fineGridVertices
      );
      traceLeave("Observer data receive");
    }

    traceEnter("Observer enter node");
    observer.enterNode(enterNodeGridTraversalEvent);
    traceLeave("Observer enter node");

    _statistics.minH = tarch::la::min(_statistics.minH, 1.0 / 3.0 * state.h);

    assertion(isRefined == SpacetreeHelper::isSpacetreeNodeRefined(fineGridVertices));
    if (isRefined) {
      if (isFineLocal) {
        _statistics.numberOfLocalRefinedCells++;
        incrementNumberOfAdjacentRefinedLocalCells(fineGridVertices);
      } else {
        _statistics.numberOfRemoteRefinedCells++;
      }
      descend(fineGridStates[cellPositionLinearised], fineGridCellIndex, fineGridVertices, observer);
    } else {
      bool isFineLocalWithTransitionTrees = isFineLocal;
      if (not isFineLocal and not _noParallelTopologyChanges) {
        isFineLocalWithTransitionTrees = SpacetreeHelper::isSpacetreeNodeLocal(
          fineGridVertices,
          _id,
          _splitTriggered,
          _splitting,
          _joinTriggered,
          _joining,
          true,
          true,
          false
        );
      }
      if (isFineLocalWithTransitionTrees) {
        _statistics.numberOfLocalUnrefinedCells++;
        const auto currentLevel = fineGridStates[cellPositionLinearised].level;
        _statistics.minDepth    = std::min(_statistics.minDepth, currentLevel);
        _statistics.maxDepth    = std::max(_statistics.maxDepth, currentLevel);
      } else {
        _statistics.numberOfRemoteUnrefinedCells++;
      }
    }

    traceEnter("Grid bookkeeping after child");
    auto leaveNodeGridTraversalEvent = _gridTraversalEventGenerator.createLeaveNodeTraversalEvent(
      enterNodeGridTraversalEvent,
      vertices,
      fineGridVertices,
      vertexTypes,
      faceTypes,
      fineGridStates[cellPositionLinearised],
      _splitTriggered,
      _splitting,
      _joinTriggered,
      _joining,
      _hasSplit,
      cellPositionWithinPatch,
      lookupTable,
      _noParallelTopologyChanges,
      spacetreeStateIsRunning
    );
    traceLeave("Grid bookkeeping after child");

    traceEnter("Observer leave node");
    observer.leaveNode(leaveNodeGridTraversalEvent);
    traceLeave("Observer leave node");

    if (canSendUserData) {
      traceEnter("Observer data send");
      sendUserData(
        fineGridStates[cellPositionLinearised],
        fineGridCellIndex,
        observer,
        leaveNodeGridTraversalEvent,
        fineGridVertices
      );
      traceLeave("Observer data send");
    }

    observer.storeNodeData(leaveNodeGridTraversalEvent);

    traceEnter("Grid bookkeeping after leave");
    if (not _noParallelTopologyChanges) {
      splitOrJoinCellBottomUp(vertices, fineGridVertices);
    }

    storeVertices(fineGridCellIndex, vertices, fineGridVertices, vertexTypes, cellPositionWithinPatch);
    traceLeave("Grid bookkeeping after leave");
  });
}

void peano4::grid::Spacetree::receiveAndMergeUserData(
  const AutomatonState&                     state,
  const tarch::la::Vector<DIMENSIONS, int>& cellIndex,
  GridTraversalObserver&                    observer,
  const GridTraversalEvent&                 enterCellTraversalEvent,
  GridVertex                                fineGridVertices[TWO_POWER_D]
) {
  assertion(
    _spacetreeState != SpacetreeState::EmptyRun and _spacetreeState != SpacetreeState::NewFromSplit
      and _spacetreeState != SpacetreeState::Joined,
    state.toString(),
    peano4::grid::toString(_spacetreeState),
    _id
  );

  for (int vertexEntry = 0; vertexEntry < TWO_POWER_D; vertexEntry++) {
    const int inputVertexStack              = enterCellTraversalEvent.getVertexDataFrom(vertexEntry);
    const int inputVertexPositionWithinCell = enterCellTraversalEvent.getVertexDataTo(vertexEntry);

    const bool isHangingVertex = enterCellTraversalEvent.getIsVertexHanging(inputVertexPositionWithinCell);
    if (StackTopology::isInOutStack(inputVertexStack) and not isHangingVertex) {
      if (enterCellTraversalEvent.getIsVertexLocal(inputVertexPositionWithinCell)) {
        const auto receivingNeighbourTrees = SpacetreeHelper::getNeighbourTreesAlongVertex(
          fineGridVertices[inputVertexPositionWithinCell],
          _id,
          _splitTriggered,
          _splitting,
          _joining,
          true,
          _spacetreeState,
          _noParallelTopologyChanges
        );
        for (int neighbourIndex = 0; neighbourIndex < receivingNeighbourTrees.count; ++neighbourIndex) {
          const int neighbourTreeId = receivingNeighbourTrees.ids[neighbourIndex];
          logDebug(
            "Receive neighbour data from tree " << neighbourTreeId << " for vertex "
                                                << fineGridVertices[inputVertexPositionWithinCell].toString()
          );
          const int neighbourSlot = getOrCreateNeighbourSlot(neighbourTreeId);
          assertion(neighbourSlot >= 0, _id, neighbourTreeId, inputVertexPositionWithinCell, toString());
          observer.receiveNodeData(
            GridTraversalObserver::NodeData::Vertex,
            inputVertexPositionWithinCell,
            neighbourSlot,
            stacks::SendReceiveContext::BoundaryExchange,
            enterCellTraversalEvent
          );
        }

        if (hasPeriodicBoundaryConditions(fineGridVertices[inputVertexPositionWithinCell].adjacentRanks)) {
          const auto outputStacks = StackTopology::getOutputStacksForPeriodicBoundaryExchange(
            fineGridVertices[inputVertexPositionWithinCell].adjacentRanks
          );
          const auto vertexIndex = vertexIndexOfCellVertex(cellIndex, inputVertexPositionWithinCell);
          for (const auto& outputStack : outputStacks) {
            const auto stamp = makeVertexStamp(state.level, vertexIndex, std::bitset<DIMENSIONS>(outputStack.second));
            const auto inputStackNumber =
              StackTopology::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(outputStack.first);
            observer.receiveNodeData(
              GridTraversalObserver::NodeData::Vertex,
              inputVertexPositionWithinCell,
              inputStackNumber,
              stacks::SendReceiveContext::PeriodicBoundaryDataSwap,
              enterCellTraversalEvent,
              &stamp
            );
          }
        }
      }
    }
  }

  for (int faceEntry = 0; faceEntry < TWO_TIMES_D; faceEntry++) {
    const int inputFaceStack              = enterCellTraversalEvent.getFaceDataFrom(faceEntry);
    const int inputFacePositionWithinCell = enterCellTraversalEvent.getFaceDataTo(faceEntry);

    const bool isHangingFace = enterCellTraversalEvent.getIsFaceHanging(inputFacePositionWithinCell);
    if (StackTopology::isInOutStack(inputFaceStack) and not isHangingFace) {
      if (enterCellTraversalEvent.getIsFaceLocal(inputFacePositionWithinCell)) {
        const int receivingNeighbourTreeId = SpacetreeHelper::getNeighbourTreeAlongFace(
          fineGridVertices,
          inputFacePositionWithinCell,
          _id,
          _splitTriggered,
          _splitting,
          _joining,
          true,
          _spacetreeState,
          _noParallelTopologyChanges
        );
        if (receivingNeighbourTreeId >= 0) {
          const int neighbourSlot = getOrCreateNeighbourSlot(receivingNeighbourTreeId);
          assertion(neighbourSlot >= 0, _id, receivingNeighbourTreeId, inputFacePositionWithinCell, toString());
          observer.receiveNodeData(
            GridTraversalObserver::NodeData::Face,
            inputFacePositionWithinCell,
            neighbourSlot,
            stacks::SendReceiveContext::BoundaryExchange,
            enterCellTraversalEvent
          );
        }

        if (isFaceAlongPeriodicBoundaryCondition(fineGridVertices, inputFacePositionWithinCell, true)) {
          const auto stamp = makeFaceStamp(state.level, cellIndex, inputFacePositionWithinCell);
          _periodicConsumerDescriptors.push_back({stamp, _id});
          const int outputStackNumber =
            StackTopology::getOutputStackForPeriodicBoundaryExchange(inputFacePositionWithinCell);
          const int inputStackNumber =
            StackTopology::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(outputStackNumber);
          logDebug("Merge face " << stamp.toString() << " from periodic boundary stack " << inputStackNumber);

          observer.receiveNodeData(
            GridTraversalObserver::NodeData::Face,
            inputFacePositionWithinCell,
            inputStackNumber,
            stacks::SendReceiveContext::PeriodicBoundaryDataSwap,
            enterCellTraversalEvent,
            &stamp
          );
        }
      }
    }
  }
}

/**
 * Enqueue boundary and fork data without consuming the observer's live
 * call-stack data because storeNodeData() persists the same records next.
 * A call-stack block is reversed relative to logical data order, so entry N
 * is accessed through top(N-1). Solver exchange guards may mask boundary and
 * periodic traffic, but fork and join contexts always copy their data.
 * The enqueue need not deep-copy a payload. Handle-backed structure-of-arrays
 * data should enqueue a non-consuming handle or pinned view so shared-memory
 * exchange does not force a host-device payload transfer.
 *
 * @see peano4::parallel::SpacetreeSet::streamDataFromSplittingTreeToNewTree()
 *   for a discussion of the splitting process and the required data flows.
 */
void peano4::grid::Spacetree::sendUserData(
  const AutomatonState&                     state,
  const tarch::la::Vector<DIMENSIONS, int>& cellIndex,
  GridTraversalObserver&                    observer,
  const GridTraversalEvent&                 leaveCellTraversalEvent,
  GridVertex                                fineGridVertices[TWO_POWER_D]
) {
  assertion(
    _spacetreeState != SpacetreeState::EmptyRun and _spacetreeState != SpacetreeState::Joined,
    peano4::grid::toString(_spacetreeState),
    state.toString(),
    leaveCellTraversalEvent.toString(),
    _id
  );

  for (int vertexEntry = 0; vertexEntry < TWO_POWER_D; vertexEntry++) {
    const int outputVertexStack              = leaveCellTraversalEvent.getVertexDataTo(vertexEntry);
    const int outputVertexPositionWithinCell = leaveCellTraversalEvent.getVertexDataFrom(vertexEntry);

    const bool isHangingVertex = leaveCellTraversalEvent.getIsVertexHanging(outputVertexPositionWithinCell);
    if (StackTopology::isInOutStack(outputVertexStack) and not isHangingVertex) {
      if (leaveCellTraversalEvent.getIsVertexLocal(outputVertexPositionWithinCell)) {
        const auto outgoingNeighbourTrees = SpacetreeHelper::getNeighbourTreesAlongVertex(
          fineGridVertices[outputVertexPositionWithinCell],
          _id,
          _splitTriggered,
          _splitting,
          _joining,
          false,
          _spacetreeState,
          _noParallelTopologyChanges
        );

        for (int neighbourIndex = 0; neighbourIndex < outgoingNeighbourTrees.count; ++neighbourIndex) {
          const int neighbourTreeId = outgoingNeighbourTrees.ids[neighbourIndex];
          logDebug(
            "Send local vertex data of " << fineGridVertices[outputVertexPositionWithinCell].toString()
                                         << " from stack " << outputVertexStack << " on tree " << _id
                                         << " to neighbour " << neighbourTreeId
                                         << ". Position within cell=" << outputVertexPositionWithinCell
          );
          const int neighbourSlot = getOrCreateNeighbourSlot(neighbourTreeId);
          assertion(neighbourSlot >= 0, _id, neighbourTreeId, outputVertexPositionWithinCell, toString());
          observer.sendNodeData(
            GridTraversalObserver::NodeData::Vertex,
            outputVertexPositionWithinCell,
            neighbourSlot,
            stacks::SendReceiveContext::BoundaryExchange,
            leaveCellTraversalEvent
          );
        }

        if (hasPeriodicBoundaryConditions(fineGridVertices[outputVertexPositionWithinCell].adjacentRanks)) {
          const auto outputStacks = StackTopology::getOutputStacksForPeriodicBoundaryExchange(
            fineGridVertices[outputVertexPositionWithinCell].adjacentRanks
          );
          const auto vertexIndex = vertexIndexOfCellVertex(cellIndex, outputVertexPositionWithinCell);
          for (const auto& outputStack : outputStacks) {
            const auto stamp = makeVertexStamp(state.level, vertexIndex, std::bitset<DIMENSIONS>(outputStack.second));
            logDebug(
              "Vertex " << fineGridVertices[outputVertexPositionWithinCell].toString() << " on tree " << _id
                        << " goes to stack " << outputStack.first << " as periodic image " << stamp.toString()
            );

            observer.sendNodeData(
              GridTraversalObserver::NodeData::Vertex,
              outputVertexPositionWithinCell,
              outputStack.first,
              stacks::SendReceiveContext::PeriodicBoundaryDataSwap,
              leaveCellTraversalEvent,
              &stamp
            );
          }
        }

        for (const auto splittingTreeId : _splitting) {
          const auto& adjacentRanks = fineGridVertices[outputVertexPositionWithinCell].backupOfAdjacentRanks;
          if (std::find(std::begin(adjacentRanks), std::end(adjacentRanks), splittingTreeId)
              != std::end(adjacentRanks)) {
            logDebug(
              "Stream local vertex data of "
              << fineGridVertices[outputVertexPositionWithinCell].toString() << " from stack " << outputVertexStack
              << " on tree " << _id << " to new worker " << splittingTreeId << ". Position within cell="
              << outputVertexPositionWithinCell << ". Event=" << leaveCellTraversalEvent.toString()
            );
            const int neighbourSlot = getOrCreateNeighbourSlot(splittingTreeId);
            assertion(neighbourSlot >= 0, _id, splittingTreeId, outputVertexPositionWithinCell, toString());
            observer.sendNodeData(
              GridTraversalObserver::NodeData::Vertex,
              outputVertexPositionWithinCell,
              neighbourSlot,
              stacks::SendReceiveContext::ForkDomain,
              leaveCellTraversalEvent
            );
          } else {
            logDebug(
              "Skip vertex data of " << fineGridVertices[outputVertexPositionWithinCell].toString() << " on tree "
                                     << _id
            );
          }
        }
      }
    }
  }

  for (int faceEntry = 0; faceEntry < TWO_TIMES_D; faceEntry++) {
    const int outputFaceStack              = leaveCellTraversalEvent.getFaceDataTo(faceEntry);
    const int outputFacePositionWithinCell = leaveCellTraversalEvent.getFaceDataFrom(faceEntry);

    const bool isHangingFace = leaveCellTraversalEvent.getIsFaceHanging(outputFacePositionWithinCell);
    if (StackTopology::isInOutStack(outputFaceStack) and not isHangingFace) {
      if (leaveCellTraversalEvent.getIsFaceLocal(outputFacePositionWithinCell)) {
        const int outgoingNeighbourTreeId = SpacetreeHelper::getNeighbourTreeAlongFace(
          fineGridVertices,
          outputFacePositionWithinCell,
          _id,
          _splitTriggered,
          _splitting,
          _joining,
          false,
          _spacetreeState,
          _noParallelTopologyChanges
        );
        if (outgoingNeighbourTreeId >= 0) {
          logDebug(
            "Send local face from stack " << outputFaceStack << " of tree " << _id << " to neighbour "
                                          << outgoingNeighbourTreeId
                                          << ". Position within cell=" << outputFacePositionWithinCell
          );

          const int neighbourSlot = getOrCreateNeighbourSlot(outgoingNeighbourTreeId);
          assertion(neighbourSlot >= 0, _id, outgoingNeighbourTreeId, outputFacePositionWithinCell, toString());
          observer.sendNodeData(
            GridTraversalObserver::NodeData::Face,
            outputFacePositionWithinCell,
            neighbourSlot,
            stacks::SendReceiveContext::BoundaryExchange,
            leaveCellTraversalEvent
          );
        }

        if (isFaceAlongPeriodicBoundaryCondition(fineGridVertices, outputFacePositionWithinCell, false)) {
          const auto stamp = makeFaceStamp(state.level, cellIndex, outputFacePositionWithinCell);
          _periodicProducerDescriptors.push_back({stamp, _id});

          const int outputStackNumber =
            StackTopology::getOutputStackForPeriodicBoundaryExchange(outputFacePositionWithinCell);

          logDebug(
            "Send face " << stamp.toString() << " of tree " << _id << " to periodic boundary stack "
                         << outputStackNumber
          );

          observer.sendNodeData(
            GridTraversalObserver::NodeData::Face,
            outputFacePositionWithinCell,
            outputStackNumber,
            stacks::SendReceiveContext::PeriodicBoundaryDataSwap,
            leaveCellTraversalEvent,
            &stamp
          );
        }
      }

      int adjacentRanks[TWO_POWER_D];
      SpacetreeHelper::getAdjacentRanksOfFace(fineGridVertices, outputFacePositionWithinCell, false, adjacentRanks);
      for (const auto splittingTreeId : _splitting) {
        if (std::find(std::begin(adjacentRanks), std::end(adjacentRanks), splittingTreeId) != std::end(adjacentRanks)) {
          logDebug(
            "Stream local face from stack " << outputFaceStack << " of tree " << _id << " to new worker "
                                            << splittingTreeId
                                            << ". Position within cell=" << outputFacePositionWithinCell
          );

          const int neighbourSlot = getOrCreateNeighbourSlot(splittingTreeId);
          assertion(neighbourSlot >= 0, _id, splittingTreeId, outputFacePositionWithinCell, toString());
          observer.sendNodeData(
            GridTraversalObserver::NodeData::Face,
            outputFacePositionWithinCell,
            neighbourSlot,
            stacks::SendReceiveContext::ForkDomain,
            leaveCellTraversalEvent
          );
        }
      }
    }
  }

  for (const auto splittingTreeId : _splitting) {
    if (SpacetreeHelper::getTreeOwningSpacetreeNode(
          fineGridVertices,
          _id,
          _splitTriggered,
          _splitting,
          _joinTriggered,
          _joining
        )
        == splittingTreeId) {
      logDebug(
        "Stream local cell of tree " << _id << " to new worker " << splittingTreeId
                                     << ". Left bottom vertex=" << fineGridVertices[0].toString()
      );

      const int neighbourSlot = getOrCreateNeighbourSlot(splittingTreeId);
      assertion(neighbourSlot >= 0, _id, splittingTreeId, 0, toString());
      observer.sendNodeData(
        GridTraversalObserver::NodeData::Cell,
        0,
        neighbourSlot,
        stacks::SendReceiveContext::ForkDomain,
        leaveCellTraversalEvent
      );
    }
  }
}

/**
 * Apply split decisions to vertex adjacency. A join in flight only enters as
 * context here; mergeCellFromWorkerWithMasterThroughoutJoin() applies it.
 *
 * Bottom-up splitting pushes one marker per leaf and reduces @f$ 3^d @f$ child
 * markers into their parent when all children move to the same tree. This
 * reduction continues after the requested leaf count reaches zero because the
 * final child group can still complete a movable subtree.
 *
 * Aggressive top-down decisions are recorded by splitCellTopDown() and applied
 * here during the bottom-up traversal. The separation prevents an adjacency
 * change from altering vertex ownership during the top-down traversal.
 *
 * @see splitCellTopDown()
 */
void peano4::grid::Spacetree::splitOrJoinCellBottomUp(
  GridVertex coarseGridVertices[TWO_POWER_D],
  GridVertex fineGridVertices[TWO_POWER_D]
) {
  [[maybe_unused]] constexpr int NoSplit = -1;

  if (not _splitTriggered.empty() and _splitTriggered.begin()->second.mode == SplitInstruction::Mode::BottomUp) {
    const bool isSplitCandidate = SpacetreeHelper::isSpacetreeNodeSplitCandidate(
      coarseGridVertices,
      fineGridVertices,
      _id,
      _splitTriggered,
      _splitting,
      _joinTriggered,
      _joining
    );
    const bool isRefined = SpacetreeHelper::isSpacetreeNodeRefined(fineGridVertices);

    logDebug(
      "Cell check for bottom-up splits with first target tree "
      << _splitTriggered.begin()->first << ". Refined=" << isRefined << ", split candidate=" << isSplitCandidate << ", "
      << fineGridVertices[0].toString()
    );

    if (isSplitCandidate and isRefined) {
      int reducedMarker = _splittedCells.back();
      for (int childIndex = 0; childIndex < THREE_POWER_D; childIndex++) {
        const int topMarker = _splittedCells.back();
        _splittedCells.pop_back();
        if (topMarker != reducedMarker) {
          reducedMarker = NoSplit;
        }
      }

      if (reducedMarker >= 0) {
        updateVertexRanksWithinCell(fineGridVertices, reducedMarker);
      }

      _splittedCells.push_back(reducedMarker);
    } else if (not isSplitCandidate and isRefined) {
      for (int childIndex = 0; childIndex < THREE_POWER_D; childIndex++) {
        _splittedCells.pop_back();
      }
      _splittedCells.push_back(NoSplit);
    } else if (isSplitCandidate and not isRefined) {
      const int targetTreeId = getSplittingTree();
      if (targetTreeId >= 0) {
        updateVertexRanksWithinCell(fineGridVertices, targetTreeId);
        updateSplittingCounter(targetTreeId);
      }
      _splittedCells.push_back(targetTreeId);
    } else {
      _splittedCells.push_back(NoSplit);
    }
  } else if (not _splitTriggered.empty()
             and _splitTriggered.begin()->second.mode == SplitInstruction::Mode::AggressiveTopDown
             and not _splittedCells.empty()) {
    const int targetTreeId = _splittedCells.back();
    _splittedCells.pop_back();

    logDebug("Cell check for top-down splits with first target tree " << _splitTriggered.begin()->first);
    assertion(
      SpacetreeHelper::isSpacetreeNodeLocal(
        fineGridVertices,
        _id,
        _splitTriggered,
        _splitting,
        _joinTriggered,
        _joining,
        false,
        false,
        false
      )
    );
    assertion(
      SpacetreeHelper::isSpacetreeNodeLocal(
        coarseGridVertices,
        _id,
        _splitTriggered,
        _splitting,
        _joinTriggered,
        _joining,
        false,
        false,
        false
      )
    );
    updateVertexRanksWithinCell(fineGridVertices, targetTreeId);
  }
}

/**
 * Record aggressive top-down split decisions. Children follow a deployed
 * parent; otherwise only a split candidate starts a new subtree. Adjacency is
 * updated later by splitOrJoinCellBottomUp() because changing it here could
 * alter vertex ownership during traversal.
 *
 * @see splitOrJoinCellBottomUp()
 */
void peano4::grid::Spacetree::splitCellTopDown(
  GridVertex coarseGridVertices[TWO_POWER_D],
  GridVertex fineGridVertices[TWO_POWER_D]
) {
  if (not _splitTriggered.empty()
      and _splitTriggered.begin()->second.mode == SplitInstruction::Mode::AggressiveTopDown) {
    const bool isParentSplit    = not _splittedCells.empty();
    const int  targetTreeId     = getSplittingTree();
    const bool isSplitCandidate = targetTreeId >= 0
      and SpacetreeHelper::isSpacetreeNodeSplitCandidate(
                                    coarseGridVertices,
                                    fineGridVertices,
                                    _id,
                                    _splitTriggered,
                                    _splitting,
                                    _joinTriggered,
                                    _joining
      );
    const bool isRefined = SpacetreeHelper::isSpacetreeNodeRefined(fineGridVertices);

    if (isParentSplit and isRefined) {
      assertion(
        SpacetreeHelper::isSpacetreeNodeLocal(
          fineGridVertices,
          _id,
          _splitTriggered,
          _splitting,
          _joinTriggered,
          _joining,
          false,
          false,
          false
        )
      );
      assertion(
        SpacetreeHelper::isSpacetreeNodeLocal(
          coarseGridVertices,
          _id,
          _splitTriggered,
          _splitting,
          _joinTriggered,
          _joining,
          false,
          false,
          false
        )
      );
      const int parentTreeId = _splittedCells.back();
      _splittedCells.push_back(parentTreeId);
    } else if (isParentSplit and not isRefined) {
      assertion(
        SpacetreeHelper::isSpacetreeNodeLocal(
          fineGridVertices,
          _id,
          _splitTriggered,
          _splitting,
          _joinTriggered,
          _joining,
          false,
          false,
          false
        )
      );
      assertion(
        SpacetreeHelper::isSpacetreeNodeLocal(
          coarseGridVertices,
          _id,
          _splitTriggered,
          _splitting,
          _joinTriggered,
          _joining,
          false,
          false,
          false
        )
      );
      updateSplittingCounter(targetTreeId);
      const int parentTreeId = _splittedCells.back();
      _splittedCells.push_back(parentTreeId);
    } else if (not isParentSplit and isSplitCandidate and isRefined) {
      assertion(
        SpacetreeHelper::isSpacetreeNodeLocal(
          fineGridVertices,
          _id,
          _splitTriggered,
          _splitting,
          _joinTriggered,
          _joining,
          false,
          false,
          false
        )
      );
      assertion(
        SpacetreeHelper::isSpacetreeNodeLocal(
          coarseGridVertices,
          _id,
          _splitTriggered,
          _splitting,
          _joinTriggered,
          _joining,
          false,
          false,
          false
        )
      );
      logDebug(
        "Refined cell at " << fineGridVertices[0].toString() << " should go with all children to tree " << targetTreeId
      );
      _splittedCells.push_back(targetTreeId);
    } else if (not isParentSplit and isSplitCandidate and not isRefined) {
      updateSplittingCounter(targetTreeId);
      assertion(
        SpacetreeHelper::isSpacetreeNodeLocal(
          fineGridVertices,
          _id,
          _splitTriggered,
          _splitting,
          _joinTriggered,
          _joining,
          false,
          false,
          false
        )
      );
      assertion(
        SpacetreeHelper::isSpacetreeNodeLocal(
          coarseGridVertices,
          _id,
          _splitTriggered,
          _splitting,
          _joinTriggered,
          _joining,
          false,
          false,
          false
        )
      );
      _splittedCells.push_back(targetTreeId);
    }
    logDebug("Split-marker stack contains " << _splittedCells.size() << " entries.");
  }
}

/**
 * Reset only the joining cell's ownership entry. A joining worker can already
 * contain adjacency updates from another joining sibling, so copying its
 * complete list would discard information held by the master.
 *
 * @see updateVertexBeforeStore()
 */
void peano4::grid::Spacetree::mergeCellFromWorkerWithMasterThroughoutJoin(
  GridVertex coarseGridVertices[TWO_POWER_D],
  GridVertex fineGridVertices[TWO_POWER_D]
) {
  for (const auto joiningWorkerId : _joining) {
    if (SpacetreeHelper::isSpacetreeNodeLocal(
          coarseGridVertices,
          _id,
          _splitTriggered,
          _splitting,
          _joinTriggered,
          _joining,
          false,
          false,
          false
        )
        and SpacetreeHelper::getTreeOwningSpacetreeNode(
              fineGridVertices,
              _id,
              _splitTriggered,
              _splitting,
              _joinTriggered,
              _joining
            )
          == joiningWorkerId) {
      logDebug("Cell from worker " << joiningWorkerId << " is merged into master " << _id);
      updateVertexRanksWithinCell(fineGridVertices, _id);
    } else {
      for (int vertexIndex = 0; vertexIndex < TWO_POWER_D; vertexIndex++) {
        logDebug("Coarse vertex: " << coarseGridVertices[vertexIndex].toString());
      }
      for (int vertexIndex = 0; vertexIndex < TWO_POWER_D; vertexIndex++) {
        logDebug("Fine vertex: " << fineGridVertices[vertexIndex].toString());
      }
    }
  }
}

/**
 * Keep completed instructions in _splitTriggered because the following sweep
 * needs their tree IDs to enter the splitting state. Top-down splits can
 * overshoot their request, so the remaining-cell counter is clamped at zero.
 */
void peano4::grid::Spacetree::updateSplittingCounter(int splittingTreeId) {
  auto splitIterator = _nextSplitInstruction;
  if (splitIterator == _splitTriggered.end() or splitIterator->first != splittingTreeId) {
    splitIterator = _splitTriggered.find(splittingTreeId);
  }
  if (splitIterator != _splitTriggered.end()) {
    splitIterator->second.numberOfFineGridCells = std::max(splitIterator->second.numberOfFineGridCells - 1, 0);
  }
}

/**
 * @return ID of the next splitting tree, or -1 if no request remains.
 */
int peano4::grid::Spacetree::getSplittingTree() {
  while (_nextSplitInstruction != _splitTriggered.end() and _nextSplitInstruction->second.numberOfFineGridCells <= 0) {
    ++_nextSplitInstruction;
  }
  return _nextSplitInstruction == _splitTriggered.end() ? -1 : _nextSplitInstruction->first;
}

/**
 * Start request selection at the first map entry for each traversal. The map
 * remains structurally unchanged while descend() advances this iterator.
 */
void peano4::grid::Spacetree::resetSplittingTreeCursor() { _nextSplitInstruction = _splitTriggered.begin(); }

const peano4::grid::GridStatistics& peano4::grid::Spacetree::getGridStatistics() const { return _statistics; }

int peano4::grid::Spacetree::getId() const { return _id; }

/** Returns the master tree id, or -1 for a root tree. */
int peano4::grid::Spacetree::getMasterId() const { return _masterId; }

/**
 * Mark both sides of a fork boundary. The refinement flag prevents
 * coarsening, while the subtree flag prevents another cut before the fork
 * settles.
 */
void peano4::grid::Spacetree::markVerticesAroundForkedCell(
  GridVertex coarseGridVertices[TWO_POWER_D],
  GridVertex fineGridVertices[TWO_POWER_D]
) const {
  peano4::dForEachCorner([&](const tarch::la::Vector<DIMENSIONS, int>&, int cornerIndex) {
    assertion(
      coarseGridVertices[cornerIndex].getState() != GridVertex::State::HangingVertex,
      coarseGridVertices[cornerIndex].toString()
    );
    coarseGridVertices[cornerIndex].setFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep, true);
    coarseGridVertices[cornerIndex].setFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep, true);
    fineGridVertices[cornerIndex].setFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep, true);
  });
}

/**
 * Register a split instruction. All splits in one sweep share the mode of the
 * lowest tree id, because both modes reuse the same marker stack and counters.
 */
void peano4::grid::Spacetree::split(int newSpacetreeId, const SplitInstruction& instruction) {
  assertion(maySplit(), newSpacetreeId, instruction, toString());
  assertion(instruction.numberOfFineGridCells >= 1, newSpacetreeId, instruction, toString());

  const bool modeNeedsNormalisation =
    not _splitTriggered.empty() and instruction.mode != _splitTriggered.begin()->second.mode;
  _splitTriggered.insert(std::pair<int, SplitInstruction>(newSpacetreeId, instruction));

  if (modeNeedsNormalisation) {
    const auto firstMode = _splitTriggered.begin()->second.mode;
    for (auto& splitEntry : _splitTriggered) {
      if (splitEntry.second.mode != firstMode) {
        logWarning(
          "Split of tree " << splitEntry.first << " is of type " << splitEntry.second.mode
                           << " while other splits have mode " << firstMode << ". Use " << firstMode
                           << " for all splits."
        );
        splitEntry.second.mode = firstMode;
      }
    }
  }
}

std::string peano4::grid::Spacetree::toString() const {
  std::ostringstream message;
  message << "Spacetree(Id=" << _id << ", MasterId=" << _masterId
          << ", State=" << peano4::grid::toString(_spacetreeState) << ", Statistics=" << _statistics;

  if (not _joinTriggered.empty()) {
    message << ", JoinTriggered={";
    for (const auto joiningTreeId : _joinTriggered) {
      message << "(" << joiningTreeId << ")";
    }
    message << "}";
  }
  if (not _joining.empty()) {
    message << ", Joining={";
    for (const auto joiningTreeId : _joining) {
      message << "(" << joiningTreeId << ")";
    }
    message << "}";
  }

  if (not _splitTriggered.empty()) {
    message << ", SplitTriggered={";
    for (const auto& splitEntry : _splitTriggered) {
      message << "(" << splitEntry.first << "," << splitEntry.second << ")";
    }
    message << "}";
  }
  if (not _splitting.empty()) {
    message << ", Splitting={";
    for (const auto splittingTreeId : _splitting) {
      message << "(" << splittingTreeId << ")";
    }
    message << "}";
  }
  message << ", NeighbourTrees=" << _neighbourTreeIds.size() << ")";
  return message.str();
}

/**
 * Report whether the tree can register a split. Cell-level topology
 * constraints are checked while the request is realised; an incomplete
 * request terminates the traversal.
 */
bool peano4::grid::Spacetree::maySplit() const {
  return _spacetreeState == SpacetreeState::Running and _joinTriggered.empty() and _joining.empty()
    and _splitting.empty() and _statistics.numberOfLocalUnrefinedCells > 0;
}

bool peano4::grid::Spacetree::mayJoinWithWorker() const {
  return _spacetreeState == SpacetreeState::Running and _splitTriggered.empty() and _joining.empty()
    and _splitting.empty() and _joinTriggered.empty();
}

/**
 * A tree can join only after it has no local refined nodes and remains
 * stationary long enough for recent fork refinement to become visible.
 *
 * @see joinWithMaster()
 */
bool peano4::grid::Spacetree::mayJoinWithMaster() const {
  return _spacetreeState == SpacetreeState::Running and _masterId >= 0
    and _statistics.numberOfStationarySweeps > NumberOfStationarySweepsToWaitAtLeastTillJoin and _splitTriggered.empty()
    and _splitting.empty() and _joinTriggered.empty() and _joining.empty()
    and _statistics.numberOfLocalRefinedCells == 0;
}

/**
 * Join a degenerate tree with its master. The stationary-sweep delay prevents
 * a newly forked tree from rejoining before its refinement becomes visible.
 */
void peano4::grid::Spacetree::joinWithMaster() {
  assertion(mayJoinWithMaster(), _id);
  _spacetreeState = SpacetreeState::JoinTriggered;
  assertion(_splitTriggered.empty());
  assertion(_splitting.empty());
  assertion(_splittedCells.empty());
  assertion(_joining.empty());
}

void peano4::grid::Spacetree::joinWithWorker(int workerId) {
  logInfo("Merge tree " << workerId << " into tree " << _id);
  assertion(_joinTriggered.count(workerId) == 0, workerId, _id);
  _joinTriggered.insert(workerId);
  assertion(_splitTriggered.empty(), workerId, toString());
  assertion(_splitting.empty(), workerId, toString());
}

bool peano4::grid::Spacetree::isInvolvedInJoinOrFork() const {
  return not _joinTriggered.empty() or not _joining.empty() or not _splitTriggered.empty() or not _splitting.empty();
}
