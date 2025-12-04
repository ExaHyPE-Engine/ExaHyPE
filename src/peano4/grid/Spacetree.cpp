// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Spacetree.h"

#include "GridControlEvent.h"
#include "GridTraversalEvent.h"
#include "PeanoCurve.h"
#include "GridTraversalObserver.h"
#include "VertexType.h"
#include "SpacetreeHelper.h"
#include "peano4/parallel/Node.h"
#include "peano4/parallel/SpacetreeSet.h"
#include "peano4/utils/Loop.h"
#include "tarch/logging/Statistics.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Lock.h"

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

tarch::logging::Log peano4::grid::Spacetree::_log("peano4::grid::Spacetree");

peano4::stacks::TreeToStacks<peano4::stacks::Stack<peano4::grid::GridVertex>> peano4::grid::Spacetree::_vertexStack;

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

  _vertexLookupTables.reserve(1000);

  logDebug("Spacetree()", "Created spacetree with " << offset << "x" << width << " and periodic BCs=" << _periodicBC);
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

  _vertexLookupTables.reserve(1000);

  logDebug("Spacetree()", "Created spacetree " << _id << " with master tree " << masterId << ": " << _root.toString());
}

void peano4::grid::Spacetree::traverse(GridTraversalObserver& observer, bool calledFromSpacetreeSet) {
  logTraceIn("traverse()");

  for (int d = 0; d < TWO_TIMES_D; d++) {
    assertion2(_root.accessNumber[d] > -2 * DIMENSIONS, d, _root.toString());
    assertion2(_root.accessNumber[d] < 2 * DIMENSIONS, d, _root.toString());
  }

  if (calledFromSpacetreeSet) {
    assertion(_joinTriggered.empty() or _splitTriggered.empty());
    assertion(_joinTriggered.empty() or _splitting.empty());
    assertion(_joining.empty() or _splitTriggered.empty());
    assertion(_joining.empty() or _splitting.empty());
    logDebug(
      "traverse()",
      _splitTriggered.size() << " tree splits triggered and " << _splitting.size() << " splitting trees on tree " << _id
    );
  }

  _statistics.clear(_id == 0);

  if (_spacetreeState == SpacetreeState::EmptyRun or _spacetreeState == SpacetreeState::NewFromSplit or _spacetreeState == SpacetreeState::Joining) {
    _gridControlEvents.clear();
  } else {
    _gridControlEvents = observer.getGridControlEvents();
    GridControlEvent::merge(_gridControlEvents);
    if (not _gridControlEvents.empty()) {
      logDebug("traverse()", "Got " << _gridControlEvents.size() << " grid control events");
    }
  }

  _splittedCells.clear();

  const bool isFirstTraversal = _spacetreeState == SpacetreeState::NewRoot;

  GridVertex vertices[TWO_POWER_D];
  dfor2(k)
    std::array<int, TWO_POWER_D> adjacentRanks;
    adjacentRanks.fill(-1);
    adjacentRanks[TWO_POWER_D - 1 - kScalar] = 0;
    vertices[kScalar] = GridVertex::create(
      0, // level
      isFirstTraversal ? GridVertex::State::Refining : GridVertex::State::Refined,
      adjacentRanks,
      false // isNewFineGridVertex
    );
    vertices[kScalar].setFlag(GridVertex::HasBeenParentOfSubtreeVertexInPreviousTreeSweep, true);
    logDebug("traverse()", "Create " << vertices[kScalar].toString() << " for tree " << _id);
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
      enddforx logDebug(
        "traverse()", "Set periodic boundary conditions: " << vertices[vertexScalar].toString()
      );
      enddforx
    }
  }

  observer.beginTraversal(_root.x, _root.h);
  descend(_root, vertices, observer);
  _statistics.maxDepth++;
  _statistics.minDepth++;
  observer.endTraversal(_root.x, _root.h);
  _root.setInverted(not _root.getInverted());

  if (calledFromSpacetreeSet) {
    _hasSplit.clear();
    for (auto& rank : _splitting) {
      _childrenIds.insert(rank);
      _hasSplit.insert(rank);
    }
    _splitting.clear();
    for (auto& p : _splitTriggered) {
      if (p.second.numberOfFineGridCells > 0) {
        logWarning(
          "traverse()",
          "Have not been able to assign enough cells from "
            << _id << " to new tree " << p.first << " (should have deployed " << p.second << " more cells)"
        );
      }
      _splitting.insert(p.first);
    }
    _splitTriggered.clear();

    _joining.clear();
    _joining.insert(_joinTriggered.begin(), _joinTriggered.end());
    _joinTriggered.clear();

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
      "traverse()", "Switched tree " << _id << " from " << grid::toString(oldState) << " into " << grid::toString(_spacetreeState)
    );
  }

  logTraceOut("traverse()");
}

void peano4::grid::Spacetree::receiveAndMergeGridVertexAtVerticalBoundary(GridVertex& vertex) {
  logTraceInWith2Arguments("receiveAndMergeGridVertexAtVerticalBoundary(GridVertex)", vertex.toString(), _id);

  for (const auto& p : _joining) {
    const auto& adjacentRanks = vertex.adjacentRanks;
    if (std::find(std::begin(adjacentRanks), std::end(adjacentRanks), p) != std::end(adjacentRanks)) {
      const int stackNo = parallel::Node::getInstance().getInputStackNumberForVerticalDataExchange(p);
      logDebug(
        "receiveAndMergeGridVertexAtVerticalBoundary(GridVertex)",
        "merge "
          << vertex.toString() << " with its replica from worker as worker is joining. Incoming stack no=" << stackNo
      );

      assertion3(not _vertexStack.getForPop(_id, stackNo)->empty(), _id, stackNo, vertex.toString());
      GridVertex copyFromFormerWorker = _vertexStack.getForPop(_id, stackNo)->pop();

      // assertionVectorNumericalEquals3(
      //   vertex.getX(), copyFromFormerWorker.getX(), vertex.toString(), copyFromFormerWorker.toString(), _id
      // );

      vertex.adjacentRanks = copyFromFormerWorker.adjacentRanks;
      logDebug(
        "receiveAndMergeGridVertexAtVerticalBoundary(GridVertex)", "updated adjacency lists: " << vertex.toString()
      );
    }
  }

  logTraceOut("receiveAndMergeGridVertexAtVerticalBoundary(GridVertex)");
}

void peano4::grid::Spacetree::updateVertexAfterLoad(
  [[maybe_unused]] GridVertex&                               vertex,
  [[maybe_unused]] GridVertex                                fineGridVertices[TWO_POWER_D],
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, int>& position,
  [[maybe_unused]] GridTraversalObserver&                        observer
) {
  if (_spacetreeState != SpacetreeState::NewFromSplit and _spacetreeState != SpacetreeState::EmptyRun) {
    receiveAndMergeGridVertexAtHorizontalBoundary(vertex);
    receiveAndMergeGridVertexAtVerticalBoundary(vertex);

    vertex.setFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep, vertex.getFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep));
    vertex.setFlag(GridVertex::HasBeenParentOfSubtreeVertexInPreviousTreeSweep, vertex.getFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep));

    vertex.clearFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep);
    vertex.clearFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep);

    vertex.numberOfAdjacentRefinedLocalCells = 0;

    if (vertex.getState() == GridVertex::State::RefinementTriggered) {
      if (SpacetreeHelper::isVertexAdjacentToLocalSpacetree(vertex, _id, _splitTriggered, _splitting, _joinTriggered, _joining, true, true, false)) {
        logDebug("updateVertexAfterLoad()", "switch vertex to refining on tree " << _id << ": " << vertex.toString());
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
        logDebug(
          "updateVertexAfterLoad()",
          "vertex "
            << vertex.toString() << " may not be erased on tree " << _id
            << " as it is father of further refined vertices. Unroll flag"
        );
        vertex.setState(GridVertex::State::Refined);
        _statistics.setCoarseningHasBeenVetoed(true);
        _statistics.numberOfStationarySweeps = 0;
      } else {
        logDebug(
          "updateVertexAfterLoad()", "erase vertex " << vertex.toString() << " outside of domain on tree " << _id
        );
        vertex.setState(GridVertex::State::Erasing);
        _statistics.numberOfStationarySweeps = 0;
      }
    }

    // has to be here. Don't want to interfere with state splitting
    if (
      not SpacetreeHelper::isVertexAdjacentToLocalSpacetree(vertex, _id, _splitTriggered, _splitting, _joinTriggered, _joining, true, true, false)
      and
      not vertex.getFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep)
      and
      vertex.getState()==GridVertex::State::Refined
    ) {
      logDebug(
        "updateVertexAfterLoad(GridVertex&)", "would like to erase " << vertex.toString() << " in spacetree " << _id
      );
      vertex.setState(GridVertex::State::EraseTriggered);
    }
  }

  if (_spacetreeState == SpacetreeState::Joining and SpacetreeHelper::isVertexAdjacentToLocalSpacetree(vertex, _id, _splitTriggered, _splitting, _joinTriggered, _joining, true, true, false)) {
    const int stackNo = peano4::parallel::Node::getInstance().getOutputStackNumberForVerticalDataExchange(_masterId);
    logDebug(
      "updateVertexAfterLoad(GridVertex&)",
      "stream vertex "
        << vertex.toString() << " from tree " << _id << " to master " << _masterId << " through stack " << stackNo
    );
    _vertexStack.getForPush(_id, stackNo)->push(std::move(vertex));
  }
}

bool peano4::grid::Spacetree::restrictToCoarseGrid(
  const tarch::la::Vector<DIMENSIONS, int>& coarseVertexPosition,
  const tarch::la::Vector<DIMENSIONS, int>& fineVertexPosition
) {
  bool result = true;
  for (int d = 0; d < DIMENSIONS; d++) {
    assertion2(coarseVertexPosition(d) >= 0, coarseVertexPosition, fineVertexPosition);
    assertion2(coarseVertexPosition(d) <= 1, coarseVertexPosition, fineVertexPosition);

    assertion2(fineVertexPosition(d) >= 0, coarseVertexPosition, fineVertexPosition);
    assertion2(fineVertexPosition(d) <= 3, coarseVertexPosition, fineVertexPosition);

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
  logTraceInWith2Arguments("updateVertexBeforeStore()", vertex.toString(), _id);

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
      logDebug(
        "updateVertexBeforeStore()",
        "have to post-refine vertex "
          << vertex.toString() << " as it surrounded by 2^d refined cells on tree " << _id << " x "
          << peano4::grid::toString(_spacetreeState)
      );
    }

    /*
        @todo Do we want to avoid hanging vertices along boundary? I think this would make sense

        if (
          vertex.getState()==GridVertex::State::Unrefined
          and
          vertex.
        ) {
          vertex.setState( GridVertex::State::RefinementTriggered );
          logDebug( "updateVertexBeforeStore()", "have to post-refine vertex " << vertex.toString() );
        }
    */

    bool restrictIsAntecessorOfRefinedVertex = vertex.getFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep);

    if (vertex.getState() == GridVertex::State::RefinementTriggered) {
      restrictIsAntecessorOfRefinedVertex = true;
    } else if (vertex.getState() == GridVertex::State::Refining) {
      vertex.setState(GridVertex::State::Refined);
      restrictIsAntecessorOfRefinedVertex = true;
    } else if (vertex.getState() == GridVertex::State::EraseTriggered) {
      restrictIsAntecessorOfRefinedVertex = true;
    } else if (vertex.getState() == GridVertex::State::Erasing) {
      vertex.setState(GridVertex::State::Unrefined);
    }

    sendGridVertex(vertex);

    if (restrictIsAntecessorOfRefinedVertex) {
      dfor2(k) if (restrictToCoarseGrid(k, fineVertexPositionWithinPatch)) {
        logDebug(
          "updateVertexBeforeStore()",
          "set antecessor flag (veto coarsenign) on vertex " << coarseGridVertices[kScalar].toString(
          ) << " due to vertex " << vertex.toString()
        );
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

  logTraceOutWith2Arguments("updateVertexBeforeStore()", vertex.toString(), _id);
}

bool peano4::grid::Spacetree::shouldEraseAdjacencyInformation(
  const GridVertex&                  vertex,
  GridVertex                         coarseGridVertices[TWO_POWER_D],
  tarch::la::Vector<DIMENSIONS, int> fineVertexPositionWithinPatch
) const {
  if (SpacetreeHelper::isVertexAdjacentToLocalSpacetree(vertex, _id, _splitTriggered, _splitting, _joinTriggered, _joining, true, true, false) or vertex.getState() != GridVertex::State::Unrefined) {
    return false;
  }

  logTraceInWith2Arguments("shouldEraseAdjacencyInformation()", vertex.toString(), fineVertexPositionWithinPatch);
  for (int d = 0; d < DIMENSIONS; d++) {
    if (fineVertexPositionWithinPatch(d) != 0 and fineVertexPositionWithinPatch(d) != 3) {
      tarch::la::Vector<DIMENSIONS, int> positionA = fineVertexPositionWithinPatch;
      tarch::la::Vector<DIMENSIONS, int> positionB = fineVertexPositionWithinPatch;

      positionA(d) = 0;
      positionB(d) = 3;

      bool result = shouldEraseAdjacencyInformation(vertex, coarseGridVertices, positionA)
                    and shouldEraseAdjacencyInformation(vertex, coarseGridVertices, positionB);

      logTraceOutWith1Argument("shouldEraseAdjacencyInformation()", result);
      return result;
    }
  }

  fineVertexPositionWithinPatch = fineVertexPositionWithinPatch / 3;

  const auto& adjacentRanks = coarseGridVertices[peano4::utils::dLinearised(fineVertexPositionWithinPatch, 2)].adjacentRanks;
  bool result = std::find(std::begin(adjacentRanks), std::end(adjacentRanks), _id) != std::end(adjacentRanks);
  logTraceOutWith1Argument("shouldEraseAdjacencyInformation()", result);
  return result;
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
  logDebug(
    "createNewPersistentVertex()",
    "inherit "
      << indexToInherit.to_ulong() << "th index from coarse vertex " << vertexToInherit << " into local index "
      << kScalar
  );
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
  _vertexLookupTables.push_back(createVertexLookupTable(fineGridStatesState, cellPositionWithin3x3Patch));
  const auto& lut = _vertexLookupTables.back();

  int                        cachedStackNo = -1;
  stacks::Stack<GridVertex>* cachedStack   = nullptr;

  for (int i = 0; i < TWO_POWER_D; i++) {
    const auto vIndex   = lut.vIdx[i];
    const int  lin      = lut.lin[i];
    auto&      vSlot    = fineGridVertices[lin];
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
        getAdjacentRanksForNewVertex(coarseGridVertices, vPos),
        true
      );
      break;
    case VertexType::Hanging:
      vSlot = GridVertex::create(
        fineGridStatesState.level,
        GridVertex::State::HangingVertex,
        getAdjacentRanksForNewVertex(coarseGridVertices, vPos),
        true
      );
      break;
    case VertexType::Persistent: {
      if (stackNo != cachedStackNo) {
        cachedStack   = _vertexStack.getForPop(_id, stackNo);
        cachedStackNo = stackNo;
      }
      auto* popStack = cachedStack;
      assertion3(not popStack->empty(), _id, stackNo, vIndex);
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
        cachedStack   = _vertexStack.getForPop(_id, stackNo);
        cachedStackNo = stackNo;
      }
      auto* popStack = cachedStack;
      assertion2(not popStack->empty(), stackNo, _id);
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

#if PEANO_DEBUG >= 1
    logDebug(
      "loadVertices()",
      "Handled " << peano4::grid::toString(type) << " vertex " << vIndex << " at " << vPos << ": " << vSlot.toString()
    );

    // const auto x = fineGridStatesState.x + tarch::la::multiplyComponents(
    //   tarch::la::convertScalar<double>(convertToIntegerVector(vIndex)), fineGridStatesState.h
    // );
    // assertionNumericalEquals5(
    //   vSlot.x,
    //   x,
    //   vSlot.toString(),
    //   fineGridStatesState.toString(),
    //   vIndex,
    //   toString(),
    //   _id
    // );
    assertionEquals5(
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
  const AutomatonState&                     fineGridStatesState,
  GridVertex                                coarseGridVertices[TWO_POWER_D],
  GridVertex                                fineGridVertices[TWO_POWER_D],
  const VertexType                          vertexTypes[TWO_POWER_D],
  const tarch::la::Vector<DIMENSIONS, int>& cellPositionWithin3x3Patch,
  GridTraversalObserver&                    observer
) {
  assertion(not _vertexLookupTables.empty());
  const auto lut = _vertexLookupTables.back();
  _vertexLookupTables.pop_back();

  int                        cachedStackNo = -1;
  stacks::Stack<GridVertex>* cachedStack   = nullptr;

  for (int i = 0; i < TWO_POWER_D; i++) {
    const int  vIndex   = lut.vIdx[i];
    const int  lin      = lut.lin[i];
    auto&      vSlot    = fineGridVertices[lin];
    VertexType type     = vertexTypes[vIndex];
    const int  stackNo  = lut.writeStack[i];
    const bool inOut    = lut.inOutWrite[i];

    if (not inOut and type == VertexType::Delete) {
      type = VertexType::Persistent;
    }

    if (type == VertexType::Hanging) {
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
      cachedStack   = _vertexStack.getForPush(_id, stackNo);
      cachedStackNo = stackNo;
    }
    auto* pushStack = cachedStack;
    pushStack->push(std::move(vSlot));
  }
}

bool peano4::grid::Spacetree::doesRankIndexIdentifyHorizontalDataExchange(int rank, bool calledByReceivingProcess)
  const {
  const bool receiverCriteria = (_joining.count(rank) == 0) and (_splitting.count(rank) == 0);
  const bool senderCriteria   = (_joining.count(rank) == 0) and (_splitTriggered.count(rank) == 0)
                              and _spacetreeState != SpacetreeState::Joining;

  const bool mandatoryCriteria = rank != _id and rank != -1 and rank != RankOfPeriodicBoundaryCondition;

  return calledByReceivingProcess ? (mandatoryCriteria and receiverCriteria) : (mandatoryCriteria and senderCriteria);
}

bool peano4::grid::Spacetree::isFaceAlongPeriodicBoundaryCondition(
  GridVertex fineGridVertices[TWO_POWER_D], int faceNumber, bool calledByReceivingProcess
) const {
  logTraceInWith2Arguments("isFaceAlongPeriodicBoundaryCondition()", faceNumber, calledByReceivingProcess);

  int adjacentRanksOfFace[TWO_POWER_D];
  SpacetreeHelper::getAdjacentRanksOfFace(fineGridVertices, faceNumber, calledByReceivingProcess, adjacentRanksOfFace);

  bool isAdjacentToLocalRank              = false;
  bool holdsPeriodicBoundaryConditionFlag = false; // semantics is either has been local or will be local
  for (int i = 0; i < TWO_POWER_D; i++) {
    isAdjacentToLocalRank |= adjacentRanksOfFace[i] == _id;
    holdsPeriodicBoundaryConditionFlag |= adjacentRanksOfFace[i] == RankOfPeriodicBoundaryCondition;
  }

  holdsPeriodicBoundaryConditionFlag &= isAdjacentToLocalRank;

  logTraceOutWith2Arguments(
    "isFaceAlongPeriodicBoundaryCondition()", holdsPeriodicBoundaryConditionFlag, isAdjacentToLocalRank
  );
  return holdsPeriodicBoundaryConditionFlag;
}

int peano4::grid::Spacetree::getNeighbourTrees(
  GridVertex fineGridVertices[TWO_POWER_D], int faceNumber, bool calledByReceivingProcess
) const {
  logTraceInWith3Arguments("getNeighbourTrees()", faceNumber, calledByReceivingProcess, _id);

  int adjacentRanksOfFace[TWO_POWER_D];
  SpacetreeHelper::getAdjacentRanksOfFace(fineGridVertices, faceNumber, calledByReceivingProcess, adjacentRanksOfFace);

  logDebug("getNeighbourTrees()", "face adjacency list=" << adjacentRanksOfFace);

  // Study whether face is local and who neighbours it. See routine
  // documentation of our particular flavour of localness in this
  // case.
  // --------------------------------------------------------------
  bool isAdjacentToLocalRank = false; // semantics is either has been local or will be local
  int  neighbour             = GridTraversalObserver::NoData;
  for (int i = 0; i < TWO_POWER_D; i++) {
    const bool mandatoryCriteria = adjacentRanksOfFace[i] != _id and adjacentRanksOfFace[i] != -1
                                   and adjacentRanksOfFace[i] != RankOfPeriodicBoundaryCondition
                                   and _spacetreeState != SpacetreeState::EmptyRun;

    if (calledByReceivingProcess) {
      isAdjacentToLocalRank |= adjacentRanksOfFace[i] == _id or _splitTriggered.count(adjacentRanksOfFace[i]) == 1
                               or _splitting.count(adjacentRanksOfFace[i]) == 1;

      const bool receiverCriteria = _splitTriggered.count(adjacentRanksOfFace[i]) == 0
                                    and _splitting.count(adjacentRanksOfFace[i]) == 0
                                    and _spacetreeState != SpacetreeState::NewFromSplit;

      if (mandatoryCriteria and receiverCriteria) {
        neighbour = adjacentRanksOfFace[i];
      }
    } else {
      isAdjacentToLocalRank |= adjacentRanksOfFace[i] == _id or _splitTriggered.count(adjacentRanksOfFace[i]) == 1;

      const bool senderCriteria = _splitTriggered.count(adjacentRanksOfFace[i]) == 0;

      if (mandatoryCriteria and senderCriteria) {
        neighbour = adjacentRanksOfFace[i];
      }
    }
  }

  if (not isAdjacentToLocalRank) {
    neighbour = GridTraversalObserver::NoData;
  }

  logTraceOutWith1Argument("getNeighbourTrees()", neighbour);
  return neighbour;
}

std::vector<int> peano4::grid::Spacetree::getNeighbourTrees(const GridVertex& vertex, bool calledByReceivingProcess)
  const {
  std::vector<int> neighbourIds;
  neighbourIds.reserve(TWO_POWER_D);

  const bool isLocalVertex = calledByReceivingProcess ? SpacetreeHelper::isVertexAdjacentToLocalSpacetree(vertex, _id, _splitTriggered, _splitting, _joinTriggered, _joining, true, false, false)
                                                      : SpacetreeHelper::isVertexAdjacentToLocalSpacetree(vertex, _id, _splitTriggered, _splitting, _joinTriggered, _joining, false, true, false);

  if (isLocalVertex) {
    const auto& adjacentRanks = calledByReceivingProcess ? vertex.backupOfAdjacentRanks : vertex.adjacentRanks;
    for (int i = 0; i < TWO_POWER_D; i++) {
      if (doesRankIndexIdentifyHorizontalDataExchange(adjacentRanks[i], calledByReceivingProcess)) {
        neighbourIds.push_back(adjacentRanks[i]);
      }
    }
  }

  std::sort(neighbourIds.begin(), neighbourIds.end());
  auto last = std::unique(neighbourIds.begin(), neighbourIds.end());
  neighbourIds.erase(last, neighbourIds.end());

  return neighbourIds;
}

void peano4::grid::Spacetree::mergeGridVertexAdjacencyListsAtHorizontalDomainBoundary(
  GridVertex& vertex, const GridVertex& inVertex, int neighbour
) {
  //
  // Merge adjacency lists. The neighbour owns some entries
  //
  for (int i = 0; i < TWO_POWER_D; i++) {
    if (vertex.adjacentRanks[i] == neighbour and neighbour != -1) {
      if (inVertex.adjacentRanks[i] != vertex.adjacentRanks[i]) {
        logDebug(
          "mergeGridVertexAdjacencyListsAtHorizontalDomainBoundary()",
          "update "
            << i << "th entry of adjacency data of vertex " << vertex.toString() << " with neighbouring vertex "
            << inVertex.toString() << " on tree " << _id
        );
      }
      vertex.adjacentRanks[i] =  inVertex.adjacentRanks[i];
    }
  }
}

void peano4::grid::Spacetree::mergeGridVertexRefinementStateAtHorizontalDomainBoundary(
  [[maybe_unused]] GridVertex& vertex, [[maybe_unused]] const GridVertex& inVertex, [[maybe_unused]] int neighbour
) {
  assertion4(
    vertex.getState() != GridVertex::State::HangingVertex, inVertex.toString(), vertex.toString(), _id, neighbour
  );
  assertion4(
    inVertex.getState() != GridVertex::State::HangingVertex, inVertex.toString(), vertex.toString(), _id, neighbour
  );
  // assertion6(
  //   tarch::la::equals(vertex.getX(), inVertex.getX())
  //   or
  //   (
  //     tarch::la::contains(vertex.getAdjacentRanks(),RankOfPeriodicBoundaryCondition)
  //     and
  //     (_periodicBC.count()>1 or tarch::la::countEqualEntries(vertex.getX(), inVertex.getX())==DIMENSIONS-static_cast<int>(_periodicBC.count()))
  //   ),
  //   inVertex.toString(), vertex.toString(), _id, neighbour,
  //   tarch::la::countEqualEntries(vertex.getX(), inVertex.getX()), _periodicBC
  // );
  assertionEquals4(vertex.level, inVertex.level, inVertex.toString(), vertex.toString(), _id, neighbour);
  assertion4(
    not(vertex.getState() == GridVertex::State::Refined and inVertex.getState() == GridVertex::State::Erasing),
    inVertex.toString(),
    vertex.toString(),
    _id,
    neighbour
  );
  assertion4(
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
  );

  //
  // Update refinement flags
  //
  assertion3(vertex.getState() != GridVertex::State::Erasing, vertex.toString(), inVertex.toString(), _id);
  assertion3(vertex.getState() != GridVertex::State::Refining, vertex.toString(), inVertex.toString(), _id);

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
    logDebug(
      "mergeGridVertexRefinementStateAtHorizontalDomainBoundary( GridVertex )",
      "set state to " << vertex.toString() << " on tree " << _id << " due to merge with neighbour"
    );
  } else if (OnlyNeighbourHasTriggeredErase || OnlyLocalHasTriggeredErase) {
    vertex.setState(GridVertex::State::EraseTriggered);
    logDebug(
      "mergeGridVertexRefinementStateAtHorizontalDomainBoundary( GridVertex )",
      "set state to " << vertex.toString() << " on tree " << _id << " due to merge with neighbour"
    );
  } else {
    assertion4(vertex.getState() == inVertex.getState(), inVertex.toString(), vertex.toString(), _id, neighbour);
  }
}

void peano4::grid::Spacetree::receiveAndMergeGridVertexAtHorizontalBoundary(GridVertex& vertex) {
  assertion(_spacetreeState != SpacetreeState::NewFromSplit and _spacetreeState != SpacetreeState::EmptyRun);

  logTraceInWith1Argument("receiveAndMergeGridVertexAtHorizontalBoundary()", vertex.toString());

  //
  // Backup adjacency lists
  //
  vertex.backupOfAdjacentRanks = vertex.adjacentRanks;

  const auto& neighbourIds = getNeighbourTrees(vertex, true);
  for (const auto& neighbour : neighbourIds) {
    assertion1(neighbour >= 0, neighbour);
    const int inStack = peano4::parallel::Node::getInstance().getInputStackNumberForHorizontalDataExchange(neighbour);

    assertion4(_vertexStack.holdsStack(_id, inStack), _id, inStack, neighbour, vertex.toString());
    assertion4(not _vertexStack.getForPop(_id, inStack)->empty(), _id, inStack, neighbour, vertex.toString());
    GridVertex inVertex = _vertexStack.getForPop(_id, inStack)->pop();

    mergeGridVertexRefinementStateAtHorizontalDomainBoundary(vertex, inVertex, neighbour);
    mergeGridVertexAdjacencyListsAtHorizontalDomainBoundary(vertex, inVertex, neighbour);
  }

  if (_id == 0 and hasPeriodicBCs(vertex.adjacentRanks)) {
    const auto& node = peano4::parallel::Node::getInstance();
    const auto  outStacks = node.getOutputStacksForPeriodicBoundaryExchange(vertex.adjacentRanks);
    for (const auto& p : outStacks) {
      const auto stackNo = peano4::parallel::Node::getPeriodicBoundaryExchangeInputStackNumberForOutputStack(p);
      assertion4(_id == 0, _id, stackNo.first, stackNo.second, vertex.toString());
      assertion3(not _vertexStack.getForPop(_id, stackNo.first)->empty(), stackNo.first, stackNo.second, _id);
      GridVertex inVertex = _vertexStack.getForPop(_id, stackNo.first)->pop();
      logDebug(
        "receiveAndMergeGridVertexAtHorizontalBoundary()",
        "read periodic BC data from stack " << stackNo.first << ": " << inVertex.toString()
      );
      logDebug(
        "receiveAndMergeGridVertexAtHorizontalBoundary()", "normals of involved symmetry axes: " << stackNo.second
      );
      mergeGridVertexRefinementStateAtHorizontalDomainBoundary(vertex, inVertex, _id);

      dfor2(k)
        assertion3(vertex.adjacentRanks[kScalar] == RankOfPeriodicBoundaryCondition
          or vertex.adjacentRanks[kScalar] == _id
          or vertex.adjacentRanks[kScalar] == -1,
          vertex.toString(),
          inVertex.toString(),
          _id
        );
      enddforx
   }
  }

  logTraceOut("receiveAndMergeGridVertexAtHorizontalBoundary()");
}

void peano4::grid::Spacetree::sendGridVertex(const GridVertex& vertex) {
  logTraceInWith2Arguments("sendGridVertex(GridVertex)", vertex.toString(), _id);

  assertion2(_spacetreeState != SpacetreeState::EmptyRun, _id, peano4::grid::toString(_spacetreeState));

  const auto& outRanks = getNeighbourTrees(vertex, false);

  for (const auto& p : outRanks) {
    //
    // Boundary exchange
    //
    const int stackNo = peano4::parallel::Node::getInstance().getOutputStackNumberForHorizontalDataExchange(p);

    GridVertex vertexCopy = vertex;
    for (int i = 0; i < TWO_POWER_D; i++) {
      int targetRank = vertexCopy.adjacentRanks[i];
      if (_spacetreeState == SpacetreeState::JoinTriggered and targetRank == _id and p != _masterId) {
        vertexCopy.adjacentRanks[i] = _masterId;
      }
    }

    _vertexStack.getForPush(_id, stackNo)->push(std::move(vertexCopy));
  }

  if (_id == 0 and hasPeriodicBCs(vertex.adjacentRanks)) {
    const auto outStacks = peano4::parallel::Node::getInstance().getOutputStacksForPeriodicBoundaryExchange(vertex.adjacentRanks);
    for (const auto& s : outStacks) {
      _vertexStack.getForPush(_id, s.first)->push(vertex);
      logDebug(
        "sendGridVertex(GridVertex)",
        "vertex " << vertex.toString() << " on tree " << _id
                  << " goes to stack " << s.first << " to realise periodic BC"
      );
    }
  }

  logTraceOutWith1Argument("sendGridVertex(GridVertex)", SpacetreeHelper::isVertexAdjacentToLocalSpacetree(vertex, _id, _splitTriggered, _splitting, _joinTriggered, _joining, true, true, false));
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
  assertion1(_spacetreeState != SpacetreeState::EmptyRun and _spacetreeState != SpacetreeState::NewFromSplit and _spacetreeState != SpacetreeState::Joining, toString());

  // Avoid changing topology while adjacency lists are unstable
  bool mayChangeGrid = true;
  UNROLL(DIMENSIONS)
  for (int i = 0; i < TWO_POWER_D; i++) {
    auto thisCellState = coarseGridVertices[i].getState();
    mayChangeGrid &= (thisCellState == GridVertex::State::HangingVertex or
                      thisCellState == GridVertex::State::Unrefined or
                      thisCellState == GridVertex::State::Refined or
                      thisCellState == GridVertex::State::EraseTriggered);
    if (not mayChangeGrid) {
      break;
    }
  }

  if (not mayChangeGrid) {
    return;
  }

  bool stateWantsToRefine = false;
  bool stateWantsToErase = false;
  for (const auto& event : _gridControlEvents) {
    if (event.getRefinementControl() == GridControlEvent::RefinementControl::Refine) {
      bool eventOverlapsState = true;
      for (int d = 0; d < DIMENSIONS; ++d) {
        const auto stateMin = fineGridState.x[d];
        const auto stateMax = stateMin + fineGridState.h[d];
        const auto eventMin = event.offset[d];
        const auto eventMax = eventMin + event.width[d];

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
        if (not (tarch::la::greaterEquals(stateMax, eventMin) and tarch::la::smallerEquals(stateMin, eventMax))) {
          eventOverlapsState = false;
        }
      }

      bool refineToTargetResolution = tarch::la::allGreaterEquals(fineGridState.h, event.h); // If the event's target resolution is smaller, we refine.
      if (refineToTargetResolution and eventOverlapsState) {
        stateWantsToRefine = true;
      }
    } else if (event.getRefinementControl() == GridControlEvent::RefinementControl::Erase) {
      // The check for children here is not really needed, but it makes it easier to debug.
      bool hasChildren = false;
      UNROLL(DIMENSIONS)
      for (int i = 0; i < TWO_POWER_D; ++i) {
        if (fineGridVertices[i].getState() == GridVertex::State::Refined) {
          hasChildren = true;
          break;
        }
      }
      // Only parents (cells that already have children) can be coarsened
      if (not hasChildren) {
        continue;
      }

      bool eventContainsState = true;
      for (int d = 0; d < DIMENSIONS; ++d) {
        const auto stateMin = fineGridState.x[d];
        const auto stateMax = stateMin + fineGridState.h[d];
        const auto eventMin = event.offset[d];
        const auto eventMax = eventMin + event.width[d];

        // Determine if the event contains the state
        //
        // event:  [eventMin --------------------- eventMax]
        // state:       [stateMin ---- stateMax]
        //               ^ contains because:
        //                 stateMin ≥ eventMin    and    stateMax ≤ eventMax
        //
        if (not (tarch::la::greaterEquals(stateMin, eventMin) and tarch::la::smallerEquals(stateMax, eventMax))) {
          eventContainsState = false;
        }
      }
      bool coarsenToTargetResolution = tarch::la::allSmallerEquals(fineGridState.h, event.h); // If the event's target resolution is greater, we coarsen.
      if (coarsenToTargetResolution and eventContainsState) {
        //std::cout << event.toString() << ", " << fineGridState.toString() << std::endl;
        stateWantsToErase = true;
      }
    }
  }

  if (stateWantsToRefine) {
    UNROLL(DIMENSIONS)
    for (int i = 0; i < TWO_POWER_D; ++i) {
      if (fineGridVertices[i].getState() == GridVertex::State::Unrefined and SpacetreeHelper::isVertexAdjacentToLocalSpacetree(fineGridVertices[i], _id, _splitTriggered, _splitting, _joinTriggered, _joining, true, true, false)) {
        fineGridVertices[i].setState(GridVertex::State::RefinementTriggered);
      }
    }
  } else if (stateWantsToErase) {
    UNROLL(DIMENSIONS)
    for (int i = 0; i < TWO_POWER_D; ++i) {
      if (fineGridVertices[i].getState() == GridVertex::State::Refined and SpacetreeHelper::isVertexAdjacentToLocalSpacetree(fineGridVertices[i], _id, _splitTriggered, _splitting, _joinTriggered, _joining, true, true, false)) {
        fineGridVertices[i].setState(GridVertex::State::EraseTriggered);
      }
    }
  }
}

void peano4::grid::Spacetree::descend(const AutomatonState& state, GridVertex vertices[TWO_POWER_D], GridTraversalObserver& observer) {
  logTraceInWith2Arguments("descend()", state.toString(), _id);
  assertion(SpacetreeHelper::isSpacetreeNodeRefined(vertices));
  const bool isCoarseLocal = SpacetreeHelper::isSpacetreeNodeLocal(vertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining, false, false);

  AutomatonState fineGridStates[THREE_POWER_D];
  AutomatonState::refineState(state, fineGridStates);

  peano4::utils::LoopDirection loopDirection = PeanoCurve::getLoopDirection(state);
  zfor3(k, loopDirection)
    const auto kLinearised = utils::dLinearised(k, 3);

    VertexType vertexTypes[TWO_POWER_D];
    getVertexTypes(vertices, k, vertexTypes);

    GridVertex fineGridVertices[TWO_POWER_D];
    loadVertices(fineGridStates[kLinearised], vertices, fineGridVertices, vertexTypes, k, observer);

    mergeCellFromWorkerWithMasterThroughoutJoin(vertices, fineGridVertices);

    // Flag areas which fork off further
    const bool isFineLocal = SpacetreeHelper::isSpacetreeNodeLocal(fineGridVertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining, false, false);
    if (isFineLocal and not isCoarseLocal) {
      markVerticesAroundForkedCell(vertices, fineGridVertices);
    } else if (not isFineLocal and isCoarseLocal) {
      markVerticesAroundForkedCell(vertices, fineGridVertices);
    }

    if (isFineLocal) {
      evaluateGridControlEvents(fineGridStates[kLinearised], vertices, fineGridVertices);
    }

    splitCellTopDown(vertices, fineGridVertices);

    GridTraversalEvent enterNodeGridTraversalEvent = _gridTraversalEventGenerator.createEnterNodeTraversalEvent(
      vertices,
      fineGridVertices,
      vertexTypes,
      fineGridStates[kLinearised],
      _splitTriggered,
      _splitting,
      _joinTriggered,
      _joining,
      _hasSplit,
      k,
      _spacetreeState == SpacetreeState::Running
    );

    observer.loadVertexData(enterNodeGridTraversalEvent);
    observer.loadFaceData(enterNodeGridTraversalEvent);
    observer.loadCellData(enterNodeGridTraversalEvent);

    if (_spacetreeState != SpacetreeState::EmptyRun and _spacetreeState != SpacetreeState::NewFromSplit and _spacetreeState != SpacetreeState::Joined) {
      receiveAndMergeUserData(fineGridStates[kLinearised], observer, enterNodeGridTraversalEvent, fineGridVertices);
    }

    observer.enterNode(enterNodeGridTraversalEvent);

    _statistics.minH = tarch::la::min(_statistics.minH, 1.0 / 3.0 * state.h);

    //
    // DFS
    //
    if (SpacetreeHelper::isSpacetreeNodeRefined(fineGridVertices)) {
      if (isFineLocal) {
        _statistics.numberOfLocalRefinedCells++;
        incrementNumberOfAdjacentRefinedLocalCells(fineGridVertices);
      } else {
        _statistics.numberOfRemoteRefinedCells++;
      }
      descend(fineGridStates[kLinearised], fineGridVertices, observer);
    } else {
      if (SpacetreeHelper::isSpacetreeNodeLocal(fineGridVertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining, true, true)) {
        _statistics.numberOfLocalUnrefinedCells++;
      } else {
        _statistics.numberOfRemoteUnrefinedCells++;
      }
    }

    GridTraversalEvent leaveNodeGridTraversalEvent = _gridTraversalEventGenerator.createLeaveNodeTraversalEvent(
      enterNodeGridTraversalEvent,
      vertices,
      fineGridVertices,
      vertexTypes,
      fineGridStates[kLinearised],
      _splitTriggered,
      _splitting,
      _joinTriggered,
      _joining,
      _hasSplit,
      k,
      _spacetreeState == SpacetreeState::Running
    );

    observer.leaveNode(leaveNodeGridTraversalEvent);

    if (_spacetreeState != SpacetreeState::EmptyRun and _spacetreeState != SpacetreeState::Joined) {
      sendUserData(fineGridStates[kLinearised], observer, leaveNodeGridTraversalEvent, fineGridVertices);
    }

    observer.storeCellData(leaveNodeGridTraversalEvent);
    observer.storeFaceData(leaveNodeGridTraversalEvent);
    observer.storeVertexData(leaveNodeGridTraversalEvent);

    splitOrJoinCellBottomUp(vertices, fineGridVertices);

    storeVertices(fineGridStates[kLinearised], vertices, fineGridVertices, vertexTypes, k, observer);
  endzfor

  logTraceOut("descend()");
}

void peano4::grid::Spacetree::receiveAndMergeUserData(
  const AutomatonState&                   state,
  GridTraversalObserver&                  observer,
  const GridTraversalEvent&               enterCellTraversalEvent,
  GridVertex                              fineGridVertices[TWO_POWER_D]
) {
  assertion3(
    _spacetreeState != SpacetreeState::EmptyRun and _spacetreeState != SpacetreeState::NewFromSplit
      and _spacetreeState != SpacetreeState::Joined,
    state.toString(),
    peano4::grid::toString(_spacetreeState),
    _id
  );

  for (int i = 0; i < TWO_POWER_D; i++) {
    int inVertexStack              = enterCellTraversalEvent.getVertexDataFrom(i);
    int inVertexPositionWithinCell = enterCellTraversalEvent.getVertexDataTo(i);

    if (peano4::grid::PeanoCurve::isInOutStack(inVertexStack)) {
      assertion(inVertexStack != peano4::grid::GridTraversalObserver::CreateOrDestroyPersistentGridEntity);
      assertion(inVertexStack != peano4::grid::GridTraversalObserver::CreateOrDestroyHangingGridEntity);

      if (enterCellTraversalEvent.getIsVertexLocal(inVertexPositionWithinCell)) {
        const auto& neighbours = getNeighbourTrees(fineGridVertices[inVertexPositionWithinCell], true);
        for (const auto& p : neighbours) {
          logDebug(
            "receiveAndMergeUserData()",
            "receive neighbour data from neighbour "
              << p << " for vertex " << fineGridVertices[inVertexPositionWithinCell].toString()
          );
          const int fromStack = peano4::parallel::Node::getInputStackNumberForHorizontalDataExchange(p);
          observer.receiveAndMergeVertexData(
            inVertexPositionWithinCell,
            fromStack, // Rank
            stacks::SendReceiveContext::BoundaryExchange,
            enterCellTraversalEvent
          );
        }

        if (_id == 0 and hasPeriodicBCs(fineGridVertices[inVertexPositionWithinCell].adjacentRanks)) {
          const auto& node = peano4::parallel::Node::getInstance();
          const auto  outStacks = node.getOutputStacksForPeriodicBoundaryExchange(fineGridVertices[inVertexPositionWithinCell].adjacentRanks);
          for (const auto& p : outStacks) {
            const auto inId = node.getPeriodicBoundaryExchangeInputStackNumberForOutputStack(p);
            observer.receiveAndMergeVertexData(
              inVertexPositionWithinCell,
              inId.first,
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
    if (peano4::grid::PeanoCurve::isInOutStack(inFaceStack)) {
      assertion(inFaceStack != peano4::grid::GridTraversalObserver::CreateOrDestroyPersistentGridEntity);
      assertion(inFaceStack != peano4::grid::GridTraversalObserver::CreateOrDestroyHangingGridEntity);

      if (enterCellTraversalEvent.getIsFaceLocal(inFacePositionWithinCell)) {
        int neighbour = getNeighbourTrees(fineGridVertices, inFacePositionWithinCell, true);
        if (neighbour >= 0) {
          const int fromStack = peano4::parallel::Node::getInputStackNumberForHorizontalDataExchange(neighbour);
          observer.receiveAndMergeFaceData(
            inFacePositionWithinCell,
            fromStack,
            stacks::SendReceiveContext::BoundaryExchange,
            enterCellTraversalEvent
          );
        }

        if (isFaceAlongPeriodicBoundaryCondition(fineGridVertices, inFacePositionWithinCell, true)) {
          const int fromStack = peano4::parallel::Node::getPeriodicBoundaryExchangeInputStackNumberForOutputStack(
            peano4::parallel::Node::getOutputStackForPeriodicBoundaryExchange(inFacePositionWithinCell)
          );

          logDebug("receiveAndMergeUserData()", "Will merge face from periodic BC stack #" << fromStack << " with local face");

          observer.receiveAndMergeFaceData(
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
  assertion4(
    _spacetreeState != SpacetreeState::EmptyRun and _spacetreeState != SpacetreeState::Joined,
    peano4::grid::toString(_spacetreeState),
    state.toString(),
    leaveCellTraversalEvent.toString(),
    _id
  );

  for (int i = 0; i < TWO_POWER_D; i++) {
    int outVertexStack              = leaveCellTraversalEvent.getVertexDataTo(i);
    int outVertexPositionWithinCell = leaveCellTraversalEvent.getVertexDataFrom(i);

    if (peano4::grid::PeanoCurve::isInOutStack(outVertexStack)) {
      assertion(outVertexStack != peano4::grid::GridTraversalObserver::CreateOrDestroyPersistentGridEntity);
      assertion(outVertexStack != peano4::grid::GridTraversalObserver::CreateOrDestroyHangingGridEntity);

      if (leaveCellTraversalEvent.getIsVertexLocal(outVertexPositionWithinCell)) {
        const auto& neighbours = getNeighbourTrees(fineGridVertices[outVertexPositionWithinCell], false);

        for (const auto& p : neighbours) {
          logDebug(
            "sendUserData()",
            "send local vertex data of "
              << fineGridVertices[outVertexPositionWithinCell].toString() << " from stack " << outVertexStack
              << " on tree " << _id << " to neighbour " << p << ". Position within cell=" << outVertexPositionWithinCell
          );
          const int toStack = peano4::parallel::Node::getOutputStackNumberForHorizontalDataExchange(p);
          observer.sendVertexData(
            outVertexPositionWithinCell,
            toStack,
            stacks::SendReceiveContext::BoundaryExchange,
            leaveCellTraversalEvent
          );
        }

        if (_id == 0 and hasPeriodicBCs(fineGridVertices[outVertexPositionWithinCell].adjacentRanks)) {
          const auto outStacks = peano4::parallel::Node::getInstance().getOutputStacksForPeriodicBoundaryExchange(fineGridVertices[outVertexPositionWithinCell].adjacentRanks);
          for (const auto& stackNo : outStacks) {
            logDebug(
              "sendUserData()",
              "vertex "
                << fineGridVertices[outVertexPositionWithinCell].toString() << " on tree " << _id << " goes to stack "
                << stackNo.first << " to realise periodic BC"
            );

            observer.sendVertexData(
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
            logDebug(
              "sendUserData()",
              "stream local vertex data of "
                << fineGridVertices[outVertexPositionWithinCell].toString() << " from stack " << outVertexStack
                << " on tree " << _id << " to upcoming worker " << p << ". Position within cell="
                << outVertexPositionWithinCell << ". event=" << leaveCellTraversalEvent.toString()
            );
            const int toStack = peano4::parallel::Node::getOutputStackNumberForVerticalDataExchange(p);
            observer.sendVertexData(
              outVertexPositionWithinCell,
              toStack,
              stacks::SendReceiveContext::ForkDomain,
              leaveCellTraversalEvent
            );
          } else {
            logDebug(
              "sendUserData()",
              "skip vertex data of " << fineGridVertices[outVertexPositionWithinCell].toString() << " on tree " << _id
            );
          }
        }
      }
    }
  }

  for (int i = 0; i < TWO_TIMES_D; i++) {
    int outFaceStack              = leaveCellTraversalEvent.getFaceDataTo(i);
    int outFacePositionWithinCell = leaveCellTraversalEvent.getFaceDataFrom(i);

    if (peano4::grid::PeanoCurve::isInOutStack(outFaceStack)) {
      assertion(outFaceStack != peano4::grid::GridTraversalObserver::CreateOrDestroyPersistentGridEntity);
      assertion(outFaceStack != peano4::grid::GridTraversalObserver::CreateOrDestroyHangingGridEntity);
      if (leaveCellTraversalEvent.getIsFaceLocal(outFacePositionWithinCell)) {
        int neighbour = getNeighbourTrees(fineGridVertices, outFacePositionWithinCell, false);
        if (neighbour >= 0) {
          logDebug(
            "sendUserData()",
            "send local face from stack "
              << outFaceStack << " of tree " << _id << " to neighbour " << neighbour
              << ". Position within cell=" << outFacePositionWithinCell
          );

          const int toStack = peano4::parallel::Node::getOutputStackNumberForHorizontalDataExchange(neighbour);
          observer.sendFaceData(
            outFacePositionWithinCell,
            toStack,
            stacks::SendReceiveContext::BoundaryExchange,
            leaveCellTraversalEvent
          );
        }

        if (isFaceAlongPeriodicBoundaryCondition(fineGridVertices, outFacePositionWithinCell, false)) {
          const int toStack = peano4::parallel::Node::getOutputStackForPeriodicBoundaryExchange(
            outFacePositionWithinCell
          );

          logDebug(
            "sendUserData()",
            "send local face from stack " << outFaceStack << " of tree " << _id << " to periodic BC stack #" << toStack
          );

          observer.sendFaceData(
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
          logDebug(
            "sendUserData()",
            "stream local face from stack "
              << outFaceStack << " of tree " << _id << " to new worker " << p
              << ". Position within cell=" << outFacePositionWithinCell
          );

          const int toStack = peano4::parallel::Node::getOutputStackNumberForVerticalDataExchange(p);
          observer.sendFaceData(
            outFacePositionWithinCell,
            toStack,
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
      logDebug(
        "sendUserData()",
        "stream local cell of tree "
          << _id << " to new worker " << p << ". Left bottom vertex=" << fineGridVertices[0].toString()
      );

      const int toStack = peano4::parallel::Node::getOutputStackNumberForVerticalDataExchange(p);
      observer.sendCellData(toStack, stacks::SendReceiveContext::ForkDomain, leaveCellTraversalEvent);
    }
  }
}

void peano4::grid::Spacetree::splitOrJoinCellBottomUp(GridVertex coarseGridVertices[TWO_POWER_D], GridVertex fineGridVertices[TWO_POWER_D]) {
  [[maybe_unused]] constexpr int NoSplit = -1;

  if (not _splitTriggered.empty() and _splitTriggered.begin()->second.mode == SplitInstruction::Mode::BottomUp) {
    const bool isSplitCandidate = SpacetreeHelper::isSpacetreeNodeSplitCandidate(coarseGridVertices, fineGridVertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining);
    const bool isRefined        = SpacetreeHelper::isSpacetreeNodeRefined(fineGridVertices);

    logDebug(
      "splitOrJoinCellBottomUp()",
      "Checked cell for bottom-up splits (first target tree=" << _splitTriggered.begin()->first
        << "). IsRefined=" << isRefined << ", IsSplitCandidate=" << isSplitCandidate
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
      assertionMsg(false, "Should not be entered as all combination are covered already");
    }
  } else if (not _splitTriggered.empty() and _splitTriggered.begin()->second.mode == SplitInstruction::Mode::AggressiveTopDown and not _splittedCells.empty()
  ) {
    int targetSpacetreeId = _splittedCells.back();
    _splittedCells.pop_back();

    logInfo(
      "splitOrJoinCellBottomUp()",
      "Checked cell for top-down splits (first split " << _splitTriggered.begin()->first << ")"
    );
    assertion(SpacetreeHelper::isSpacetreeNodeLocal(fineGridVertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining, false, false));
    assertion(SpacetreeHelper::isSpacetreeNodeLocal(coarseGridVertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining, false, false));
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
      assertion(SpacetreeHelper::isSpacetreeNodeLocal(fineGridVertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining, false, false));
      assertion(SpacetreeHelper::isSpacetreeNodeLocal(coarseGridVertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining, false, false));
      int parentId = _splittedCells.back();
      _splittedCells.push_back(parentId);
    } else if (isParentSplit and not isRefined) {
      assertion(SpacetreeHelper::isSpacetreeNodeLocal(fineGridVertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining, false, false));
      assertion(SpacetreeHelper::isSpacetreeNodeLocal(coarseGridVertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining, false, false));
      updateSplittingCounter(targetSpacetreeId);
      int parentId = _splittedCells.back();
      _splittedCells.push_back(parentId);
    } else if (not isParentSplit and isSplitCandidate and isRefined) {
      assertion(SpacetreeHelper::isSpacetreeNodeLocal(fineGridVertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining, false, false));
      assertion(SpacetreeHelper::isSpacetreeNodeLocal(coarseGridVertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining, false, false));
      logDebug(
        "splitCellTopDown()",
        "Refined cell at " << fineGridVertices[0].toString() << " should go with all children to tree " << targetSpacetreeId
      );
      _splittedCells.push_back(targetSpacetreeId);
    } else if (not isParentSplit and isSplitCandidate and not isRefined) {
      updateSplittingCounter(targetSpacetreeId);
      assertion(SpacetreeHelper::isSpacetreeNodeLocal(fineGridVertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining, false, false));
      assertion(SpacetreeHelper::isSpacetreeNodeLocal(coarseGridVertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining, false, false));
      _splittedCells.push_back(targetSpacetreeId);
    }
    logDebug("splitCellTopDown()", "#SplittedCells=" << _splittedCells.size());
  }
}

void peano4::grid::Spacetree::mergeCellFromWorkerWithMasterThroughoutJoin(GridVertex coarseGridVertices[TWO_POWER_D], GridVertex fineGridVertices[TWO_POWER_D]) {
  for (const auto& worker : _joining) {
    if (SpacetreeHelper::isSpacetreeNodeLocal(coarseGridVertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining, false, false) and SpacetreeHelper::getTreeOwningSpacetreeNode(fineGridVertices, _id, _splitTriggered, _splitting, _joinTriggered, _joining) == worker) {
      logDebug(
        "mergeCellFromWorkerWithMasterThroughoutJoin()",
        "Cell from worker " << worker << " is merged into master " << _id
      );
      updateVertexRanksWithinCell(fineGridVertices, _id);
    } else {
      for (int i = 0; i < TWO_POWER_D; i++) {
        logDebug("mergeCellFromWorkerWithMasterThroughoutJoin()", "- " << coarseGridVertices[i].toString());
      }
      for (int i = 0; i < TWO_POWER_D; i++) {
        logDebug("mergeCellFromWorkerWithMasterThroughoutJoin()", "- " << fineGridVertices[i].toString());
      }
    }
  }
}

void peano4::grid::Spacetree::updateSplittingCounter(int treeId) {
  for (auto& p : _splitTriggered) {
    if (p.first == treeId) {
      p.second.numberOfFineGridCells = std::max(p.second.numberOfFineGridCells - 1, 0);
      break;
    }
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
  dfor2(k) assertion1(
    coarseGridVertices[kScalar].getState() != GridVertex::State::HangingVertex, coarseGridVertices[kScalar].toString()
  );
  coarseGridVertices[kScalar].setFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep, true);
  coarseGridVertices[kScalar].setFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep, true);
  fineGridVertices[kScalar].setFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep, true);
  enddforx
}

void peano4::grid::Spacetree::split(int newSpacetreeId, const SplitInstruction& instruction) {
  assertion3(
    instruction.numberOfFineGridCells == std::numeric_limits<int>::max() or maySplit(),
    newSpacetreeId,
    instruction,
    toString()
  );
  assertion3(instruction.numberOfFineGridCells != std::numeric_limits<int>::max(), newSpacetreeId, instruction, toString());
  assertion3(instruction.numberOfFineGridCells >= 1, newSpacetreeId, instruction, toString());

  _splitTriggered.insert(std::pair<int, SplitInstruction>(newSpacetreeId, instruction));

  auto firstMode = _splitTriggered.begin()->second.mode;
  for (auto& p : _splitTriggered) {
    if (p.second.mode != firstMode) {
      logWarning(
        "split()",
        "Split of tree "
          << p.first << " is of type " << p.second.mode << " while other splits have mode " << firstMode
          << ". Switch all splits to " << firstMode
      );
      p.second.mode = firstMode;
    }
  }
}

std::string peano4::grid::Spacetree::toString() const {
  std::ostringstream msg;
  msg << "(Id=" << _id << ",MasterId=" << _masterId << ",State=" << peano4::grid::toString(_spacetreeState)
      << ",Statistics=" << _statistics;

  if (not _joinTriggered.empty()) {
    msg << ",JoinTriggered={";
    for (const auto& p : _joinTriggered) {
      msg << "(" << p << ")";
    }
    msg << "}";
  }
  if (not _joining.empty()) {
    msg << ",Joining={";
    for (const auto& p : _joining) {
      msg << "(" << p << ")";
    }
    msg << "}";
  }

  if (not _splitTriggered.empty()) {
    msg << ",SplitTriggered={";
    for (const auto& p : _splitTriggered) {
      msg << "(" << p.first << "," << p.second << ")";
    }
    msg << "}";
  }
  if (not _splitting.empty()) {
    msg << ",Splitting={";
    for (const auto& p : _splitting) {
      msg << "(" << p << ")";
    }
    msg << "}";
  }
  msg << ",Stacks:" << _vertexStack.toString() << ")";
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
  assertion1(mayJoinWithMaster(), _id);
  _spacetreeState = SpacetreeState::JoinTriggered;
  assertion(_splitTriggered.empty());
  assertion(_splitting.empty());
  assertion(_splittedCells.empty());
  assertion(_joining.empty());
}

void peano4::grid::Spacetree::joinWithWorker(int id) {
  logInfo("joinWithWorker()", "Merge tree " << id << " into tree " << _id);
  assertion2(_joinTriggered.count(id) == 0, id, _id);
  _joinTriggered.insert(id);
  assertion2(_splitTriggered.empty(), id, toString());
  assertion2(_splitting.empty(), id, toString());
}

bool peano4::grid::Spacetree::isInvolvedInJoinOrFork() const {
  return not _joinTriggered.empty() or not _joining.empty() or not _splitTriggered.empty() or not _splitting.empty();
}
