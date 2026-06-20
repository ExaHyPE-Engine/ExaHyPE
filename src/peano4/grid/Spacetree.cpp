// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Spacetree.h"

#include "GridControlEvent.h"
#include "GridTraversalEvent.h"
#include "PeanoCurve.h"
#include "GridTraversalObserver.h"
#include "StackTopology.h"
#include "VertexType.h"
#include "SpacetreeHelper.h"
#include "peano4/utils/Loop.h"
#include "tarch/mpi/Rank.h"

namespace {
  static constexpr auto RankOfPeriodicBoundaryCondition = peano4::grid::SpacetreeHelper::RankOfPeriodicBoundaryCondition;
  static constexpr auto NumberOfStationarySweepsToWaitAtLeastTillJoin = peano4::grid::SpacetreeHelper::NumberOfStationarySweepsToWaitAtLeastTillJoin;

  inline static bool hasPeriodicBCs(const std::array<int, TWO_POWER_D>& adj) {
    for (int i = 0; i < TWO_POWER_D; ++i) {
      if (adj[i] == RankOfPeriodicBoundaryCondition) {
        return true;
      }
    }
    return false;
  }
} // namespace

peano4::grid::Spacetree::Spacetree(
  const tarch::la::Vector<DIMENSIONS, double>& offset,
  const tarch::la::Vector<DIMENSIONS, double>& width,
  const std::bitset<DIMENSIONS>&               periodicBC
):
  _id(0),
  _spacetreeState(SpacetreeState::NewRoot),
  _root{},
  _statistics{},
  _masterId(-1),
  _periodicBC(periodicBC),
  _gridTraversalEventGenerator(0) {

  _root.x = offset;
  _root.h = width;
  _root.setInverted(false);

  for (int i = 0; i < TWO_TIMES_D; i++) {
    _root.accessNumber[i] = 0;
  }

  _statistics.numberOfStationarySweeps = 0;
  _statistics.clear(_id == 0);

  _noParallelTopologyChanges = true;

  logDebug("Created spacetree with " << offset << "x" << width << " and periodic BCs=" << _periodicBC);
}

peano4::grid::Spacetree::Spacetree(
  int                                          newId,
  int                                          masterId,
  const tarch::la::Vector<DIMENSIONS, double>& offset,
  const tarch::la::Vector<DIMENSIONS, double>& width,
  bool                                         traversalInverted
):
  _id(newId),
  _spacetreeState(SpacetreeState::EmptyRun),
  _root{},
  _statistics{},
  _masterId(masterId),
  _periodicBC(0),
  _splitTriggered(),
  _splitting(),
  _gridTraversalEventGenerator(newId) {

  _root.level = 0;
  _root.x = offset;
  _root.h = width;
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

peano4::stacks::Stack<peano4::grid::GridVertex>& peano4::grid::Spacetree::getLocalExchangeVertexStack(int localStackId) {
  assertion(localStackId >= 0 and localStackId < StackTopology::TotalNumberOfLocalExchangeStacks, localStackId, StackTopology::TotalNumberOfLocalExchangeStacks);
  return _localExchangeVertexStacks[localStackId];
}

const peano4::stacks::Stack<peano4::grid::GridVertex>& peano4::grid::Spacetree::getLocalExchangeVertexStack(int localStackId) const {
  assertion(localStackId >= 0 and localStackId < StackTopology::TotalNumberOfLocalExchangeStacks, localStackId, StackTopology::TotalNumberOfLocalExchangeStacks);
  return _localExchangeVertexStacks[localStackId];
}

peano4::stacks::Stack<peano4::grid::GridVertex>& peano4::grid::Spacetree::getParallelExchangeVertexStack(int slot, StackTopology::ParallelExchangeStack which) {
  assertion(slot >= 0 and slot < static_cast<int>(_parallelExchangeVertexStacks.size()), slot, static_cast<int>(_parallelExchangeVertexStacks.size()));
  return _parallelExchangeVertexStacks[slot][StackTopology::toIndex(which)];
}

const peano4::stacks::Stack<peano4::grid::GridVertex>& peano4::grid::Spacetree::getParallelExchangeVertexStack(int slot, StackTopology::ParallelExchangeStack which) const {
  assertion(slot >= 0 and slot < static_cast<int>(_parallelExchangeVertexStacks.size()), slot, static_cast<int>(_parallelExchangeVertexStacks.size()));
  return _parallelExchangeVertexStacks[slot][StackTopology::toIndex(which)];
}

int peano4::grid::Spacetree::getNumberOfNeighbourTrees() const {
  return static_cast<int>(_neighbourTreeIds.size());
}

int peano4::grid::Spacetree::getNeighbourTreeIdForSlot(int slot) const {
  assertion(slot >= 0 and slot < static_cast<int>(_neighbourTreeIds.size()));
  return _neighbourTreeIds[slot];
}

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
  logDebug(_splitTriggered.size() << " tree splits triggered and " << _splitting.size() << " splitting trees on tree " << _id);

  _statistics.clear(_id == 0);

  if (_spacetreeState == SpacetreeState::EmptyRun or _spacetreeState == SpacetreeState::NewFromSplit or _spacetreeState == SpacetreeState::Joining) {
    _gridControlEvents.clear();
  } else {
    _gridControlEvents = observer.getGridControlEvents();
    GridControlEvent::merge(_gridControlEvents);
    if (not _gridControlEvents.empty()) {
      logDebug("Spacetree " << _id << " got " << _gridControlEvents.size() << " grid control events");
    }
  }

  _splittedCells.clear();

  _noParallelTopologyChanges = _splitTriggered.empty()
      and _splitting.empty()
      and _joinTriggered.empty()
      and _joining.empty();

  const bool isFirstTraversal = _spacetreeState == SpacetreeState::NewRoot;

  GridVertex vertices[TWO_POWER_D];
  dfor2(k)
    std::array<int, TWO_POWER_D> adjacentRanks;
    adjacentRanks.fill(-1);
    adjacentRanks[TWO_POWER_D - 1 - kScalar] = 0;
    vertices[kScalar] = GridVertex::create(
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
  enddforx

  for (int d = 0; d < DIMENSIONS; d++) {
    if (_periodicBC[d]) {
      dfor2(vertex) dfor2(entries) tarch::la::Vector<DIMENSIONS, int> entry = entries;
      if (vertex(d) == 0) {
        entry(d) = 0;
      } else {
        entry(d) = 1;
      }
      vertices[vertexScalar].adjacentRanks[peano4::utils::dLinearised(entry, 2)] = RankOfPeriodicBoundaryCondition;
      vertices[vertexScalar].backupOfAdjacentRanks[peano4::utils::dLinearised(entry, 2)] = RankOfPeriodicBoundaryCondition;
      enddforx logDebug("Set periodic boundary conditions: " << vertices[vertexScalar].toString());
      enddforx
    }
  }

  observer.beginTraversal(_root.x, _root.h);
  descend(_root, vertices, observer);

  _hasSplit.clear();
  for (auto& rank : _splitting) {
    _childrenIds.insert(rank);
    _hasSplit.insert(rank);
  }
  _splitting.clear();
  for (auto& p : _splitTriggered) {
    if (p.second.numberOfFineGridCells > 0) {
      logWarning(
        "Have not been able to assign enough cells from "
          << _id << " to new tree " << p.first << " (should have deployed " << p.second << " more cells)"
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
  logDebug("Switched tree " << _id << " from " << grid::toString(oldState) << " into " << grid::toString(_spacetreeState));

  traceLeave();
}

void peano4::grid::Spacetree::receiveAndMergeGridVertexAtVerticalBoundary(GridVertex& vertex) {
  for (const auto& p : _joining) {
    const auto& adjacentRanks = vertex.adjacentRanks;
    if (std::find(std::begin(adjacentRanks), std::end(adjacentRanks), p) != std::end(adjacentRanks)) {
      const int slot = getOrCreateNeighbourSlot(p);
      logDebug("Merge " << vertex.toString() << " with its replica from worker as worker is joining. Incoming channel for worker=" << p);

      auto& in = _parallelExchangeVertexStacks[slot][StackTopology::toIndex(StackTopology::ParallelExchangeStack::VerticalInput)];
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

void peano4::grid::Spacetree::updateVertexAfterLoad(
  GridVertex&                               vertex,
  GridVertex                                fineGridVertices[TWO_POWER_D],
  const tarch::la::Vector<DIMENSIONS, int>& position,
  GridTraversalObserver&                    observer
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
    receiveAndMergeGridVertexAtHorizontalBoundary(vertex);
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
    const auto s = vertex.getState();
    const bool prevCoarseVertexTypeWasHanging =
      s == GridVertex::State::RefinementTriggered
      or s == GridVertex::State::Delete
      or s == GridVertex::State::New
      or s == GridVertex::State::Unrefined
      or s == GridVertex::State::HangingVertex;
    vertex.setFlag(GridVertex::HasBeenVertexTypeHangingInPreviousTreeSweep, prevCoarseVertexTypeWasHanging);

    // const auto s = vertex.getState();
    // const bool prevCoarseVertexTypeWasHanging =
    //   s == GridVertex::State::RefinementTriggered
    //   or s == GridVertex::State::Unrefined
    //   or s == GridVertex::State::HangingVertex;
    // vertex.setFlag(GridVertex::HasBeenVertexTypeHangingInPreviousTreeSweep, prevCoarseVertexTypeWasHanging);

    // const auto s = vertex.getState();
    // const bool prevCoarseVertexTypeWasHanging =
    //   (s == GridVertex::State::HangingVertex)
    //   or (s == GridVertex::State::Unrefined and vertex.getFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep));
    // vertex.setFlag(GridVertex::HasBeenVertexTypeHangingInPreviousTreeSweep, prevCoarseVertexTypeWasHanging);

    // const bool prevCoarseVertexTypeWasHanging = (vertex.getState() == GridVertex::State::HangingVertex);
    // vertex.setFlag(GridVertex::HasBeenVertexTypeHangingInPreviousTreeSweep, prevCoarseVertexTypeWasHanging);

    vertex.setFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep, vertex.getFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep));
    vertex.setFlag(GridVertex::HasBeenParentOfSubtreeVertexInPreviousTreeSweep, vertex.getFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep));

    vertex.clearFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep);
    vertex.clearFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep);

    vertex.numberOfAdjacentRefinedLocalCells = 0;

    if (vertex.getState() == GridVertex::State::RefinementTriggered) {
      if (vertexIsAdjacentToLocalTree) {
        logDebug("switch vertex to refining on tree " << _id << ": " << vertex.toString());
        vertex.setState(GridVertex::State::Refining);
        _statistics.numberOfStationarySweeps = 0;
      } else {
        vertex.setState(GridVertex::State::Unrefined);
        // Could be set by number of adjacent cells, e.g.
        // Anyway, don't reset stationary counter as this might
        // stop the tree from merging into its father
        //_statistics.numberOfStationarySweeps = 0;
      }
    } else if (vertex.getState() == GridVertex::State::EraseTriggered) {
      if (vertex.getFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep)) {
        logDebug("Vertex "
            << vertex.toString() << " may not be erased on tree " << _id
            << " as it is father of further refined vertices. Unroll flag.");
        vertex.setState(GridVertex::State::Refined);
        _statistics.setCoarseningHasBeenVetoed(true);
        _statistics.numberOfStationarySweeps = 0;
      } else {
        logDebug("Erase vertex " << vertex.toString() << " outside of domain on tree " << _id);
        vertex.setState(GridVertex::State::Erasing);
        _statistics.numberOfStationarySweeps = 0;
      }
    }

    // has to be here. Don't want to interfere with state splitting
    if (
      not vertexIsAdjacentToLocalTree
      and
      not vertex.getFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep)
      and
      vertex.getState()==GridVertex::State::Refined
    ) {
      logDebug("Would like to erase " << vertex.toString() << " in spacetree " << _id);
      vertex.setState(GridVertex::State::EraseTriggered);
    }
  }

  if (_spacetreeState == SpacetreeState::Joining and vertexIsAdjacentToLocalTree) {
    const int slot = getOrCreateNeighbourSlot(_masterId);
    logDebug("Stream vertex "
        << vertex.toString() << " from tree " << _id << " to master " << _masterId << " through vertical exchange channel");
    _parallelExchangeVertexStacks[slot][StackTopology::toIndex(StackTopology::ParallelExchangeStack::VerticalOutput)].push(std::move(vertex));
  }
}

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

    result
      &= ((coarseVertexPosition(d) == 1 and fineVertexPosition(d) > 0) or (coarseVertexPosition(d) == 0 and fineVertexPosition(d) < 3));
  }

  return result;
}

void peano4::grid::Spacetree::updateVertexBeforeStore(
  GridVertex&                               vertex,
  GridVertex                                coarseGridVertices[TWO_POWER_D],
  const tarch::la::Vector<DIMENSIONS, int>& fineVertexPositionWithinPatch
) {
  if (_spacetreeState != SpacetreeState::EmptyRun) {
    if (vertex.getState() == GridVertex::State::New) {
      vertex.setState(GridVertex::State::Unrefined);
    }

    if (
      _spacetreeState==SpacetreeState::Running
      and
      vertex.numberOfAdjacentRefinedLocalCells==TWO_POWER_D
      and
      vertex.getState()==GridVertex::State::Unrefined
    ) {
      vertex.setState(GridVertex::State::RefinementTriggered);
      logDebug("Have to post-refine vertex "
          << vertex.toString() << " as it is surrounded by 2^d refined cells on tree " << _id << " x "
          << peano4::grid::toString(_spacetreeState));
    }

    /*
        @todo Do we want to avoid hanging vertices along boundary? I think this would make sense

        if (
          vertex.getState()==GridVertex::State::Unrefined
          and
          vertex.
        ) {
          vertex.setState( GridVertex::State::RefinementTriggered );
          logDebug("have to post-refine vertex " << vertex.toString());
        }
    */

    bool restrictIsAntecessorOfRefinedVertex = vertex.getFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep);

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

    sendGridVertex(vertex);

    if (restrictIsAntecessorOfRefinedVertex) {
      dfor2(k) if (restrictToCoarseGrid(k, fineVertexPositionWithinPatch)) {
        logDebug("Set antecessor flag (veto coarsening) on vertex " << coarseGridVertices[kScalar].toString(
          ) << " due to vertex " << vertex.toString());
        coarseGridVertices[kScalar].setFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep, true);
        coarseGridVertices[kScalar].setFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep,
          coarseGridVertices[kScalar].getFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep)
          or vertex.getFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep)
        );
      }
      enddforx
    }

    if (shouldEraseAdjacencyInformation(vertex, coarseGridVertices, fineVertexPositionWithinPatch)) {
      vertex.adjacentRanks.fill(-1);
    }
  }
}

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

    const auto& adjacentRanks = coarseGridVertices[utils::dLinearised(position, 2)].adjacentRanks;
    return std::find(std::begin(adjacentRanks), std::end(adjacentRanks), _id) == std::end(adjacentRanks);
  };

  return isOutsideRecursively(isOutsideRecursively, fineVertexPositionWithinPatch);
}

std::array<int, TWO_POWER_D> peano4::grid::Spacetree::getAdjacentRanksForNewVertex(
  GridVertex coarseGridVertices[TWO_POWER_D], const tarch::la::Vector<DIMENSIONS, int>& vertexPositionWithin3x3Patch
) const {
  std::array<int, TWO_POWER_D> adjacentRanks;
  adjacentRanks.fill(-1);

  dfor2(k) std::bitset<DIMENSIONS> vertexToInherit;
  std::bitset<DIMENSIONS>          indexToInherit;
  for (int d = 0; d < DIMENSIONS; d++) {
    if (vertexPositionWithin3x3Patch(d) <= 1) {
      vertexToInherit.set(d, false);
      indexToInherit.set(d, k(d) > 0 or vertexPositionWithin3x3Patch(d) > 0);
    } else {
      vertexToInherit.set(d, true);
      indexToInherit.set(d, not(k(d) < 1 or vertexPositionWithin3x3Patch(d) < 3));
    }
  }
  logDebug("inherit "
      << indexToInherit.to_ulong() << "th index from coarse vertex " << vertexToInherit << " into local index "
      << kScalar);
  adjacentRanks[kScalar] = coarseGridVertices[vertexToInherit.to_ulong()].adjacentRanks[indexToInherit.to_ulong()];
  enddforx

    return adjacentRanks;
}

void peano4::grid::Spacetree::loadVertices(
  const AutomatonState&                     fineGridStatesState,
  GridVertex                                coarseGridVertices[TWO_POWER_D],
  GridVertex                                fineGridVertices[TWO_POWER_D],
  const VertexType                          vertexTypes[TWO_POWER_D],
  const tarch::la::Vector<DIMENSIONS, int>& cellPositionWithin3x3Patch,
  GridTraversalObserver&                    observer
) {
  _vertexLookupTables.push_back(&createVertexLookupTable(fineGridStatesState, cellPositionWithin3x3Patch));
  const auto& lut = *_vertexLookupTables.back();

  int                        cachedStackNo = -1;
  stacks::Stack<GridVertex>* cachedStack   = nullptr;

  for (int i = 0; i < TWO_POWER_D; i++) {
    const int  vIndex   = lut.vIdx[i];
    auto&      vSlot    = fineGridVertices[vIndex];
    const auto vPos     = lut.vPos[i];
    VertexType type     = vertexTypes[vIndex];
    const int  stackNo  = lut.readStack[i];
    const bool inOut    = lut.inOutRead[i];

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
        updateVertexAfterLoad(
          vSlot,
          coarseGridVertices,
          vPos,
          observer
        );
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
        updateVertexAfterLoad(
          vSlot,
          coarseGridVertices,
          vPos,
          observer
        );
        vSlot.setState(GridVertex::State::Delete);
      }
    } break;
    }

#if PEANO_DEBUG
    const auto expectedX = GridVertex::getVertexCoordinate(fineGridStatesState.x, fineGridStatesState.h, vIndex);
    assertion(tarch::la::equals((vSlot.getX()), (expectedX)), vSlot.getX(), expectedX, vSlot.toString(), fineGridStatesState.toString(), vIndex, toString(), _id);
    vSlot.setX(expectedX);
#endif

#if PEANO_DEBUG
    logDebug("Handled " << peano4::grid::toString(type) << " vertex " << vIndex << " at " << vPos << ": " << vSlot.toString());
    assertion((vSlot.level) == (fineGridStatesState.level), vSlot.level, fineGridStatesState.level, vSlot.toString(), fineGridStatesState.toString(), vIndex, toString(), _id);
#endif
  }
}

void peano4::grid::Spacetree::storeVertices(
  const AutomatonState&                     fineGridStatesState,
  GridVertex                                coarseGridVertices[TWO_POWER_D],
  GridVertex                                fineGridVertices[TWO_POWER_D],
  const VertexType                          vertexTypes[TWO_POWER_D],
  const tarch::la::Vector<DIMENSIONS, int>& cellPositionWithin3x3Patch,
  GridTraversalObserver&                    observer
) {
  assertion(not _vertexLookupTables.empty());
  const auto& lut = *_vertexLookupTables.back();

  int                        cachedStackNo = -1;
  stacks::Stack<GridVertex>* cachedStack   = nullptr;

  for (int i = 0; i < TWO_POWER_D; i++) {
    const int  vIndex   = lut.vIdx[i];
    auto&      vSlot    = fineGridVertices[vIndex];
    VertexType type     = vertexTypes[vIndex];
    const int  stackNo  = lut.writeStack[i];
    const bool inOut    = lut.inOutWrite[i];

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
      updateVertexBeforeStore(vSlot, coarseGridVertices, lut.vPos[i]);
    }
    if (stackNo != cachedStackNo) {
      assertion(stackNo >= 0 and stackNo < StackTopology::TotalNumberOfLocalExchangeStacks, stackNo, _id);
      cachedStack   = &_localExchangeVertexStacks[stackNo];
      cachedStackNo = stackNo;
    }
    auto* pushStack = cachedStack;
    pushStack->push(std::move(vSlot));
    _statistics.numberOfVertices++;
  }

  _vertexLookupTables.pop_back();
}

bool peano4::grid::Spacetree::isFaceAlongPeriodicBoundaryCondition(
  GridVertex fineGridVertices[TWO_POWER_D], int faceNumber, bool calledByReceivingProcess
) const {

  int adjacentRanksOfFace[TWO_POWER_D];
  SpacetreeHelper::getAdjacentRanksOfFace(fineGridVertices, faceNumber, calledByReceivingProcess, adjacentRanksOfFace);

  bool isAdjacentToLocalRank              = false;
  bool holdsPeriodicBoundaryConditionFlag = false; // semantics is either has been local or will be local
  for (int i = 0; i < TWO_POWER_D; i++) {
    isAdjacentToLocalRank |= adjacentRanksOfFace[i] == _id;
    holdsPeriodicBoundaryConditionFlag |= adjacentRanksOfFace[i] == RankOfPeriodicBoundaryCondition;
  }

  holdsPeriodicBoundaryConditionFlag &= isAdjacentToLocalRank;

  return holdsPeriodicBoundaryConditionFlag;
}

void peano4::grid::Spacetree::mergeGridVertexAdjacencyListsAtHorizontalDomainBoundary(
  GridVertex& vertex, const GridVertex& inVertex, int neighbour
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
        logDebug("Update "
            << i << "th entry of adjacency data of vertex " << vertex.toString() << " with neighbouring vertex "
            << inVertex.toString() << " on tree " << _id);
      }
      vertex.adjacentRanks[i] =  inVertex.adjacentRanks[i];
    }
  }
}

void peano4::grid::Spacetree::mergeGridVertexRefinementStateAtHorizontalDomainBoundary(
  GridVertex&       vertex,
  const GridVertex& inVertex,
  int               neighbour
) {
  assertion(
    vertex.getState() != GridVertex::State::HangingVertex, inVertex.toString(), vertex.toString(), _id, neighbour
  );
  assertion(
    inVertex.getState() != GridVertex::State::HangingVertex, inVertex.toString(), vertex.toString(), _id, neighbour
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
  assertion((vertex.level) == (inVertex.level), vertex.level, inVertex.level, inVertex.toString(), vertex.toString(), _id, neighbour);
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
  vertex.setFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep,
    vertex.getFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep)
    or inVertex.getFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep)
    // or inVertex.getFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep)
  );

  // vertex.setFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep,
  //   vertex.getFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep)
  //   or inVertex.getFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep)
  //   or inVertex.getFlag(GridVertex::HasBeenParentOfSubtreeVertexInPreviousTreeSweep)
  // );

  //
  // Update refinement flags
  //
  assertion(vertex.getState() != GridVertex::State::Erasing, vertex.toString(), inVertex.toString(), _id);
  assertion(vertex.getState() != GridVertex::State::Refining, vertex.toString(), inVertex.toString(), _id);

  const bool OnlyNeighbourHasTriggeredRefinement
    = inVertex.getState() == GridVertex::State::RefinementTriggered
      && (vertex.getState() == GridVertex::State::Unrefined or vertex.getState() == GridVertex::State::New or vertex.getState() == GridVertex::State::Delete);
  const bool OnlyLocalHasTriggeredRefinement =
      vertex.getState()==GridVertex::State::RefinementTriggered &&
      (inVertex.getState()==GridVertex::State::Unrefined or inVertex.getState()==GridVertex::State::New or inVertex.getState()==GridVertex::State::Delete);
  const bool OnlyNeighbourHasTriggeredErase = inVertex.getState() == GridVertex::State::EraseTriggered
                                              && vertex.getState() == GridVertex::State::Refined;
  const bool OnlyLocalHasTriggeredErase = vertex.getState() == GridVertex::State::EraseTriggered
                                          && inVertex.getState() == GridVertex::State::Refined;

  if (OnlyNeighbourHasTriggeredRefinement || OnlyLocalHasTriggeredRefinement) {
    vertex.setState(GridVertex::State::RefinementTriggered);
    logDebug("set state to " << vertex.toString() << " on tree " << _id << " due to merge with neighbour");
  } else if (OnlyNeighbourHasTriggeredErase || OnlyLocalHasTriggeredErase) {
    vertex.setState(GridVertex::State::EraseTriggered);
    logDebug("set state to " << vertex.toString() << " on tree " << _id << " due to merge with neighbour");
  } else {
    assertion(vertex.getState() == inVertex.getState(), inVertex.toString(), vertex.toString(), _id, neighbour);
  }
}

void peano4::grid::Spacetree::receiveAndMergeGridVertexAtHorizontalBoundary(GridVertex& vertex) {
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
    auto& in = _parallelExchangeVertexStacks[slot][StackTopology::toIndex(StackTopology::ParallelExchangeStack::HorizontalInput)];
    assertion(not in.empty(), _id, neighbour, vertex.toString(), in.toString());
    GridVertex inVertex = in.pop();
    mergeGridVertexRefinementStateAtHorizontalDomainBoundary(vertex, inVertex, neighbour);
    mergeGridVertexAdjacencyListsAtHorizontalDomainBoundary(vertex, inVertex, neighbour);
  }

  if (_id == 0 and hasPeriodicBCs(vertex.adjacentRanks)) {
    const auto outStacks = StackTopology::getOutputStacksForPeriodicBoundaryExchange(vertex.adjacentRanks);
    for (const auto& p : outStacks) {
      // p.first  = periodic BC output stack number (local)
      // p.second = bitmask of normals / symmetry axes involved
      assertion(
        StackTopology::isPeriodicBoundaryExchangeOutputStackNumber(p.first),
        _id, p.first, p.second, vertex.toString()
      );
      const int fromStack = StackTopology::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(p.first);
      assertion(fromStack >= 0 and fromStack < StackTopology::TotalNumberOfLocalExchangeStacks, fromStack, p.first, StackTopology::TotalNumberOfLocalExchangeStacks, _id);
      assertion(not _localExchangeVertexStacks[fromStack].empty(), fromStack, p.first, p.second, _id);
      GridVertex inVertex = _localExchangeVertexStacks[fromStack].pop();
      logDebug("Read periodic BC data from input stack " << fromStack
        << " (mapped from output stack " << p.first << "): "
        << inVertex.toString());
      logDebug("Normals of involved symmetry axes: " << p.second);
      mergeGridVertexRefinementStateAtHorizontalDomainBoundary(vertex, inVertex, _id);

      dfor2(k)
        assertion(vertex.adjacentRanks[kScalar] == RankOfPeriodicBoundaryCondition
          or vertex.adjacentRanks[kScalar] == _id
          or vertex.adjacentRanks[kScalar] == -1,
          vertex.toString(),
          inVertex.toString(),
          _id
        );
      enddforx
   }
  }
}

void peano4::grid::Spacetree::sendGridVertex(const GridVertex& vertex) {
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

    _parallelExchangeVertexStacks[slot][StackTopology::toIndex(StackTopology::ParallelExchangeStack::HorizontalOutput)].push(std::move(vertexCopy));
  }

  if (_id == 0 and hasPeriodicBCs(vertex.adjacentRanks)) {
    const auto outStacks = StackTopology::getOutputStacksForPeriodicBoundaryExchange(vertex.adjacentRanks);
    for (const auto& s : outStacks) {
      assertion(s.first >= 0 and s.first < StackTopology::TotalNumberOfLocalExchangeStacks, s.first, _id);
      _localExchangeVertexStacks[s.first].push(vertex);
      logDebug("Vertex " << vertex.toString() << " on tree " << _id
                  << " goes to stack " << s.first << " to realise periodic BC");
    }
  }
}

void peano4::grid::Spacetree::incrementNumberOfAdjacentRefinedLocalCells(GridVertex vertices[TWO_POWER_D]) {
  dfor2(k)
    vertices[kScalar].numberOfAdjacentRefinedLocalCells += 1;
  enddforx
}

void peano4::grid::Spacetree::updateVertexRanksWithinCell(GridVertex fineGridVertices[TWO_POWER_D], int newId) {
  dfor2(k)
    fineGridVertices[kScalar].adjacentRanks[TWO_POWER_D - kScalar - 1] = newId;
  enddforx
}

void peano4::grid::Spacetree::evaluateGridControlEvents(const AutomatonState& fineGridState, GridVertex coarseGridVertices[TWO_POWER_D], GridVertex fineGridVertices[TWO_POWER_D]) {
  if (_gridControlEvents.empty()) {
    return;
  }
  assertion(_spacetreeState != SpacetreeState::EmptyRun and _spacetreeState != SpacetreeState::NewFromSplit and _spacetreeState != SpacetreeState::Joining, toString());

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
        tarch::la::allGreaterEquals(stateMax, eventMin)
        and
        tarch::la::allSmallerEquals(stateMin, eventMax);

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
        tarch::la::allGreaterEquals(stateMin, eventMin)
        and
        tarch::la::allSmallerEquals(stateMax, eventMax);

      const bool coarsenToTargetResolution =
        // If the event's target resolution is greater or equal, we coarsen.
        tarch::la::allGreaterEquals(event.targetH, fineGridState.h)
        // Erase events are computed on leaves, but have to be applied on the
        // direct parents of these leaves. Therefore, we coarsen only if the
        // current cell's size equals the target resolution, i.e. the cell is not
        // finer than one refinement level below the event's target resolution.
        and
        not tarch::la::allSmallerEquals(3.0 * fineGridState.h, event.targetH);

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

  // Avoid changing topology while while any of the surrounding coarse vertices are in transitional states
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

      // Mark vertex as needed by a refined region in this sweep
      v.setFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep, true);

      const auto s = v.getState();
      if (s == GridVertex::State::EraseTriggered) {
        // Cancel pending erase if an adjacent cell wants to keep/refine.
        v.setState(GridVertex::State::Refined);
      }
      else if (mayChangeGrid and s == GridVertex::State::Unrefined) {
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

      if (
        mayChangeGrid
        and
        v.getState() == GridVertex::State::Refined
        and
        not v.getFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep)
      ) {
        v.setState(GridVertex::State::EraseTriggered);
      }
    }
  }
}

void peano4::grid::Spacetree::descend(const AutomatonState& state, GridVertex vertices[TWO_POWER_D], GridTraversalObserver& observer) {
  assertion(SpacetreeHelper::isSpacetreeNodeRefined(vertices));

  const bool hasJoiningTransitions   = not _joining.empty();
  const bool hasGridControlEvents    = not _gridControlEvents.empty();
  const bool hasTriggeredSplits      = not _splitTriggered.empty();
  const bool spacetreeStateIsRunning = _spacetreeState == SpacetreeState::Running;
  const bool canReceiveUserData      = _spacetreeState != SpacetreeState::EmptyRun
                                  and _spacetreeState != SpacetreeState::NewFromSplit
                                  and _spacetreeState != SpacetreeState::Joined;
  const bool canSendUserData         = _spacetreeState != SpacetreeState::EmptyRun
                                  and _spacetreeState != SpacetreeState::Joined;

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

  peano4::utils::LoopDirection loopDirection = PeanoCurve::getLoopDirection(state);
  zfor3(k, loopDirection)
    traceEnter("Grid bookkeeping before enter");
    const auto kLinearised = utils::dLinearised(k, 3);

    VertexType vertexTypes[TWO_POWER_D];
    getVertexTypes(vertices, k, vertexTypes);

    GridVertex fineGridVertices[TWO_POWER_D];
    loadVertices(fineGridStates[kLinearised], vertices, fineGridVertices, vertexTypes, k, observer);

    const auto& lut = *_vertexLookupTables.back();

    FaceType faceTypes[TWO_TIMES_D];
    for (int i = 0; i < TWO_TIMES_D; ++i) {
      faceTypes[i] = getFaceType(vertexTypes, lut.faceIndex[i]);
    }

    if (hasJoiningTransitions) {
      mergeCellFromWorkerWithMasterThroughoutJoin(vertices, fineGridVertices);
    }

    const bool isRefined = SpacetreeHelper::isSpacetreeNodeRefined(fineGridVertices);
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
      receiveAndMergeUserData(fineGridStates[kLinearised], observer, enterNodeGridTraversalEvent, fineGridVertices);
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
      descend(fineGridStates[kLinearised], fineGridVertices, observer);
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
        int currentLevel = fineGridStates[kLinearised].level;
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
      sendUserData(fineGridStates[kLinearised], observer, leaveNodeGridTraversalEvent, fineGridVertices);
      traceLeave("Observer data send");
    }

    observer.storeNodeData(leaveNodeGridTraversalEvent);

    traceEnter("Grid bookkeeping after leave");
    if (not _noParallelTopologyChanges) {
      splitOrJoinCellBottomUp(vertices, fineGridVertices);
    }

    storeVertices(fineGridStates[kLinearised], vertices, fineGridVertices, vertexTypes, k, observer);
    traceLeave("Grid bookkeeping after leave");
  endzfor
}

void peano4::grid::Spacetree::receiveAndMergeUserData(
  const AutomatonState&                   state,
  GridTraversalObserver&                  observer,
  const GridTraversalEvent&               enterCellTraversalEvent,
  GridVertex                              fineGridVertices[TWO_POWER_D]
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
          logDebug("receive neighbour data from neighbour "
              << p << " for vertex " << fineGridVertices[inVertexPositionWithinCell].toString());
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

        if (_id == 0 and hasPeriodicBCs(fineGridVertices[inVertexPositionWithinCell].adjacentRanks)) {
          const auto  outStacks = StackTopology::getOutputStacksForPeriodicBoundaryExchange(fineGridVertices[inVertexPositionWithinCell].adjacentRanks);
          for (const auto& p : outStacks) {
            const auto fromStack = StackTopology::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(p.first);
            observer.receiveNodeData(
              GridTraversalObserver::NodeData::Vertex,
              inVertexPositionWithinCell,
              fromStack,
              stacks::SendReceiveContext::PeriodicBoundaryDataSwap,
              enterCellTraversalEvent
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
          const int outStack  = StackTopology::getOutputStackForPeriodicBoundaryExchange(inFacePositionWithinCell);
          const int fromStack = StackTopology::mapPeriodicBoundaryExchangeOutputStackOntoInputStack(outStack);
          logDebug("Will merge face from periodic BC stack #" << fromStack << " with local face");

          observer.receiveNodeData(
            GridTraversalObserver::NodeData::Face,
            inFacePositionWithinCell,
            fromStack,
            stacks::SendReceiveContext::PeriodicBoundaryDataSwap,
            enterCellTraversalEvent
          );
        }
      }
    }
  }
}

void peano4::grid::Spacetree::sendUserData(
  const AutomatonState&     state,
  GridTraversalObserver&    observer,
  const GridTraversalEvent& leaveCellTraversalEvent,
  GridVertex                fineGridVertices[TWO_POWER_D]
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
          logDebug("send local vertex data of "
              << fineGridVertices[outVertexPositionWithinCell].toString() << " from stack " << outVertexStack
              << " on tree " << _id << " to neighbour " << p << ". Position within cell=" << outVertexPositionWithinCell
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

        if (_id == 0 and hasPeriodicBCs(fineGridVertices[outVertexPositionWithinCell].adjacentRanks)) {
          const auto outStacks = StackTopology::getOutputStacksForPeriodicBoundaryExchange(fineGridVertices[outVertexPositionWithinCell].adjacentRanks);
          for (const auto& stackNo : outStacks) {
            logDebug("vertex "
                << fineGridVertices[outVertexPositionWithinCell].toString() << " on tree " << _id << " goes to stack "
                << stackNo.first << " to realise periodic BC");

            observer.sendNodeData(
              GridTraversalObserver::NodeData::Vertex,
              outVertexPositionWithinCell,
              stackNo.first,
              stacks::SendReceiveContext::PeriodicBoundaryDataSwap,
              leaveCellTraversalEvent
            );
          }
        }

        for (const auto& p : _splitting) {
          const auto& adjacentRanks = fineGridVertices[outVertexPositionWithinCell].backupOfAdjacentRanks;
          if (std::find(std::begin(adjacentRanks), std::end(adjacentRanks), p) != std::end(adjacentRanks)) {
            logDebug("stream local vertex data of "
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
            logDebug("skip vertex data of " << fineGridVertices[outVertexPositionWithinCell].toString() << " on tree " << _id);
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
          logDebug("send local face from stack "
              << outFaceStack << " of tree " << _id << " to neighbour " << outgoingNeighbourTree
              << ". Position within cell=" << outFacePositionWithinCell);

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
          const int toStack = StackTopology::getOutputStackForPeriodicBoundaryExchange(
            outFacePositionWithinCell
          );

          logDebug("send local face from stack " << outFaceStack << " of tree " << _id << " to periodic BC stack #" << toStack);

          observer.sendNodeData(
            GridTraversalObserver::NodeData::Face,
            outFacePositionWithinCell,
            toStack,
            stacks::SendReceiveContext::PeriodicBoundaryDataSwap,
            leaveCellTraversalEvent
          );
        }
      }

      int adjacentRanks[TWO_POWER_D];
      SpacetreeHelper::getAdjacentRanksOfFace(fineGridVertices, outFacePositionWithinCell, false, adjacentRanks);
      for (const auto& p : _splitting) {
        if (std::find(std::begin(adjacentRanks), std::end(adjacentRanks), p) != std::end(adjacentRanks)) {
          logDebug("stream local face from stack "
              << outFaceStack << " of tree " << _id << " to new worker " << p
              << ". Position within cell=" << outFacePositionWithinCell);

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

  //int outCellStack = leaveCellTraversalEvent.getCellData();
  for (const auto& p : _splitting) {
    if (SpacetreeHelper::getTreeOwningSpacetreeNode(fineGridVertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining) == p) {
      logDebug("stream local cell of tree "
          << _id << " to new worker " << p << ". Left bottom vertex=" << fineGridVertices[0].toString());

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

void peano4::grid::Spacetree::splitOrJoinCellBottomUp(GridVertex coarseGridVertices[TWO_POWER_D], GridVertex fineGridVertices[TWO_POWER_D]) {
  [[maybe_unused]] constexpr int NoSplit = -1;

  if (not _splitTriggered.empty() and _splitTriggered.begin()->second.mode == SplitInstruction::Mode::BottomUp) {
    const bool isSplitCandidate = SpacetreeHelper::isSpacetreeNodeSplitCandidate(coarseGridVertices, fineGridVertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining);
    const bool isRefined        = SpacetreeHelper::isSpacetreeNodeRefined(fineGridVertices);

    logDebug("Checked cell for bottom-up splits (first target tree=" << _splitTriggered.begin()->first
        << "). IsRefined=" << isRefined << ", IsSplitCandidate=" << isSplitCandidate
        << " / " << fineGridVertices[0].toString());

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
  } else if (not _splitTriggered.empty() and _splitTriggered.begin()->second.mode == SplitInstruction::Mode::AggressiveTopDown and not _splittedCells.empty()
  ) {
    int targetSpacetreeId = _splittedCells.back();
    _splittedCells.pop_back();

    logInfo("Checked cell for top-down splits (first split " << _splitTriggered.begin()->first << ")");
    assertion(SpacetreeHelper::isSpacetreeNodeLocal(fineGridVertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining, false, false, false));
    assertion(SpacetreeHelper::isSpacetreeNodeLocal(coarseGridVertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining, false, false, false));
    updateVertexRanksWithinCell(fineGridVertices, targetSpacetreeId);
  }
}

void peano4::grid::Spacetree::splitCellTopDown(GridVertex coarseGridVertices[TWO_POWER_D], GridVertex fineGridVertices[TWO_POWER_D]) {
  [[maybe_unused]] constexpr int NoSplit = -1;

  if (not _splitTriggered.empty() and _splitTriggered.begin()->second.mode == SplitInstruction::Mode::AggressiveTopDown) {
    const bool isParentSplit     = not _splittedCells.empty();
    const int  targetSpacetreeId = getSplittingTree();
    const bool isSplitCandidate = targetSpacetreeId >= 0 and SpacetreeHelper::isSpacetreeNodeSplitCandidate(coarseGridVertices, fineGridVertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining);
    const bool isRefined        = SpacetreeHelper::isSpacetreeNodeRefined(fineGridVertices);

    if (isParentSplit and isRefined) {
      assertion(SpacetreeHelper::isSpacetreeNodeLocal(fineGridVertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining, false, false, false));
      assertion(SpacetreeHelper::isSpacetreeNodeLocal(coarseGridVertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining, false, false, false));
      int parentId = _splittedCells.back();
      _splittedCells.push_back(parentId);
    } else if (isParentSplit and not isRefined) {
      assertion(SpacetreeHelper::isSpacetreeNodeLocal(fineGridVertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining, false, false, false));
      assertion(SpacetreeHelper::isSpacetreeNodeLocal(coarseGridVertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining, false, false, false));
      updateSplittingCounter(targetSpacetreeId);
      int parentId = _splittedCells.back();
      _splittedCells.push_back(parentId);
    } else if (not isParentSplit and isSplitCandidate and isRefined) {
      assertion(SpacetreeHelper::isSpacetreeNodeLocal(fineGridVertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining, false, false, false));
      assertion(SpacetreeHelper::isSpacetreeNodeLocal(coarseGridVertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining, false, false, false));
      logDebug("Refined cell at " << fineGridVertices[0].toString() << " should go with all children to tree " << targetSpacetreeId);
      _splittedCells.push_back(targetSpacetreeId);
    } else if (not isParentSplit and isSplitCandidate and not isRefined) {
      updateSplittingCounter(targetSpacetreeId);
      assertion(SpacetreeHelper::isSpacetreeNodeLocal(fineGridVertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining, false, false, false));
      assertion(SpacetreeHelper::isSpacetreeNodeLocal(coarseGridVertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining, false, false, false));
      _splittedCells.push_back(targetSpacetreeId);
    }
    logDebug("#SplittedCells=" << _splittedCells.size());
  }
}

void peano4::grid::Spacetree::mergeCellFromWorkerWithMasterThroughoutJoin(GridVertex coarseGridVertices[TWO_POWER_D], GridVertex fineGridVertices[TWO_POWER_D]) {
  for (const auto& worker : _joining) {
    if (SpacetreeHelper::isSpacetreeNodeLocal(coarseGridVertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining, false, false, false) and SpacetreeHelper::getTreeOwningSpacetreeNode(fineGridVertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining) == worker) {
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

void peano4::grid::Spacetree::updateSplittingCounter(int treeId) {
  auto it = _splitTriggered.find(treeId);
  if (it != _splitTriggered.end()) {
    it->second.numberOfFineGridCells = std::max(it->second.numberOfFineGridCells - 1, 0);
  }
}

int peano4::grid::Spacetree::getSplittingTree() const {
  for (auto& p : _splitTriggered) {
    if (p.second.numberOfFineGridCells > 0) {
      return p.first;
    }
  }
  return -1;
}

const peano4::grid::GridStatistics& peano4::grid::Spacetree::getGridStatistics() const { return _statistics; }

void peano4::grid::Spacetree::markVerticesAroundForkedCell(GridVertex coarseGridVertices[TWO_POWER_D], GridVertex fineGridVertices[TWO_POWER_D]) const {
  dfor2(k)
    assertion(coarseGridVertices[kScalar].getState() != GridVertex::State::HangingVertex, coarseGridVertices[kScalar].toString());
    coarseGridVertices[kScalar].setFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep, true);
    coarseGridVertices[kScalar].setFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep, true);
    fineGridVertices[kScalar].setFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep, true);
  enddforx
}

void peano4::grid::Spacetree::split(int newSpacetreeId, const SplitInstruction& instruction) {
  assertion(
    instruction.numberOfFineGridCells == std::numeric_limits<int>::max() or maySplit(),
    newSpacetreeId,
    instruction,
    toString()
  );
  assertion(instruction.numberOfFineGridCells != std::numeric_limits<int>::max(), newSpacetreeId, instruction, toString());
  assertion(instruction.numberOfFineGridCells >= 1, newSpacetreeId, instruction, toString());

  _splitTriggered.insert(std::pair<int, SplitInstruction>(newSpacetreeId, instruction));

  auto firstMode = _splitTriggered.begin()->second.mode;
  for (auto& p : _splitTriggered) {
    if (p.second.mode != firstMode) {
      logWarning("Split of tree "
          << p.first << " is of type " << p.second.mode << " while other splits have mode " << firstMode
          << ". Switch all splits to " << firstMode);
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

bool peano4::grid::Spacetree::maySplit() const {
  return _spacetreeState == SpacetreeState::Running and _joinTriggered.empty() and _joining.empty()
         and _splitting.empty() and _statistics.numberOfLocalUnrefinedCells > 0
         and (_splitTriggered.empty() or _splitTriggered.begin()->second.numberOfFineGridCells < std::numeric_limits<int>::max());
}

bool peano4::grid::Spacetree::mayJoinWithWorker() const {
  return _spacetreeState == SpacetreeState::Running and _splitTriggered.empty() and _joining.empty()
         and _splitting.empty() and _joinTriggered.empty();
}

bool peano4::grid::Spacetree::mayJoinWithMaster() const {
  return _spacetreeState == SpacetreeState::Running and _masterId >= 0
         and _statistics.numberOfStationarySweeps > NumberOfStationarySweepsToWaitAtLeastTillJoin
         and _splitTriggered.empty() and _splitting.empty() and _joinTriggered.empty() and _joining.empty()
         and _statistics.numberOfLocalRefinedCells == 0;
}

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
