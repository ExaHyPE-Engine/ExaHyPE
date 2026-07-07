// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Spacetree.h"

#include "GridControlEvent.h"
#include "GridTraversalEvent.h"
#include "GridTraversalObserver.h"
#include "PeanoCurve.h"
#include "SpacetreeHelper.h"
#include "StackTopology.h"
#include "VertexType.h"

#include "peano4/Loop.h"
#include "tarch/mpi/Rank.h"

namespace {
  static constexpr auto RankOfPeriodicBoundaryCondition =
    peano4::grid::SpacetreeHelper::RankOfPeriodicBoundaryCondition;
  static constexpr auto NumberOfStationarySweepsToWaitAtLeastTillJoin =
    peano4::grid::SpacetreeHelper::NumberOfStationarySweepsToWaitAtLeastTillJoin;

  inline static bool hasPeriodicBCs(const std::array<int, TWO_POWER_D>& adj) {
    for (int i = 0; i < TWO_POWER_D; ++i) {
      if (adj[i] == RankOfPeriodicBoundaryCondition) {
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
  _gridTraversalEventGenerator(newRootId) {
  _root.x = offset;
  _root.h = width;
  _root.setInverted(false);

  for (int i = 0; i < TWO_TIMES_D; i++) {
    _root.accessNumber[i] = 0;
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
  _splitting(),
  _gridTraversalEventGenerator(newId) {
  _root.level = 0;
  _root.x     = offset;
  _root.h     = width;
  _root.setInverted(traversalInverted);

  for (int i = 0; i < TWO_TIMES_D; i++) {
    _root.accessNumber[i] = 0;
  }

  _statistics.numberOfStationarySweeps = -NumberOfStationarySweepsToWaitAtLeastTillJoin - 2;
  _statistics.clear(_id == 0);

  _noParallelTopologyChanges = true;

  logDebug("Created spacetree " << _id << " with master tree " << masterId << ": " << _root.toString());
}

int peano4::grid::Spacetree::getOrCreateNeighbourSlot(int neighbourTreeId) {
  assertion(neighbourTreeId >= 0, neighbourTreeId);
  for (std::size_t i = 0; i < _neighbourTreeIds.size(); ++i) {
    if (_neighbourTreeIds[i] == neighbourTreeId) {
      return static_cast<int>(i);
    }
  }
  const int slot = static_cast<int>(_neighbourTreeIds.size());
  _neighbourTreeIds.push_back(neighbourTreeId);
  _parallelExchangeVertexStacks.emplace_back(); // Keep vectors in sync (slot==index)
  return slot;
}

int peano4::grid::Spacetree::getNeighbourSlot(int neighbourTreeId) const {
  assertion(neighbourTreeId >= 0, neighbourTreeId);
  for (std::size_t i = 0; i < _neighbourTreeIds.size(); ++i) {
    if (_neighbourTreeIds[i] == neighbourTreeId) {
      return static_cast<int>(i);
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
 * Stamp list running in lockstep with the periodic boundary entries of the
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

peano4::stacks::Stack<peano4::grid::GridVertex>&
peano4::grid::Spacetree::getParallelExchangeVertexStack(int slot, StackTopology::ParallelExchangeStack which) {
  assertion(
    slot >= 0 and slot < static_cast<int>(_parallelExchangeVertexStacks.size()),
    slot,
    static_cast<int>(_parallelExchangeVertexStacks.size())
  );
  return _parallelExchangeVertexStacks[slot][StackTopology::toIndex(which)];
}

const peano4::stacks::Stack<peano4::grid::GridVertex>&
peano4::grid::Spacetree::getParallelExchangeVertexStack(int slot, StackTopology::ParallelExchangeStack which) const {
  assertion(
    slot >= 0 and slot < static_cast<int>(_parallelExchangeVertexStacks.size()),
    slot,
    static_cast<int>(_parallelExchangeVertexStacks.size())
  );
  return _parallelExchangeVertexStacks[slot][StackTopology::toIndex(which)];
}

int peano4::grid::Spacetree::getNumberOfNeighbourTrees() const { return static_cast<int>(_neighbourTreeIds.size()); }

int peano4::grid::Spacetree::getNeighbourTreeIdForSlot(int slot) const {
  assertion(slot >= 0 and slot < static_cast<int>(_neighbourTreeIds.size()));
  return _neighbourTreeIds[slot];
}

/**
 * @see GridTraversalEvent for a discussion of the arising actions. The traversal
 *   actually does not issue these actions. It invokes only an observer and hands
 *   in the GridTraversalEvent. It is then up to the observer to read out the
 *   event and to issue the actions.
 */
void peano4::grid::Spacetree::traverse(GridTraversalObserver& observer) {
  traceEnter();

  for (int d = 0; d < TWO_TIMES_D; d++) {
    assertion(_root.accessNumber[d] > -2 * DIMENSIONS, d, _root.toString());
    assertion(_root.accessNumber[d] < 2 * DIMENSIONS, d, _root.toString());
  }

  assertion(_joinTriggered.empty() or _splitTriggered.empty());
  assertion(_joinTriggered.empty() or _splitting.empty());
  assertion(_joining.empty() or _splitTriggered.empty());
  assertion(_joining.empty() or _splitting.empty());
  logDebug(
    _splitTriggered.size() << " tree splits triggered and " << _splitting.size() << " splitting trees on tree " << _id
  );

  _statistics.clear(_id == 0);

  _periodicProducerDescriptors.clear();
  _periodicConsumerDescriptors.clear();

  if (_spacetreeState == SpacetreeState::EmptyRun or _spacetreeState == SpacetreeState::NewFromSplit
      or _spacetreeState == SpacetreeState::Joining) {
    _gridControlEvents.clear();
  } else {
    _gridControlEvents = observer.getGridControlEvents();
    GridControlEvent::merge(_gridControlEvents);
    if (not _gridControlEvents.empty()) {
      logDebug("Spacetree " << _id << " got " << _gridControlEvents.size() << " grid control events");
    }
  }

  _splittedCells.clear();

  _noParallelTopologyChanges =
    _splitTriggered.empty() and _splitting.empty() and _joinTriggered.empty() and _joining.empty();

  const bool isFirstTraversal = _spacetreeState == SpacetreeState::NewRoot;

  GridVertex vertices[TWO_POWER_D];
  peano4::dForEachCorner([&](const tarch::la::Vector<DIMENSIONS, int>& k, int kScalar) {
    std::array<int, TWO_POWER_D> adjacentRanks;
    adjacentRanks.fill(-1);
    // A root tree owns its level-0 cell; on a worker the cell belongs to the
    // master's chain, which keeps every coarse level remote there.
    adjacentRanks[TWO_POWER_D - 1 - kScalar] = _masterId >= 0 ? _masterId : _id;
    vertices[kScalar]                        = GridVertex::create(
      0, // level
      isFirstTraversal ? GridVertex::State::Refining : GridVertex::State::Refined,
      _root.x,
      _root.h,
      kScalar,
      adjacentRanks,
      false // isNewFineGridVertex
    );
    vertices[kScalar].setFlag(GridVertex::HasBeenParentOfSubtreeVertexInPreviousTreeSweep, true);
    logDebug("Create " << vertices[kScalar].toString() << " for tree " << _id);
  });

  for (int d = 0; d < DIMENSIONS; d++) {
    if (_periodicBC[d]) {
      peano4::dForEachCorner([&](const tarch::la::Vector<DIMENSIONS, int>& vertex, int vertexScalar) {
        peano4::dForEachCorner([&](const tarch::la::Vector<DIMENSIONS, int>& entries, int entriesScalar) {
          tarch::la::Vector<DIMENSIONS, int> entry = entries;
          if (vertex(d) == 0) {
            entry(d) = 0;
          } else {
            entry(d) = 1;
          }
          vertices[vertexScalar].adjacentRanks[peano4::dLinearised(entry, 2)]         = RankOfPeriodicBoundaryCondition;
          vertices[vertexScalar].backupOfAdjacentRanks[peano4::dLinearised(entry, 2)] = RankOfPeriodicBoundaryCondition;
        });
        logDebug("Set periodic boundary conditions: " << vertices[vertexScalar].toString());
      });
    }
  }

  observer.beginTraversal(_root.x, _root.h);
  descend(_root, tarch::la::Vector<DIMENSIONS, int>(0), vertices, observer);

  _hasSplit.clear();
  for (auto& rank : _splitting) {
    _childrenIds.insert(rank);
    _hasSplit.insert(rank);
  }
  _splitting.clear();
  for (auto& p : _splitTriggered) {
    if (p.second.numberOfFineGridCells > 0) {
      logWarning(
        "Have not been able to assign enough cells from " << _id << " to new tree " << p.first
                                                          << " (should have deployed " << p.second << " more cells)"
      );
    }
    _splitting.insert(p.first);
    getOrCreateNeighbourSlot(p.first); // Ensure slot exists for vertical streaming
  }
  _splitTriggered.clear();

  _joining.clear();
  _joining.insert(_joinTriggered.begin(), _joinTriggered.end());
  _joinTriggered.clear();

  observer.endTraversal(_root.x, _root.h);

  {
    std::size_t residentVertices = 0;
    for (const auto& stack : _localExchangeVertexStacks) {
      residentVertices += stack.size();
    }
    for (const auto& slot : _parallelExchangeVertexStacks) {
      for (const auto& stack : slot) {
        residentVertices += stack.size();
      }
    }
    _statistics.numberOfVertices = static_cast<int>(residentVertices);
  }

  _root.setInverted(not _root.getInverted());

  [[maybe_unused]] const auto oldState = _spacetreeState;
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
    "Switched tree " << _id << " from " << grid::toString(oldState) << " into " << grid::toString(_spacetreeState)
  );

  traceLeave();
}

/**
 * This is a merge routine for vertical data exchange. It is important
 * that you merge
 *
 * <h2> Join behaviour </h2>
 *
 * If we are joining in another rank, we might get update adjacency information
 * for this rank. Such adjacency updates are important for the outgoing data,
 * i.e. for the send. Logically, the master and the dying worker run in parallel,
 * that is the updates on the worker belong logically into steps after the actual
 * load.
 *
 * It therefore is important that you invoke this operation afer
 * receiveAndMergeGridVertexAtVerticalBoundary() and that this routine overwrites the
 * adjacentRanks yet not the backup of this field. This way, we are consistent
 * with any horizontal merges on the worker.
 *
 * @see receiveAndMergeGridVertexAtHorizontalBoundary()
 */
void peano4::grid::Spacetree::receiveAndMergeGridVertexAtVerticalBoundary(GridVertex& vertex) {
  for (const auto& p : _joining) {
    const auto& adjacentRanks = vertex.adjacentRanks;
    if (std::find(std::begin(adjacentRanks), std::end(adjacentRanks), p) != std::end(adjacentRanks)) {
      const int slot = getOrCreateNeighbourSlot(p);
      logDebug(
        "Merge " << vertex.toString()
                 << " with its replica from worker as worker is joining. Incoming channel for worker=" << p
      );

      auto& in =
        _parallelExchangeVertexStacks[slot]
                                     [StackTopology::toIndex(StackTopology::ParallelExchangeStack::VerticalInput)];
      assertion(not in.empty(), _id, p, vertex.toString());
      GridVertex copyFromFormerWorker = in.pop();

#if PEANO_DEBUG
      assertion(
        tarch::la::equals(vertex.getX(), copyFromFormerWorker.getX()),
        vertex.getX(),
        copyFromFormerWorker.getX(),
        vertex.toString(),
        copyFromFormerWorker.toString(),
        _id
      );
#endif

      vertex.adjacentRanks = copyFromFormerWorker.adjacentRanks;
      logDebug("Updated adjacency lists: " << vertex.toString());
    }
  }
}

/**
 * This operation has multiple purposes
 *
 * - Merge with neighbour vertices. See
 *   receiveAndMergeGridVertexAtHorizontalBoundary().
 * - Roll over the flags. These guys now are merged already. See
 *   receiveAndMergeGridVertexAtHorizontalBoundary().
 * - Do the refinement flag (state) update.
 * - Erase non-local vertices if they do not carry a veto flag.
 *
 *
 * <h2> Flag update </h2>
 *
 * Implements the standard refinement status transition, i.e. a triggered
 * becomes actually ing. And if a vertex has been refining or erasing in
 * the previous sweep, we finally update it to refined or unrefined.
 *
 * This operation has to be called whenever we load a vertex from the input
 * stream, i.e. we touch it for the very first time. We are not allowed to
 * invoke it after we've created a new vertex.
 *
 * We don't do any statistics here. I've moved all the statistics into
 * updateVertexBeforeStore().
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
    receiveAndMergeGridVertexAtHorizontalBoundary(vertex, vertexIndex, vertexIsAdjacentToLocalTree);
    receiveAndMergeGridVertexAtVerticalBoundary(vertex);

    // We cache, per coarse-grid vertex, whether it should be treated as hanging-like when
    // reconstructing the previous sweep’s derived vertex types.
    //
    // This is not the same as the vertex state was literally HangingVertex.
    // The flag is used by getCoarseGridVertexTypeFromPreviousTreeSweep() /
    // getVertexTypesFromPreviousTreeSweep() to rebuild the previous VertexType[] array for a cell.
    // That reconstruction only distinguishes two coarse categories: {Persistent, Hanging}.
    // Several transient / AMR-boundary states are conservatively mapped to VertexType::Hanging,
    // because they indicate the vertex is on/near a refinement/coarsening interface where
    // fine-grid entities can be hanging and their data may not be stable like interior entities.
    //
    // Rationale for each state treated as hanging-like (=> VertexType::Hanging):
    // - RefinementTriggered: refinement will introduce fine-grid constraints at this vertex.
    // - New: vertex has just been created by AMR; it is not a stable interior vertex yet.
    // - Delete: vertex is being removed by AMR; treat as non-persistent for previous-type reconstruction.
    // - Unrefined: vertex belongs to a coarse region adjacent to refinement; this typically induces
    //   hanging configurations on the fine grid when projected down.
    // - HangingVertex: the literal hanging-vertex state.
    //
    // All other states are treated as persistent interior for the purposes of the previous-sweep
    // VertexType reconstruction.
    const auto s                              = vertex.getState();
    const bool prevCoarseVertexTypeWasHanging = s == GridVertex::State::RefinementTriggered
      or s == GridVertex::State::Delete or s == GridVertex::State::New or s == GridVertex::State::Unrefined
      or s == GridVertex::State::HangingVertex;
    vertex.setFlag(GridVertex::HasBeenVertexTypeHangingInPreviousTreeSweep, prevCoarseVertexTypeWasHanging);

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
        // the tree from merging into its father.
      }
    } else if (vertex.getState() == GridVertex::State::EraseTriggered) {
      if (vertex.getFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep)) {
        logDebug(
          "Vertex " << vertex.toString() << " may not be erased on tree " << _id
                    << " as it is father of further refined vertices. Unroll flag."
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

    // Has to run here so it does not interfere with the state splitting above.
    if (not vertexIsAdjacentToLocalTree
        and not vertex.getFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep)
        and vertex.getState() == GridVertex::State::Refined) {
      logDebug("Would like to erase " << vertex.toString() << " in spacetree " << _id);
      vertex.setState(GridVertex::State::EraseTriggered);
    }
  }

  if (_spacetreeState == SpacetreeState::Joining and vertexIsAdjacentToLocalTree) {
    const int slot = getOrCreateNeighbourSlot(_masterId);
    logDebug(
      "Stream vertex " << vertex.toString() << " from tree " << _id << " to master " << _masterId
                       << " through vertical exchange channel"
    );
    _parallelExchangeVertexStacks[slot][StackTopology::toIndex(StackTopology::ParallelExchangeStack::VerticalOutput)]
      .push(std::move(vertex));
  }
}

/**
 * Determines whether to restrict a vertex to the coarser level or not.
 */
bool peano4::grid::Spacetree::restrictToCoarseGrid(
  const tarch::la::Vector<DIMENSIONS, int>& coarseVertexPosition,
  const tarch::la::Vector<DIMENSIONS, int>& fineVertexPosition
) {
  bool result = true;
  for (int d = 0; d < DIMENSIONS; d++) {
    assertion(coarseVertexPosition(d) >= 0, coarseVertexPosition, fineVertexPosition);
    assertion(coarseVertexPosition(d) <= 1, coarseVertexPosition, fineVertexPosition);

    assertion(fineVertexPosition(d) >= 0, coarseVertexPosition, fineVertexPosition);
    assertion(fineVertexPosition(d) <= 3, coarseVertexPosition, fineVertexPosition);

    result &=
      ((coarseVertexPosition(d) == 1 and fineVertexPosition(d) > 0)
       or (coarseVertexPosition(d) == 0 and fineVertexPosition(d) < 3));
  }

  return result;
}

/**
 * <h2> Restriction of veto flags </h2>
 *
 * We make each vertex hold a flag isAntecessorOfRefinedVertex. If it is
 * set, we veto any erasing here. This way, we can ensure that the trees
 * remove at most one level at a time. We illustrate all further
 * explanations with a simple example:
 *
 * @image html Spacetree_updateVertexBeforeStore_restrictIsAntecessorOfRefinedVertex.png
 *
 * Let the red vertices be refined. In a serial code, the tree is not
 * split. You have to glue together the left and right tree. As the middle
 * level's vertex holds the refinement flag, the top level vertex carries
 * the flag isAntecessorOfRefinedVertex. It consequently never is erased.
 * Users first have to remove the finer level.
 *
 * If we split up tree into two threads, this approach fails if the flag
 * is restricted per core in the bottom-up steps. By the end of the sweep,
 * the flag is set on the right tree, but it is not set on the left tree.
 * We consequently might decide to erase on the left tree but veto this on
 * the right tree.
 *
 * To eliminate this behaviour, we split up the flag into a current flag
 * and a flag from the previous solution. This flag is rolled over. If the
 * flag is set, it undoes any erase trigger.
 *
 * <h2> Clean-up of vertices </h2>
 *
 * I originally had the following idea:
 *
 * If a vertex is remote, we manually clear all of its adjacency flags.
 * This is a security thing. It might happen that such a vertex refers to
 * a tree x and that this index x is later re-used (for a new local subtree
 * for example). In this case, a left-over index might suddenly suggest that
 * a totally irrelevant vertex is adjacent to the newish x tree.
 *
 * However, this is wrong in some cases: We have to keep our adjacency.
 * Because if we merge later on, we rely on this adjacency to find out
 * which vertices we have to join.
 *
 * So we have to run for the middle way: We erase all adjacency data
 * only when a vertex is not adjacent to any kid.
 *
 * <h2> A posteriori refinement </h2>
 *
 * If a vertex is surrounded by @f$ 2^d @f$ refined cells and is not a refined
 * vertex, we have this weird situation that we have a hanging vertex right
 * within a regularly refined subdomain. Topologically, this is allowed, but
 * it makes no sense, introduces strange artefacts in the visualisation and
 * is very difficult to explain. So I keep track of the refined adjacent cells
 * and refine a posteriori if all adjacent cells are refined.
 *
 * @param fineVertexPositionWithinPatch Position of vertex within 3x3 or 3x3x3 patch respectively
 *
 * @see updateVertexAfterLoad()
 * @see mergeCellFromWorkerWithMasterThroughoutJoin() for an explanation why we have to
 *         keep all adjacency information that we actually need later for
 *         merges.
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

    if (_spacetreeState == SpacetreeState::Running and vertex.numberOfAdjacentRefinedLocalCells == TWO_POWER_D
        and vertex.getState() == GridVertex::State::Unrefined) {
      vertex.setState(GridVertex::State::RefinementTriggered);
      logDebug(
        "Have to post-refine vertex " << vertex.toString() << " as it is surrounded by 2^d refined cells on tree "
                                      << _id << " x " << peano4::grid::toString(_spacetreeState)
      );
    }

    bool restrictIsAntecessorOfRefinedVertex =
      vertex.getFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep);

    if (vertex.getState() == GridVertex::State::RefinementTriggered) {
      restrictIsAntecessorOfRefinedVertex = true;
    } else if (vertex.getState() == GridVertex::State::Refining) {
      vertex.setState(GridVertex::State::Refined);
      restrictIsAntecessorOfRefinedVertex = true;
    } else if (vertex.getState() == GridVertex::State::Refined) {
      // If a vertex is refined, there exists a finer level below it
      restrictIsAntecessorOfRefinedVertex = true;
    } else if (vertex.getState() == GridVertex::State::EraseTriggered) {
      restrictIsAntecessorOfRefinedVertex = true;
    } else if (vertex.getState() == GridVertex::State::Erasing) {
      vertex.setState(GridVertex::State::Unrefined);
    }

    sendGridVertex(vertex, vertexIndex);

    if (restrictIsAntecessorOfRefinedVertex) {
      peano4::dForEachCorner([&](const tarch::la::Vector<DIMENSIONS, int>& k, int kScalar) {
        if (restrictToCoarseGrid(k, fineVertexPositionWithinPatch)) {
          logDebug(
            "Set antecessor flag (veto coarsening) on vertex " << coarseGridVertices[kScalar].toString()
                                                               << " due to vertex " << vertex.toString()
          );
          coarseGridVertices[kScalar].setFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep, true);
          coarseGridVertices[kScalar].setFlag(
            GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep,
            coarseGridVertices[kScalar].getFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep)
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
 *
 * <h2> Implementation </h2>
 *
 * We only remove adjacency information for unrefined outside vertices
 * and we are very restrictive what we consider to be outside. Indeed,
 * we consider any vertex still local when we are in a join or fork.
 * If a vertex is really outside but refined, we wait for the erase to
 * pass through before we consider erasing any adjacency info.
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
    for (int d = 0; d < DIMENSIONS; d++) {
      if (position(d) != 0 and position(d) != 3) {
        tarch::la::Vector<DIMENSIONS, int> positionA = position;
        tarch::la::Vector<DIMENSIONS, int> positionB = position;

        positionA(d) = 0;
        positionB(d) = 3;

        return self(self, positionA) and self(self, positionB);
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

  peano4::dForEachCorner([&](const tarch::la::Vector<DIMENSIONS, int>& k, int kScalar) {
    std::bitset<DIMENSIONS> vertexToInherit;
    std::bitset<DIMENSIONS> indexToInherit;
    for (int d = 0; d < DIMENSIONS; d++) {
      if (vertexPositionWithin3x3Patch(d) <= 1) {
        vertexToInherit.set(d, false);
        indexToInherit.set(d, k(d) > 0 or vertexPositionWithin3x3Patch(d) > 0);
      } else {
        vertexToInherit.set(d, true);
        indexToInherit.set(d, not(k(d) < 1 or vertexPositionWithin3x3Patch(d) < 3));
      }
    }
    logDebug(
      "Inherit " << indexToInherit.to_ulong() << "th index from coarse vertex " << vertexToInherit
                 << " into local index " << kScalar
    );
    adjacentRanks[kScalar] = coarseGridVertices[vertexToInherit.to_ulong()].adjacentRanks[indexToInherit.to_ulong()];
  });

  return adjacentRanks;
}

/**
 * Load the vertices of one cell
 *
 * The load process has to be done along the local order of the Peano
 * SFC. For this, I rely on PeanoCurve::getFirstVertexIndex().
 *
 * <h2> Hanging vertices </h2>
 *
 * I originally thought that I might be able to run hanging vertices
 * through the in/out stacks, too. In Peano 3, I had such a feature.
 * Yet, there I had a way more complicated logic. In Peano 4, I simplify
 * the logic. As a consequence, I don't know how many adjacent cells a
 * hanging vertex has and thus create a hanging vertex per adjacent
 * cell.
 */
void peano4::grid::Spacetree::loadVertices(
  const AutomatonState&                     fineGridStatesState,
  const tarch::la::Vector<DIMENSIONS, int>& cellIndex,
  GridVertex                                coarseGridVertices[TWO_POWER_D],
  GridVertex                                fineGridVertices[TWO_POWER_D],
  const VertexType                          vertexTypes[TWO_POWER_D],
  const tarch::la::Vector<DIMENSIONS, int>& cellPositionWithin3x3Patch
) {
  _vertexLookupTables.push_back(&createVertexLookupTable(fineGridStatesState, cellPositionWithin3x3Patch));
  const auto& lut = *_vertexLookupTables.back();

  int                        cachedStackNo = -1;
  stacks::Stack<GridVertex>* cachedStack   = nullptr;

  for (int i = 0; i < TWO_POWER_D; i++) {
    const int  vIndex  = lut.vIdx[i];
    auto&      vSlot   = fineGridVertices[vIndex];
    const auto vPos    = lut.vPos[i];
    VertexType type    = vertexTypes[vIndex];
    const int  stackNo = lut.readStack[i];
    const bool inOut   = lut.inOutRead[i];

    if (not inOut and type == VertexType::New) {
      type = VertexType::Persistent;
    }

    switch (type) {
    case VertexType::New:
      assertion(inOut);
      vSlot = GridVertex::create(
        fineGridStatesState.level,
        GridVertex::State::New,
        fineGridStatesState.x,
        fineGridStatesState.h,
        vIndex,
        getAdjacentRanksForNewVertex(coarseGridVertices, vPos),
        true
      );
      break;
    case VertexType::Hanging:
      vSlot = GridVertex::create(
        fineGridStatesState.level,
        GridVertex::State::HangingVertex,
        fineGridStatesState.x,
        fineGridStatesState.h,
        vIndex,
        getAdjacentRanksForNewVertex(coarseGridVertices, vPos),
        true
      );
      break;
    case VertexType::Persistent: {
      if (stackNo != cachedStackNo) {
        assertion(stackNo >= 0 and stackNo < StackTopology::TotalNumberOfLocalExchangeStacks, stackNo, _id);
        cachedStack   = &_localExchangeVertexStacks[stackNo];
        cachedStackNo = stackNo;
      }
      auto* popStack = cachedStack;
      assertion(not popStack->empty(), _id, stackNo, vIndex);
      vSlot = popStack->pop();
      if (inOut) {
        updateVertexAfterLoad(vSlot, vertexIndexOfCellVertex(cellIndex, vIndex));
      }
    } break;
    case VertexType::Delete: {
      if (stackNo != cachedStackNo) {
        assertion(stackNo >= 0 and stackNo < StackTopology::TotalNumberOfLocalExchangeStacks, stackNo, _id);
        cachedStack   = &_localExchangeVertexStacks[stackNo];
        cachedStackNo = stackNo;
      }
      auto* popStack = cachedStack;
      assertion(not popStack->empty(), stackNo, _id);
      vSlot = popStack->pop();
      if (inOut) {
        updateVertexAfterLoad(vSlot, vertexIndexOfCellVertex(cellIndex, vIndex));
        vSlot.setState(GridVertex::State::Delete);
      }
    } break;
    }

#if PEANO_DEBUG
    const auto expectedX = GridVertex::getVertexCoordinate(fineGridStatesState.x, fineGridStatesState.h, vIndex);
    assertion(
      tarch::la::equals((vSlot.getX()), (expectedX)),
      vSlot.getX(),
      expectedX,
      vSlot.toString(),
      fineGridStatesState.toString(),
      vIndex,
      toString(),
      _id
    );
    vSlot.setX(expectedX);
#endif

#if PEANO_DEBUG
    logDebug(
      "Handled " << peano4::grid::toString(type) << " vertex " << vIndex << " at " << vPos << ": " << vSlot.toString()
    );
    assertion(
      (vSlot.level) == (fineGridStatesState.level),
      vSlot.level,
      fineGridStatesState.level,
      vSlot.toString(),
      fineGridStatesState.toString(),
      vIndex,
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
  const auto& lut = *_vertexLookupTables.back();

  int                        cachedStackNo = -1;
  stacks::Stack<GridVertex>* cachedStack   = nullptr;

  for (int i = 0; i < TWO_POWER_D; i++) {
    const int  vIndex  = lut.vIdx[i];
    auto&      vSlot   = fineGridVertices[vIndex];
    VertexType type    = vertexTypes[vIndex];
    const int  stackNo = lut.writeStack[i];
    const bool inOut   = lut.inOutWrite[i];

    if (not inOut and type == VertexType::Delete) {
      type = VertexType::Persistent;
    }

    if (type == VertexType::Hanging) {
      _statistics.numberOfHangingVertices++;
      continue;
    }
    if (type == VertexType::Delete) {
      assertion(inOut);
      continue;
    }

    if (inOut) {
      updateVertexBeforeStore(vSlot, coarseGridVertices, lut.vPos[i], vertexIndexOfCellVertex(cellIndex, vIndex));
    }
    if (stackNo != cachedStackNo) {
      assertion(stackNo >= 0 and stackNo < StackTopology::TotalNumberOfLocalExchangeStacks, stackNo, _id);
      cachedStack   = &_localExchangeVertexStacks[stackNo];
      cachedStackNo = stackNo;
    }
    auto* pushStack = cachedStack;
    pushStack->push(std::move(vSlot));
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

  bool isAdjacentToLocalRank              = false;
  bool holdsPeriodicBoundaryConditionFlag = false;
  for (int i = 0; i < TWO_POWER_D; i++) {
    isAdjacentToLocalRank |= adjacentRanksOfFace[i] == _id;
    holdsPeriodicBoundaryConditionFlag |= adjacentRanksOfFace[i] == RankOfPeriodicBoundaryCondition;
  }

  holdsPeriodicBoundaryConditionFlag &= isAdjacentToLocalRank;

  return holdsPeriodicBoundaryConditionFlag;
}

void peano4::grid::Spacetree::mergeGridVertexAdjacencyListsAtHorizontalDomainBoundary(
  GridVertex&       vertex,
  const GridVertex& inVertex,
  int               neighbour
) {
  //
  // Merge adjacency lists. The neighbour owns some entries
  //
  for (int i = 0; i < TWO_POWER_D; i++) {
    // The ownership decision must be based on the pre-merge adjacency list.
    // Otherwise, earlier merges within the same receive can change
    // vertex.adjacentRanks and thereby change ownership for later neighbours.
    if (vertex.backupOfAdjacentRanks[i] == neighbour and neighbour != -1) {
      if (inVertex.adjacentRanks[i] != vertex.adjacentRanks[i]) {
        logDebug(
          "Update " << i << "th entry of adjacency data of vertex " << vertex.toString() << " with neighbouring vertex "
                    << inVertex.toString() << " on tree " << _id
        );
      }
      vertex.adjacentRanks[i] = inVertex.adjacentRanks[i];
    }
  }
}

/**
 * Called by receiveAndMergeGridVertexAtHorizontalBoundary(). Besides tons of assertions,
 * the routine realises the mergers of the refinement flags and the
 * setIsAntecessorOfRefinedVertexInCurrentTreeSweep. Please consult
 * receiveAndMergeGridVertexAtHorizontalBoundary() for a higher level overview.
 */
void peano4::grid::Spacetree::mergeGridVertexRefinementStateAtHorizontalDomainBoundary(
  GridVertex&       vertex,
  const GridVertex& inVertex,
  int               neighbour
) {
  assertion(
    vertex.getState() != GridVertex::State::HangingVertex,
    inVertex.toString(),
    vertex.toString(),
    _id,
    neighbour
  );
  assertion(
    inVertex.getState() != GridVertex::State::HangingVertex,
    inVertex.toString(),
    vertex.toString(),
    _id,
    neighbour
  );
#if PEANO_DEBUG
  assertion(
    tarch::la::equals(vertex.getX(), inVertex.getX())
    or
    (
      std::find(
        std::begin(vertex.adjacentRanks),
        std::end(vertex.adjacentRanks),
        RankOfPeriodicBoundaryCondition
      ) != std::end(vertex.adjacentRanks)
      and
      (_periodicBC.count()>1 or tarch::la::countEqualEntries(vertex.getX(), inVertex.getX())==DIMENSIONS-static_cast<int>(_periodicBC.count()))
    ),
    inVertex.toString(), vertex.toString(), _id, neighbour,
    tarch::la::countEqualEntries(vertex.getX(), inVertex.getX()), _periodicBC
  );
#endif
  assertion(
    (vertex.level) == (inVertex.level),
    vertex.level,
    inVertex.level,
    inVertex.toString(),
    vertex.toString(),
    _id,
    neighbour
  );
  assertion(
    not(vertex.getState() == GridVertex::State::Refined and inVertex.getState() == GridVertex::State::Erasing),
    inVertex.toString(),
    vertex.toString(),
    _id,
    neighbour
  );
  assertion(
    not(vertex.getState() == GridVertex::State::Refined and inVertex.getState() == GridVertex::State::Unrefined),
    inVertex.toString(),
    vertex.toString(),
    _id,
    neighbour
  );

  //
  // Merge helper flags
  //
  vertex.setFlag(
    GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep,
    vertex.getFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep)
      or inVertex.getFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep)
  );

  //
  // Update refinement flags
  //
  assertion(vertex.getState() != GridVertex::State::Erasing, vertex.toString(), inVertex.toString(), _id);
  assertion(vertex.getState() != GridVertex::State::Refining, vertex.toString(), inVertex.toString(), _id);

  const bool onlyNeighbourHasTriggeredRefinement = inVertex.getState() == GridVertex::State::RefinementTriggered
    and (vertex.getState() == GridVertex::State::Unrefined or vertex.getState() == GridVertex::State::New
         or vertex.getState() == GridVertex::State::Delete);
  const bool onlyLocalHasTriggeredRefinement = vertex.getState() == GridVertex::State::RefinementTriggered
    and (inVertex.getState() == GridVertex::State::Unrefined or inVertex.getState() == GridVertex::State::New
         or inVertex.getState() == GridVertex::State::Delete);
  const bool onlyNeighbourHasTriggeredErase =
    inVertex.getState() == GridVertex::State::EraseTriggered and vertex.getState() == GridVertex::State::Refined;
  const bool onlyLocalHasTriggeredErase =
    vertex.getState() == GridVertex::State::EraseTriggered and inVertex.getState() == GridVertex::State::Refined;

  if (onlyNeighbourHasTriggeredRefinement or onlyLocalHasTriggeredRefinement) {
    vertex.setState(GridVertex::State::RefinementTriggered);
    logDebug("Set state to " << vertex.toString() << " on tree " << _id << " due to merge with neighbour");
  } else if (onlyNeighbourHasTriggeredErase or onlyLocalHasTriggeredErase) {
    vertex.setState(GridVertex::State::EraseTriggered);
    logDebug("Set state to " << vertex.toString() << " on tree " << _id << " due to merge with neighbour");
  } else {
    assertion(vertex.getState() == inVertex.getState(), inVertex.toString(), vertex.toString(), _id, neighbour);
  }
}

/**
 * Manage the data exchange after a vertex is loaded for the first time
 *
 * The operation has three jobs to do:
 * - We backup the adjacency ranks.
 * - Exchange vertices along domain boundary.
 * - Exchange vertices belonging to periodic boundaries.
 *
 * The order of these three steps is important. The first one is a simple
 * copy. The other ones loop over neighbours and call a series of operations.
 * Logically, domain boundaries and periodic boundaries for me are both
 * realised by domain cuts. Therefore, we use the same routines within.
 *
 * <h2> Backup of adjacency data </h2>
 *
 * It is convenient to merge the adjacency flags straightaway after a vertex
 * has been loaded and its boundary counterparts have dropped in. However, once
 * we merge we lose the information about the previous traversal's adjacency.
 * This means, when we construct the neighbour information (who merges with
 * what) for the user data (createEnterCellTraversalEvent()) we lack the
 * information we actually need. Therefore, this routine backups the
 * adjacency information from the vertex.
 *
 * <h2> Boundary data exchange (grid) </h2>
 *
 * For a local vertex, i.e. one adjacent to the current active tree, which
 * is neighbouring another tree, we do receive this tree's data copy and
 * then merge it into the local tree.
 *
 * This approach doesn't work straightforwardly when we split: Lets assume
 * tree 0 is triggered to split into 0 and 1. In a first step, tree 0 enters
 * the state split triggered. It now updates all adjacency lists, so
 * boundary data is already sent out to 1 - even though 1 is not instantiated
 * yet. Once we are done, we copy over (parts of) tree 0 into tree 1, so
 * tree 1 now does exist and is well-prepared to receive the data already
 * dropped by 0. This second step is the actual splitting step. Now, other
 * ranks have still sent in data to 0 which actually should go to 1. We
 * receive those guys, but we have to eliminate explicitly any incoming data
 * we'd expect from 1 as 1 didn't have the chance yet to send it out.
 *
 * The function is called directly
 * after a vertex has been read from the input stream. An update of the
 * refinement states (e.g. switch from
 * refinement-triggered to refining) happens after the merge. Any update of the
 * refinement state in this operation hence immediately affects the
 * vertex updates in this very iteration.
 *
 * The update of the adjacency information is simple: If a neighbour tells us
 * that it has changed an adjacency entry in one of its own fields, i.e. if
 * it has changed its own entry, we copy this information over. Otherwise, we
 * ignore any updates (there should not be any).
 *
 * <h2> Periodic boundary conditions </h2>
 *
 * Periodic boundary conditions fall back to standard boundary data
 * exchanges through specialised stacks on any tree: the exchange deposits
 * every wrapped image of this vertex, stamped with its integer identity,
 * onto this tree's periodic input stacks, and we pop each partner copy by
 * stamp and merge its refinement state. This keeps the refinement of the
 * two sides of a wrap in lockstep, which is what makes AMR across a
 * periodic boundary work. Adjacency data needs no update: the
 * RankOfPeriodicBoundaryCondition markers are static domain-boundary
 * classification.
 *
 * <h2> Horizontal vs. vertical </h2>
 *
 * Has to happen before receiveAndMergeGridVertexAtVerticalBoundary(). See the
 * receiveAndMergeGridVertexAtVerticalBoundary()'s documentation for an explanation.
 * Some more reasons are given in the guidebook.
 *
 * <h2> Context </h2>
 *
 * The routine is called by updateVertexAfterLoad(). The counterpart of the
 * routine is sendGridVertex(). However, as my sends are literally
 * just memcopies, sends are way simpler than this routine. The operation
 * affects solely the grid's vertices. It does not interfere with any user
 * data. In principle, this follows my pattern that the grid has to be there
 * first and then events are triggered afterwards.
 */
void peano4::grid::Spacetree::receiveAndMergeGridVertexAtHorizontalBoundary(
  GridVertex&                               vertex,
  const tarch::la::Vector<DIMENSIONS, int>& vertexIndex,
  bool                                      vertexIsAdjacentToLocalTree
) {
  assertion(_spacetreeState != SpacetreeState::NewFromSplit and _spacetreeState != SpacetreeState::EmptyRun);

  //
  // Backup adjacency lists
  //
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
  for (int n = 0; n < receivingNeighbourTrees.count; ++n) {
    const int neighbour = receivingNeighbourTrees.ids[n];
    assertion(neighbour >= 0, neighbour);
    const int slot = getNeighbourSlot(neighbour);
    assertion(slot >= 0, _id, neighbour, vertex.toString(), toString());
    auto& in =
      _parallelExchangeVertexStacks[slot]
                                   [StackTopology::toIndex(StackTopology::ParallelExchangeStack::HorizontalInput)];
    assertion(not in.empty(), _id, neighbour, vertex.toString(), in.toString());
    GridVertex inVertex = in.pop();
    mergeGridVertexRefinementStateAtHorizontalDomainBoundary(vertex, inVertex, neighbour);
    mergeGridVertexAdjacencyListsAtHorizontalDomainBoundary(vertex, inVertex, neighbour);
  }

  if (vertexIsAdjacentToLocalTree and hasPeriodicBCs(vertex.adjacentRanks)) {
    const auto outStacks = StackTopology::getOutputStacksForPeriodicBoundaryExchange(vertex.adjacentRanks);
    for (const auto& p : outStacks) {
      // p.first  = periodic BC output stack number (local)
      // p.second = bitmask of the wrapped axes of this image
      assertion(
        StackTopology::isPeriodicBoundaryExchangeOutputStackNumber(p.first),
        _id,
        p.first,
        p.second,
        vertex.toString()
      );
      const auto stamp = makeVertexStamp(vertex.level, vertexIndex, std::bitset<DIMENSIONS>(p.second));
      _periodicConsumerDescriptors.push_back({stamp, _id});

      const int fromStack = StackTopology::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(p.first);
      assertion(
        fromStack >= 0 and fromStack < StackTopology::TotalNumberOfLocalExchangeStacks,
        fromStack,
        p.first,
        StackTopology::TotalNumberOfLocalExchangeStacks,
        _id
      );
      // One copy arrives per tree holding the wrapped image; merge them all,
      // like the horizontal merge handles one copy per neighbour tree. On the
      // first sweep after a regrid the partner may not have produced yet, so
      // finding nothing is legitimate then.
      while (auto inVertex = popPeriodicEntryMatching(
               _localExchangeVertexStacks[fromStack],
               _localExchangeVertexStackStamps[fromStack],
               stamp
             )) {
        logDebug("Merge periodic image " << stamp.toString() << " into " << vertex.toString() << " on tree " << _id);
        mergeGridVertexRefinementStateAtHorizontalDomainBoundary(vertex, *inVertex, _id);
      }
    }
  }
}

/**
 * Send a copy of the vertex to every neighbouring tree. The routine has to
 * run exactly when the vertex goes to the in/out stacks, so
 * updateVertexBeforeStore() calls it just before the vertex moves to the
 * output stack: every vertex is sent once per sweep, and never during an
 * empty tree run. The neighbour resolution returns each adjacent tree once,
 * so no tree receives duplicate copies.
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

  for (int n = 0; n < outgoingNeighbourTrees.count; ++n) {
    const int p = outgoingNeighbourTrees.ids[n];
    //
    // Boundary exchange
    //
    const int slot = getOrCreateNeighbourSlot(p);

    GridVertex vertexCopy = vertex;
    for (int i = 0; i < TWO_POWER_D; i++) {
      int targetRank = vertexCopy.adjacentRanks[i];
      if (_spacetreeState == SpacetreeState::JoinTriggered and targetRank == _id and p != _masterId) {
        vertexCopy.adjacentRanks[i] = _masterId;
      }
    }

    _parallelExchangeVertexStacks[slot][StackTopology::toIndex(StackTopology::ParallelExchangeStack::HorizontalOutput)]
      .push(std::move(vertexCopy));
  }

  // The marker test comes first, so runs without periodic boundary conditions
  // skip the locality evaluation altogether.
  if (hasPeriodicBCs(vertex.adjacentRanks)
      and SpacetreeHelper::isVertexAdjacentToLocalSpacetree(
        vertex, _id, _splitTriggered, _splitting, _joinTriggered, _joining, true, true, false
      )) {
    const auto outStacks = StackTopology::getOutputStacksForPeriodicBoundaryExchange(vertex.adjacentRanks);
    for (const auto& s : outStacks) {
      assertion(s.first >= 0 and s.first < StackTopology::TotalNumberOfLocalExchangeStacks, s.first, _id);
      const auto stamp = makeVertexStamp(vertex.level, vertexIndex, std::bitset<DIMENSIONS>(s.second));
      _periodicProducerDescriptors.push_back({stamp, _id});
      _localExchangeVertexStacks[s.first].push(vertex);
      _localExchangeVertexStackStamps[s.first].push_back(stamp);
      logDebug(
        "Vertex " << vertex.toString() << " on tree " << _id << " goes to stack " << s.first << " as periodic image "
                  << stamp.toString()
      );
    }
  }
}

/**
 * Every local refined cell should call this routine. We increment the
 * respective counter. Upon storage, we then refine if the counter
 * equals 2^d. This way we avoid hanging vertices within the domain.
 */
void peano4::grid::Spacetree::incrementNumberOfAdjacentRefinedLocalCells(GridVertex vertices[TWO_POWER_D]) {
  peano4::dForEachCorner([&](const tarch::la::Vector<DIMENSIONS, int>& k, int kScalar) {
    vertices[kScalar].numberOfAdjacentRefinedLocalCells += 1;
  });
}

/**
 * If a cell gets a new id, we have to update its vertices. This routine is
 * only used for splits. I originally thought I might use it for joins as
 * well. Indeed I can. But I can only do this on the master. The worker may
 * not update any cell immediately. If I do this, then the adjacency
 * information of the adjacent vertices is overwritten and I loose this
 * clue that these vertices are adjacent to the local, joining rank.
 *
 * If we split, we have to be careful that we preserve the master-worker
 * topology over the ranks. So this routine may only be called on the master,
 * on a cell that is local, and on a cell whose parent is local, too.
 */
void peano4::grid::Spacetree::updateVertexRanksWithinCell(GridVertex fineGridVertices[TWO_POWER_D], int newId) {
  peano4::dForEachCorner([&](const tarch::la::Vector<DIMENSIONS, int>& k, int kScalar) {
    fineGridVertices[kScalar].adjacentRanks[TWO_POWER_D - kScalar - 1] = newId;
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
      // Determine if the event overlaps the state
      //
      // event:  [eventMin --------------------- eventMax]
      // state:       [stateMin ---- stateMax]
      //               ^ overlap because:
      //                 stateMax ≥ eventMin    and    stateMin ≤ eventMax
      //
      // event:  [eventMin ---- eventMax]
      // state:                        [stateMin----stateMax]
      //                               ^ touches at eventMax == stateMin
      //
      const bool overlaps =
        tarch::la::allGreaterEquals(stateMax, eventMin) and tarch::la::allSmallerEquals(stateMin, eventMax);

      // If the event's target resolution is smaller, we refine.
      const bool refineToTargetResolution = tarch::la::allSmaller(event.targetH, fineGridState.h);

      if (refineToTargetResolution and overlaps) {
        wantsRefine = true;
        break; // Refine dominates any later erase
      }
    } else if (event.getRefinementControl() == GridControlEvent::RefinementControl::Erase) {
      // Determine if the event contains the state
      //
      // event:  [eventMin --------------------- eventMax]
      // state:       [stateMin ---- stateMax]
      //               ^ contains because:
      //                 stateMin ≥ eventMin    and    stateMax ≤ eventMax
      //
      const bool contains =
        tarch::la::allGreaterEquals(stateMin, eventMin) and tarch::la::allSmallerEquals(stateMax, eventMax);

      const bool coarsenToTargetResolution =
        // If the event's target resolution is greater or equal, we coarsen.
        tarch::la::allGreaterEquals(event.targetH, fineGridState.h)
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

  if (not wantsRefine and not wantsErase) {
    return;
  }

  std::array<bool, TWO_POWER_D> isLocalFineVertex{};
  UNROLL(DIMENSIONS)
  for (int i = 0; i < TWO_POWER_D; ++i) {
    isLocalFineVertex[i] = SpacetreeHelper::isVertexAdjacentToLocalSpacetree(
      fineGridVertices[i],
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

  auto isTopologyStable = [](GridVertex::State s) {
    switch (s) {
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

  // Avoid changing topology while any of the surrounding coarse vertices are in transitional states.
  bool mayChangeGrid = true;
  UNROLL(DIMENSIONS)
  for (int i = 0; i < TWO_POWER_D; i++) {
    if (not isTopologyStable(coarseGridVertices[i].getState())) {
      mayChangeGrid = false;
      break;
    }
  }

  if (wantsRefine) {
    UNROLL(DIMENSIONS)
    for (int i = 0; i < TWO_POWER_D; ++i) {
      auto& v = fineGridVertices[i];
      if (not isLocalFineVertex[i]) {
        continue;
      }

      v.setFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep, true);

      const auto s = v.getState();
      if (s == GridVertex::State::EraseTriggered) {
        // Cancel pending erase if an adjacent cell wants to keep/refine.
        v.setState(GridVertex::State::Refined);
      } else if (mayChangeGrid and s == GridVertex::State::Unrefined) {
        v.setState(GridVertex::State::RefinementTriggered);
      }
    }
  } else { // wantsErase
    UNROLL(DIMENSIONS)
    for (int i = 0; i < TWO_POWER_D; ++i) {
      auto& v = fineGridVertices[i];
      if (not isLocalFineVertex[i]) {
        continue;
      }

      if (mayChangeGrid and v.getState() == GridVertex::State::Refined
          and not v.getFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep)) {
        v.setState(GridVertex::State::EraseTriggered);
      }
    }
  }
}

/**
 * ## Veto erases
 *
 * The routine markVerticesAroundForkedCell() is used in this routine
 * to ensure that we don't erase the grid over a worker domain. We have to do
 * this both on the master and the worker. The worker invokes the veto
 * coarsening when it comes from a coarse remote tree and runs into local fine
 * grid cell. The master does it the other way round: It sets the vetos if
 * we hit a remote cell out from the local tree. In both cases, the bottom-up
 * analysis of the flag ensures that the information propagates all the way up
 * the grid hierarchies.
 *
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
  for (int d = 0; d < DIMENSIONS; d++) {
    assertion(cellIndexFromState(state, _root.x)(d) == cellIndex(d), d, cellIndex, state.toString(), _id);
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

  peano4::LoopDirection loopDirection = PeanoCurve::getLoopDirection(state);
  peano4::zForEach3(loopDirection, [&](const tarch::la::Vector<DIMENSIONS, int>& k) {
    traceEnter("Grid bookkeeping before enter");
    const auto kLinearised       = peano4::dLinearised(k, 3);
    const auto fineGridCellIndex = childCellIndex(cellIndex, k);

    VertexType vertexTypes[TWO_POWER_D];
    getVertexTypes(vertices, k, vertexTypes);

    GridVertex fineGridVertices[TWO_POWER_D];
    loadVertices(fineGridStates[kLinearised], fineGridCellIndex, vertices, fineGridVertices, vertexTypes, k);

    const auto& lut = *_vertexLookupTables.back();

    FaceType faceTypes[TWO_TIMES_D];
    for (int i = 0; i < TWO_TIMES_D; ++i) {
      faceTypes[i] = getFaceType(vertexTypes, lut.faceIndex[i]);
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
      evaluateGridControlEvents(fineGridStates[kLinearised], vertices, fineGridVertices);
    }

    if (hasTriggeredSplits) {
      splitCellTopDown(vertices, fineGridVertices);
    }

    GridTraversalEvent enterNodeGridTraversalEvent = _gridTraversalEventGenerator.createEnterNodeTraversalEvent(
      vertices,
      fineGridVertices,
      vertexTypes,
      faceTypes,
      fineGridStates[kLinearised],
      _splitTriggered,
      _splitting,
      _joinTriggered,
      _joining,
      _hasSplit,
      k,
      lut,
      _noParallelTopologyChanges,
      spacetreeStateIsRunning
    );
    traceLeave("Grid bookkeeping before enter");

    observer.loadNodeData(enterNodeGridTraversalEvent);

    if (canReceiveUserData) {
      traceEnter("Observer data receive");
      receiveAndMergeUserData(
        fineGridStates[kLinearised],
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

    //
    // DFS
    //
    assertion(isRefined == SpacetreeHelper::isSpacetreeNodeRefined(fineGridVertices));
    if (isRefined) {
      if (isFineLocal) {
        _statistics.numberOfLocalRefinedCells++;
        incrementNumberOfAdjacentRefinedLocalCells(fineGridVertices);
      } else {
        _statistics.numberOfRemoteRefinedCells++;
      }
      descend(fineGridStates[kLinearised], fineGridCellIndex, fineGridVertices, observer);
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
        int currentLevel     = fineGridStates[kLinearised].level;
        _statistics.minDepth = std::min(_statistics.minDepth, currentLevel);
        _statistics.maxDepth = std::max(_statistics.maxDepth, currentLevel);
      } else {
        _statistics.numberOfRemoteUnrefinedCells++;
      }
    }

    traceEnter("Grid bookkeeping after child");
    GridTraversalEvent leaveNodeGridTraversalEvent = _gridTraversalEventGenerator.createLeaveNodeTraversalEvent(
      enterNodeGridTraversalEvent,
      vertices,
      fineGridVertices,
      vertexTypes,
      faceTypes,
      fineGridStates[kLinearised],
      _splitTriggered,
      _splitting,
      _joinTriggered,
      _joining,
      _hasSplit,
      k,
      lut,
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
        fineGridStates[kLinearised],
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

    storeVertices(fineGridCellIndex, vertices, fineGridVertices, vertexTypes, k);
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

  for (int i = 0; i < TWO_POWER_D; i++) {
    int inVertexStack              = enterCellTraversalEvent.getVertexDataFrom(i);
    int inVertexPositionWithinCell = enterCellTraversalEvent.getVertexDataTo(i);

    const bool isHangingVertex = enterCellTraversalEvent.getIsVertexHanging(inVertexPositionWithinCell);
    if (StackTopology::isInOutStack(inVertexStack) and not isHangingVertex) {
      if (enterCellTraversalEvent.getIsVertexLocal(inVertexPositionWithinCell)) {
        const auto receivingNeighbourTrees = SpacetreeHelper::getNeighbourTreesAlongVertex(
          fineGridVertices[inVertexPositionWithinCell],
          _id,
          _splitTriggered,
          _splitting,
          _joining,
          true,
          _spacetreeState,
          _noParallelTopologyChanges
        );
        for (int n = 0; n < receivingNeighbourTrees.count; ++n) {
          const auto p = receivingNeighbourTrees.ids[n];
          logDebug(
            "Receive neighbour data from neighbour " << p << " for vertex "
                                                     << fineGridVertices[inVertexPositionWithinCell].toString()
          );
          const auto slot = getOrCreateNeighbourSlot(p);
          assertion(slot >= 0, _id, p, inVertexPositionWithinCell, toString());
          observer.receiveNodeData(
            GridTraversalObserver::NodeData::Vertex,
            inVertexPositionWithinCell,
            slot,
            stacks::SendReceiveContext::BoundaryExchange,
            enterCellTraversalEvent
          );
        }

        if (hasPeriodicBCs(fineGridVertices[inVertexPositionWithinCell].adjacentRanks)) {
          const auto outStacks = StackTopology::getOutputStacksForPeriodicBoundaryExchange(
            fineGridVertices[inVertexPositionWithinCell].adjacentRanks
          );
          const auto vertexIndex = vertexIndexOfCellVertex(cellIndex, inVertexPositionWithinCell);
          for (const auto& p : outStacks) {
            const auto stamp     = makeVertexStamp(state.level, vertexIndex, std::bitset<DIMENSIONS>(p.second));
            const auto fromStack = StackTopology::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(p.first);
            observer.receiveNodeData(
              GridTraversalObserver::NodeData::Vertex,
              inVertexPositionWithinCell,
              fromStack,
              stacks::SendReceiveContext::PeriodicBoundaryDataSwap,
              enterCellTraversalEvent,
              &stamp
            );
          }
        }
      }
    }
  }

  for (int i = 0; i < TWO_TIMES_D; i++) {
    int inFaceStack              = enterCellTraversalEvent.getFaceDataFrom(i);
    int inFacePositionWithinCell = enterCellTraversalEvent.getFaceDataTo(i);

    const bool isHangingFace = enterCellTraversalEvent.getIsFaceHanging(inFacePositionWithinCell);
    if (StackTopology::isInOutStack(inFaceStack) and not isHangingFace) {
      if (enterCellTraversalEvent.getIsFaceLocal(inFacePositionWithinCell)) {
        const int receivingNeighbourTree = SpacetreeHelper::getNeighbourTreeAlongFace(
          fineGridVertices,
          inFacePositionWithinCell,
          _id,
          _splitTriggered,
          _splitting,
          _joining,
          true,
          _spacetreeState,
          _noParallelTopologyChanges
        );
        if (receivingNeighbourTree >= 0) {
          const auto slot = getOrCreateNeighbourSlot(receivingNeighbourTree);
          assertion(slot >= 0, _id, receivingNeighbourTree, inFacePositionWithinCell, toString());
          observer.receiveNodeData(
            GridTraversalObserver::NodeData::Face,
            inFacePositionWithinCell,
            slot,
            stacks::SendReceiveContext::BoundaryExchange,
            enterCellTraversalEvent
          );
        }

        if (isFaceAlongPeriodicBoundaryCondition(fineGridVertices, inFacePositionWithinCell, true)) {
          const auto stamp = makeFaceStamp(state.level, cellIndex, inFacePositionWithinCell);
          _periodicConsumerDescriptors.push_back({stamp, _id});
          const int outStack  = StackTopology::getOutputStackForPeriodicBoundaryExchange(inFacePositionWithinCell);
          const int fromStack = StackTopology::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(outStack);
          logDebug("Will merge face " << stamp.toString() << " from periodic BC stack #" << fromStack);

          observer.receiveNodeData(
            GridTraversalObserver::NodeData::Face,
            inFacePositionWithinCell,
            fromStack,
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
 * Send user data
 *
 * Send out data along the MPI boundary (horizontal) and stream data to
 * new workers during splitting (vertical). descend() calls this routine after
 * leaveNode() and before storeNodeData(). sendNodeData() therefore reads
 * the live call-stack data and has to enqueue exchange data without
 * consuming this source object. The subsequent storeNodeData() still has
 * to persist the same live data.
 *
 *
 * ## Data order on the call stack
 *
 * When we send out data, we read it from the current call-stack block.
 * As the call-stack data structure is a stack, we have to be careful:
 * If a cell carries N pieces of data and we want to enqueue the first
 * piece of data for exchange, then we have to pick the Nth entry from the
 * call-stack block. With our modified top(), we can access these
 * elements directly. We just have to be careful that top() equals
 * actually a top(0), so picking the Nth element requires us to call
 * top(N-1).
 *
 * The enqueue operation is not necessarily a deep copy. Future
 * handle-backed SoA data, for example GPU memory-pool allocations, should
 * enqueue a non-consuming handle or pinned view so shared-memory exchange
 * does not trigger a CPU-GPU payload transfer.
 *
 *
 * ## Masking and selecting transfer data
 *
 * The routine here does not realise the actual data transfer. It delegates
 * the exchange-stack population to the observer's sendNodeData(). As
 * observers are provided by the user - most frequently generated through
 * the Python API - we know which user data we have tied to the mesh and
 * hence can champion the data transfer.
 *
 * This separation of concerns implies that sendNodeData() also can
 * decide to mask out sent data. However, we have to be careful with this
 * masking: We want to be able to mask out normal boundary or periodic
 * exchange, but split and join contexts are copy contexts. They must not be
 * suppressed by solver-specific exchange guards.
 *
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

  for (int i = 0; i < TWO_POWER_D; i++) {
    int outVertexStack              = leaveCellTraversalEvent.getVertexDataTo(i);
    int outVertexPositionWithinCell = leaveCellTraversalEvent.getVertexDataFrom(i);

    const bool isHangingVertex = leaveCellTraversalEvent.getIsVertexHanging(outVertexPositionWithinCell);
    if (StackTopology::isInOutStack(outVertexStack) and not isHangingVertex) {
      if (leaveCellTraversalEvent.getIsVertexLocal(outVertexPositionWithinCell)) {
        const auto outgoingNeighbourTrees = SpacetreeHelper::getNeighbourTreesAlongVertex(
          fineGridVertices[outVertexPositionWithinCell],
          _id,
          _splitTriggered,
          _splitting,
          _joining,
          false,
          _spacetreeState,
          _noParallelTopologyChanges
        );

        for (int n = 0; n < outgoingNeighbourTrees.count; ++n) {
          const auto p = outgoingNeighbourTrees.ids[n];
          logDebug(
            "Send local vertex data of " << fineGridVertices[outVertexPositionWithinCell].toString() << " from stack "
                                         << outVertexStack << " on tree " << _id << " to neighbour " << p
                                         << ". Position within cell=" << outVertexPositionWithinCell
          );
          const auto slot = getOrCreateNeighbourSlot(p);
          assertion(slot >= 0, _id, p, outVertexPositionWithinCell, toString());
          observer.sendNodeData(
            GridTraversalObserver::NodeData::Vertex,
            outVertexPositionWithinCell,
            slot,
            stacks::SendReceiveContext::BoundaryExchange,
            leaveCellTraversalEvent
          );
        }

        if (hasPeriodicBCs(fineGridVertices[outVertexPositionWithinCell].adjacentRanks)) {
          const auto outStacks = StackTopology::getOutputStacksForPeriodicBoundaryExchange(
            fineGridVertices[outVertexPositionWithinCell].adjacentRanks
          );
          const auto vertexIndex = vertexIndexOfCellVertex(cellIndex, outVertexPositionWithinCell);
          for (const auto& stackNo : outStacks) {
            const auto stamp = makeVertexStamp(state.level, vertexIndex, std::bitset<DIMENSIONS>(stackNo.second));
            logDebug(
              "Vertex " << fineGridVertices[outVertexPositionWithinCell].toString() << " on tree " << _id
                        << " goes to stack " << stackNo.first << " as periodic image " << stamp.toString()
            );

            observer.sendNodeData(
              GridTraversalObserver::NodeData::Vertex,
              outVertexPositionWithinCell,
              stackNo.first,
              stacks::SendReceiveContext::PeriodicBoundaryDataSwap,
              leaveCellTraversalEvent,
              &stamp
            );
          }
        }

        for (const auto& p : _splitting) {
          const auto& adjacentRanks = fineGridVertices[outVertexPositionWithinCell].backupOfAdjacentRanks;
          if (std::find(std::begin(adjacentRanks), std::end(adjacentRanks), p) != std::end(adjacentRanks)) {
            logDebug(
              "Stream local vertex data of "
              << fineGridVertices[outVertexPositionWithinCell].toString() << " from stack " << outVertexStack
              << " on tree " << _id << " to upcoming worker " << p << ". Position within cell="
              << outVertexPositionWithinCell << ". event=" << leaveCellTraversalEvent.toString()
            );
            const auto slot = getOrCreateNeighbourSlot(p);
            assertion(slot >= 0, _id, p, outVertexPositionWithinCell, toString());
            observer.sendNodeData(
              GridTraversalObserver::NodeData::Vertex,
              outVertexPositionWithinCell,
              slot,
              stacks::SendReceiveContext::ForkDomain,
              leaveCellTraversalEvent
            );
          } else {
            logDebug(
              "Skip vertex data of " << fineGridVertices[outVertexPositionWithinCell].toString() << " on tree " << _id
            );
          }
        }
      }
    }
  }

  for (int i = 0; i < TWO_TIMES_D; i++) {
    int outFaceStack              = leaveCellTraversalEvent.getFaceDataTo(i);
    int outFacePositionWithinCell = leaveCellTraversalEvent.getFaceDataFrom(i);

    const bool isHangingFace = leaveCellTraversalEvent.getIsFaceHanging(outFacePositionWithinCell);
    if (StackTopology::isInOutStack(outFaceStack) and not isHangingFace) {
      if (leaveCellTraversalEvent.getIsFaceLocal(outFacePositionWithinCell)) {
        const int outgoingNeighbourTree = SpacetreeHelper::getNeighbourTreeAlongFace(
          fineGridVertices,
          outFacePositionWithinCell,
          _id,
          _splitTriggered,
          _splitting,
          _joining,
          false,
          _spacetreeState,
          _noParallelTopologyChanges
        );
        if (outgoingNeighbourTree >= 0) {
          logDebug(
            "Send local face from stack " << outFaceStack << " of tree " << _id << " to neighbour "
                                          << outgoingNeighbourTree
                                          << ". Position within cell=" << outFacePositionWithinCell
          );

          const auto slot = getOrCreateNeighbourSlot(outgoingNeighbourTree);
          assertion(slot >= 0, _id, outgoingNeighbourTree, outFacePositionWithinCell, toString());
          observer.sendNodeData(
            GridTraversalObserver::NodeData::Face,
            outFacePositionWithinCell,
            slot,
            stacks::SendReceiveContext::BoundaryExchange,
            leaveCellTraversalEvent
          );
        }

        if (isFaceAlongPeriodicBoundaryCondition(fineGridVertices, outFacePositionWithinCell, false)) {
          const auto stamp = makeFaceStamp(state.level, cellIndex, outFacePositionWithinCell);
          _periodicProducerDescriptors.push_back({stamp, _id});

          const int toStack = StackTopology::getOutputStackForPeriodicBoundaryExchange(outFacePositionWithinCell);

          logDebug("Send face " << stamp.toString() << " of tree " << _id << " to periodic BC stack #" << toStack);

          observer.sendNodeData(
            GridTraversalObserver::NodeData::Face,
            outFacePositionWithinCell,
            toStack,
            stacks::SendReceiveContext::PeriodicBoundaryDataSwap,
            leaveCellTraversalEvent,
            &stamp
          );
        }
      }

      int adjacentRanks[TWO_POWER_D];
      SpacetreeHelper::getAdjacentRanksOfFace(fineGridVertices, outFacePositionWithinCell, false, adjacentRanks);
      for (const auto& p : _splitting) {
        if (std::find(std::begin(adjacentRanks), std::end(adjacentRanks), p) != std::end(adjacentRanks)) {
          logDebug(
            "Stream local face from stack " << outFaceStack << " of tree " << _id << " to new worker " << p
                                            << ". Position within cell=" << outFacePositionWithinCell
          );

          const auto slot = getOrCreateNeighbourSlot(p);
          assertion(slot >= 0, _id, p, outFacePositionWithinCell, toString());
          observer.sendNodeData(
            GridTraversalObserver::NodeData::Face,
            outFacePositionWithinCell,
            slot,
            stacks::SendReceiveContext::ForkDomain,
            leaveCellTraversalEvent
          );
        }
      }
    }
  }

  for (const auto& p : _splitting) {
    if (SpacetreeHelper::getTreeOwningSpacetreeNode(
          fineGridVertices,
          _id,
          _splitTriggered,
          _splitting,
          _joinTriggered,
          _joining
        )
        == p) {
      logDebug(
        "Stream local cell of tree " << _id << " to new worker " << p
                                     << ". Left bottom vertex=" << fineGridVertices[0].toString()
      );

      const auto slot = getOrCreateNeighbourSlot(p);
      assertion(slot >= 0, _id, p, 0, toString());
      observer.sendNodeData(
        GridTraversalObserver::NodeData::Cell,
        0,
        slot,
        stacks::SendReceiveContext::ForkDomain,
        leaveCellTraversalEvent
      );
    }
  }
}

/**
 * Realise the splits and joins
 *
 * This routine alters the vertex adjacency information. That is, it does
 * not yet really split or join. It identifies where to split and join.
 * The adjacency information will be exchanged with (potential) neighbours
 * after this grid traversal and the actual splitting or joining then
 * happens in the next grid sweep. That is, this routine realises the
 * SplitTriggered or JoinTriggered phase. See the discussion in
 * @ref peano_domain_decomposition "Peano's domain decomposition"
 * documentation for a discussion of these subsequent phases.
 *
 * On the split side, we have two types of splits: Splits that are
 * identified bottom-up. These splits are realised carefully and never
 * split off too many cells. The other splits are top-down splits and
 * they are more aggressive, i.e. they might split off too many cells
 * eventually. What type of split you want to have is controlled by a
 * flag within the split specification when you call split().
 *
 *
 * ## Bottom-up splits
 *
 * The split logic is protected by one big check if splits are active.
 * If no splits are triggered, there is no need to check things further.
 * Any further logic is guided by the fact if a spacetree node is refined
 * and if it is a potential split candidate. The latter is determined by
 * evaluating SpacetreeHelper::isSpacetreeNodeSplitCandidate().
 *
 * Our logic is that we focus first of all on the fine grid cells: We aim
 * to split a certain number of fine grid cells into a new tree. If all
 * children of a refined node are split into the same new tree, and if this
 * (parent) node is a potential split candidate, too, then we move it to
 * the newly created tree as well.
 *
 * We realise an @f$ LR(3^d) @f$ grammar to identify the latter situation.
 * This grammar uses the container (stack) _splittedCells over integers.
 *
 * - Every leaf pushes a marker into this container: _id if it is and
 *   remains local, the new id if the node has been split, and a -1
 *   otherwise.
 * - A refined cell induces a reduce on the container. @f$ 3^d@f$ markers
 *   are removed and analysed: If they all have the same id as the next
 *   split marker to be used, we add it.
 * - A refined cell finally adds its marker again.
 *
 * It is important that I evaluate the reduce analysis even if no splits
 * are do be done anymore. Because it might happen that I have just done
 * exactly 9 splits for example (2d) and thus, the parent of these 9
 * guys should go to the remote node, too. The punchline is that only
 * leaves check if they have to fork. The behaviour of refined octants
 * follows the decisions made for their children before.
 *
 *
 * ## Top-down splits
 *
 * The down splits are discussed in splitCellTopDown(). This is where all
 * the logic sits. The present routine merely impleemnts the decisions
 * made there.
 *
 *
 * ## Rationale
 *
 * I originally realised the splits solely bottom-up, i.e. decided to
 * omit any analysis if the coarse node has already been split. This way, it is
 * very easy to accommodate Peano's @ref peano_domain_decomposition "two fundamental splitting constraints":
 * We simply veto the split-off of any cell with a hanging node. However,
 * such an approach leads to the situation where adaptive mesh refinements
 * severely constrain the algorithm. In the example in the general
 * discussion
 *
 * @image html ../../../documentation/Doxygen/domain-decomposition/regularity-constraint.png
 *
 * we would not split off any of the cells that are adjacent to the
 * AMR boundary. As we never split off a cell unless all of its
 * children are of the same rank, we would never ever split off the
 * green cell. This is bad in a situation where only splitting off the
 * green cell and all of its children would yield a good domain
 * decomposition. So a sole bottom-up approach is not an option.
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
      "Checked cell for bottom-up splits (first target tree=" << _splitTriggered.begin()->first << "). IsRefined="
                                                              << isRefined << ", IsSplitCandidate=" << isSplitCandidate
                                                              << " / " << fineGridVertices[0].toString()
    );

    if (isSplitCandidate and isRefined) {
      int reducedMarker = _splittedCells.back();
      for (int i = 0; i < THREE_POWER_D; i++) {
        int topMarker = _splittedCells.back();
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
      for (int i = 0; i < THREE_POWER_D; i++) {
        _splittedCells.pop_back();
      }
      _splittedCells.push_back(NoSplit);
    } else if (isSplitCandidate and not isRefined) {
      int targetSpacetreeId = getSplittingTree();
      if (targetSpacetreeId >= 0) {
        updateVertexRanksWithinCell(fineGridVertices, targetSpacetreeId);
        updateSplittingCounter(targetSpacetreeId);
      }
      _splittedCells.push_back(targetSpacetreeId);
    } else if (not isSplitCandidate and not isRefined) {
      _splittedCells.push_back(NoSplit);
    } else {
      assertion(false, "Should not be entered as all combination are covered already");
    }
  } else if (not _splitTriggered.empty()
             and _splitTriggered.begin()->second.mode == SplitInstruction::Mode::AggressiveTopDown
             and not _splittedCells.empty()) {
    int targetSpacetreeId = _splittedCells.back();
    _splittedCells.pop_back();

    logInfo("Checked cell for top-down splits (first split " << _splitTriggered.begin()->first << ")");
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
    updateVertexRanksWithinCell(fineGridVertices, targetSpacetreeId);
  }
}

/**
 * Split cell in a top down fashion
 *
 * The algorithm logic behind this splitting strategy is very simple: We
 * first find out what the next split-off tree would be.
 *
 * - If a cell's parent is deployed to this tree, deploy the current octant
 *   too. After all, we want to split in a top-down manner.
 * - If an octant's parent is not yet deployed but the cell would be a
 *   split candidate, we trigger the split.
 * - If we deploy an unrefined octant to a new tree, we have to decrease
 *   the internal counter by calling updateSplittingCounter().
 *
 * Unfortunately, the actual splitting is not that simple: We cannot alter
 * the adjacency information of a vertex on-the-fly without running risk to
 * change the vertex ownership. Therefore, we only realise the analysis
 * here and deploy the actual split realisation to splitOrJoinCellBottomUp().
 * Analysis means that we push a marker on the helper stack
 * _splittedCells whenever we want to split. And we update our internal
 * splitting counters. But we do not realise the split itself, i.e. we do
 * not alter adjacency information.
 *
 * @see splitOrJoinCellBottomUp()
 */
void peano4::grid::Spacetree::splitCellTopDown(
  GridVertex coarseGridVertices[TWO_POWER_D],
  GridVertex fineGridVertices[TWO_POWER_D]
) {
  [[maybe_unused]] constexpr int NoSplit = -1;

  if (not _splitTriggered.empty()
      and _splitTriggered.begin()->second.mode == SplitInstruction::Mode::AggressiveTopDown) {
    const bool isParentSplit     = not _splittedCells.empty();
    const int  targetSpacetreeId = getSplittingTree();
    const bool isSplitCandidate  = targetSpacetreeId >= 0
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
      int parentId = _splittedCells.back();
      _splittedCells.push_back(parentId);
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
      updateSplittingCounter(targetSpacetreeId);
      int parentId = _splittedCells.back();
      _splittedCells.push_back(parentId);
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
        "Refined cell at " << fineGridVertices[0].toString() << " should go with all children to tree "
                           << targetSpacetreeId
      );
      _splittedCells.push_back(targetSpacetreeId);
    } else if (not isParentSplit and isSplitCandidate and not isRefined) {
      updateSplittingCounter(targetSpacetreeId);
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
      _splittedCells.push_back(targetSpacetreeId);
    }
    logDebug("#SplittedCells=" << _splittedCells.size());
  }
}

/**
 * Merge data from worker with master data throughout join
 *
 * ## Join process
 *
 * If we receive a remote cell, then this cell is not refined. In this case
 * the (former) workers holds all the valid adjacency data: We might on a
 * master receive a cell right from the middle of the worker's domain where
 * the master has no clue about any adjacency. So we might think that we can
 * copy over the (former) worker's adjacency data.
 *
 * As we also receive hanging vertices from the worker, we can safely (and
 * pretty naively) copy over the adjacency. THe first non-hanging vertex
 * will bring in the right adjacency information.
 *
 * There is however one tricky special case. Assume that a rank A serves as
 * master to B and C and both of these workers merge into A. We furthermore
 * study one vertex in-between B and C. The merge runs as follows:
 *
 * - Rank B tells rank C that it will merge its cell into A.
 * - Rank C tells rank B that it will merge its cell into A.
 * - Both ranks update their respective local adjacency lists.
 * - Rank B streams its vertex up to rank A.
 *
 * If we simply copied the adjacency list from B, we would lose the
 * information that a cell is adjacent to C. B has already updated its list,
 * so it is slightly "ahead" of time.
 *
 * So for a successful merge, it is important that we actually only reset
 * the flags of this very cell. We do so through
 * updateVertexRanksWithinCell(). This logic relies on the fact that we
 * keep adjacency flags of all vertices which are remote yet just one level
 * below the current rank. This check, realised in updateVertexBeforeStore(),
 * ensures we kind of know what we do.
 *
 *
 * <h2> Joins </h2>
 *
 * If we join (parts of) a worker into the local partition, it can happen
 * that the rank receives a vertex that was remote before. As a
 * consequence, the local copy of a vertex holds invalid data whereas the
 * incoming data holds up-to-date adjacency information. So if a vertex is
 * remote locally, we take the worker's vertex to deliver accurate adjacency
 * data. The only alteration we make is that we replace the
 * RankOfCellWitchWillBeJoined markers with the rank of the worker.
 * RankOfCellWitchWillBeJoined is used to mark those cells which go to the
 * worker in the join triggered phase, i.e. it is used only on the worker
 * and only to prepare a join.
 *
 *
 * @see updateVertexBeforeStore()
 */
void peano4::grid::Spacetree::mergeCellFromWorkerWithMasterThroughoutJoin(
  GridVertex coarseGridVertices[TWO_POWER_D],
  GridVertex fineGridVertices[TWO_POWER_D]
) {
  for (const auto& worker : _joining) {
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
          == worker) {
      logDebug("Cell from worker " << worker << " is merged into master " << _id);
      updateVertexRanksWithinCell(fineGridVertices, _id);
    } else {
      for (int i = 0; i < TWO_POWER_D; i++) {
        logDebug("- " << coarseGridVertices[i].toString());
      }
      for (int i = 0; i < TWO_POWER_D; i++) {
        logDebug("- " << fineGridVertices[i].toString());
      }
    }
  }
}

/**
 * Reduce splitting counter
 *
 * Every tree in the list of trees for which we have to trigger a split
 * has an integer counter which highlights how many cells still to offload
 * to the newly created tree. Decrement this counter.
 *
 * However, do not remove the entry from the split triggered set. We need
 * all of these indices to know in the next sweep which child trees are now
 * in the state splitting i.e. transition from split triggered into
 * splitting.
 *
 * If we have top-down splits, we might split off two many cells. In this
 * case, ensure that the internal counter does not underrun zero.
 */
void peano4::grid::Spacetree::updateSplittingCounter(int treeId) {
  auto it = _splitTriggered.find(treeId);
  if (it != _splitTriggered.end()) {
    it->second.numberOfFineGridCells = std::max(it->second.numberOfFineGridCells - 1, 0);
  }
}

/**
 * @return Id of splitting tree or -1 if there's none.
 */
int peano4::grid::Spacetree::getSplittingTree() const {
  for (auto& p : _splitTriggered) {
    if (p.second.numberOfFineGridCells > 0) {
      return p.first;
    }
  }
  return -1;
}

const peano4::grid::GridStatistics& peano4::grid::Spacetree::getGridStatistics() const { return _statistics; }

int peano4::grid::Spacetree::getId() const { return _id; }

/**
 * Master tree of this tree, or -1 for a root tree.
 */
int peano4::grid::Spacetree::getMasterId() const { return _masterId; }

/**
 * setIsAntecessorOfRefinedVertexInCurrentTreeSweep()
 *
 * If a cell is given away to another rank, we have to mark the vertices
 * of its mother cell such that we do not coarsen above it and thus
 * effectively remove our whole splitted subpartition. We do this by two
 * means: First, we set the marker
 * setIsAntecessorOfRefinedVertexInCurrentTreeSweep(). We erase at most
 * one level at a time in Peano. Therefore, this flag effectively vetoes
 * any erase call. This flag ensures that updateVertexAfterLoad() in the
 * next grid sweep will erase the mesh.
 *
 * We do so on both the master and the worker. On the master cell, i.e.
 * the one that forks another cell off, it is clear that we have to mark
 * the vertices, as the vertex information ("hey, I have forked off a finer
 * grid cell and you therefore shall not erase the mesh here") has to
 * propagate immediately. We also set it on the worker, as its vertices
 * will be subject to boundary data exchange. This way, we inform
 * neighbours about the topology, too.
 */
void peano4::grid::Spacetree::markVerticesAroundForkedCell(
  GridVertex coarseGridVertices[TWO_POWER_D],
  GridVertex fineGridVertices[TWO_POWER_D]
) const {
  peano4::dForEachCorner([&](const tarch::la::Vector<DIMENSIONS, int>& k, int kScalar) {
    assertion(
      coarseGridVertices[kScalar].getState() != GridVertex::State::HangingVertex,
      coarseGridVertices[kScalar].toString()
    );
    coarseGridVertices[kScalar].setFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep, true);
    coarseGridVertices[kScalar].setFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep, true);
    fineGridVertices[kScalar].setFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep, true);
  });
}

/**
 * Add a split instruction
 *
 * Add a new split instruction. Peano's documentation
 * @ref peano_domain_decomposition "discusses splits in detail", and also
 * clarifies why we need two different approaches to split (see
 * SplitInstruction).
 *
 * If we add a split, we should commit to one split type: Either all of
 * our splits are top-down or all of them are bottom-up. If we had both
 * types in one grid sweep, we could run into inconsistent grid
 * configurations. At the moment, this is an assumption, i.e. it might
 * work, but I'm just not sure if it really holds.
 *
 * This constrains (only one type of splitting) is not necessary
 * algorithmically. The logic would support a sequence of different
 * split types. However, both splits use the same helper data structures,
 * and they might be messed up with different split types are combined.
 */
void peano4::grid::Spacetree::split(int newSpacetreeId, const SplitInstruction& instruction) {
  assertion(
    instruction.numberOfFineGridCells == std::numeric_limits<int>::max() or maySplit(),
    newSpacetreeId,
    instruction,
    toString()
  );
  assertion(
    instruction.numberOfFineGridCells != std::numeric_limits<int>::max(),
    newSpacetreeId,
    instruction,
    toString()
  );
  assertion(instruction.numberOfFineGridCells >= 1, newSpacetreeId, instruction, toString());

  _splitTriggered.insert(std::pair<int, SplitInstruction>(newSpacetreeId, instruction));

  auto firstMode = _splitTriggered.begin()->second.mode;
  for (auto& p : _splitTriggered) {
    if (p.second.mode != firstMode) {
      logWarning(
        "Split of tree " << p.first << " is of type " << p.second.mode << " while other splits have mode " << firstMode
                         << ". Switch all splits to " << firstMode
      );
      p.second.mode = firstMode;
    }
  }
}

std::string peano4::grid::Spacetree::toString() const {
  std::ostringstream msg;
  msg << "Spacetree(Id=" << _id << ", MasterId=" << _masterId << ", State=" << peano4::grid::toString(_spacetreeState)
      << ", Statistics=" << _statistics;

  if (not _joinTriggered.empty()) {
    msg << ", JoinTriggered={";
    for (const auto& p : _joinTriggered) {
      msg << "(" << p << ")";
    }
    msg << "}";
  }
  if (not _joining.empty()) {
    msg << ", Joining={";
    for (const auto& p : _joining) {
      msg << "(" << p << ")";
    }
    msg << "}";
  }

  if (not _splitTriggered.empty()) {
    msg << ", SplitTriggered={";
    for (const auto& p : _splitTriggered) {
      msg << "(" << p.first << "," << p.second << ")";
    }
    msg << "}";
  }
  if (not _splitting.empty()) {
    msg << ", Splitting={";
    for (const auto& p : _splitting) {
      msg << "(" << p << ")";
    }
    msg << "}";
  }
  msg << ", NeighbourTrees=" << _neighbourTreeIds.size() << ")";
  return msg.str();
}

/**
 * Is the tree in principle allowed to split
 *
 * A tree cannot split if it is brand new or currently involved in some
 * load balancing. Even if a tree is allowed to split, it does not mean
 * that any split might be successful. A tree might for example be
 * stationary yet host so few cells that it cannot split anymore without
 * violating @ref peano_domain_decomposition "Peano's domain decomposition constraints".
 * This per-cell decision is made by
 * peano4::grid::SpacetreeHelper::isSpacetreeNodeSplitCandidate().
 */
bool peano4::grid::Spacetree::maySplit() const {
  return _spacetreeState == SpacetreeState::Running and _joinTriggered.empty() and _joining.empty()
    and _splitting.empty() and _statistics.numberOfLocalUnrefinedCells > 0
    and (_splitTriggered.empty()
         or _splitTriggered.begin()->second.numberOfFineGridCells < std::numeric_limits<int>::max());
}

/**
 * We allow at most one join at a time and not while we split
 */
bool peano4::grid::Spacetree::mayJoinWithWorker() const {
  return _spacetreeState == SpacetreeState::Running and _splitTriggered.empty() and _joining.empty()
    and _splitting.empty() and _joinTriggered.empty();
}

/**
 * Only ranks that have no kids are allowed to join. This is
 * implicitly ensured as we have only degenerated trees here,
 * i.e. we only allow a tree to join if it does not host local
 * spacetree nodes. Furthermore, we require a tree to be
 * stationary, i.e. to linger around like that for a while.
 *
 * @see join()
 */
bool peano4::grid::Spacetree::mayJoinWithMaster() const {
  return _spacetreeState == SpacetreeState::Running and _masterId >= 0
    and _statistics.numberOfStationarySweeps > NumberOfStationarySweepsToWaitAtLeastTillJoin and _splitTriggered.empty()
    and _splitting.empty() and _joinTriggered.empty() and _joining.empty()
    and _statistics.numberOfLocalRefinedCells == 0;
}

/**
 * Join with master. Call this routine only for degenerated trees,
 * i.e. for trees without any refined local cells.
 *
 * We have to be careful with the merges. Often when we fork a tree,
 * this tree first is not refined further. We need a couple of sweeps
 * to see whether a tree is refined further or not.
 */
void peano4::grid::Spacetree::joinWithMaster() {
  assertion(mayJoinWithMaster(), _id);
  _spacetreeState = SpacetreeState::JoinTriggered;
  assertion(_splitTriggered.empty());
  assertion(_splitting.empty());
  assertion(_splittedCells.empty());
  assertion(_joining.empty());
}

void peano4::grid::Spacetree::joinWithWorker(int id) {
  logInfo("Merge tree " << id << " into tree " << _id);
  assertion(_joinTriggered.count(id) == 0, id, _id);
  _joinTriggered.insert(id);
  assertion(_splitTriggered.empty(), id, toString());
  assertion(_splitting.empty(), id, toString());
}

bool peano4::grid::Spacetree::isInvolvedInJoinOrFork() const {
  return not _joinTriggered.empty() or not _joining.empty() or not _splitTriggered.empty() or not _splitting.empty();
}
