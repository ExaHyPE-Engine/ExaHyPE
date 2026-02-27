// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridTraversalEventGenerator.h"

#include "CellType.h"
#include "FaceType.h"
#include "GridTraversalEvent.h"
#include "GridTraversalObserver.h"
#include "PeanoCurve.h"
#include "Spacetree.h"
#include "SpacetreeHelper.h"
#include "peano4/utils/Loop.h"

tarch::logging::Log peano4::grid::GridTraversalEventGenerator::_log("peano4::grid::GridTraversalEventGenerator");

peano4::grid::GridTraversalEventGenerator::GridTraversalEventGenerator(int id):
  _treeNumber(id) {}

peano4::grid::GridTraversalEvent peano4::grid::GridTraversalEventGenerator::createEnterNodeTraversalEvent(
  GridVertex                                coarseGridVertices[TWO_POWER_D],
  GridVertex                                fineGridVertices[TWO_POWER_D],
  const VertexType                          vertexTypes[TWO_POWER_D],
  const AutomatonState&                     state,
  const SplitSpecification&                 splitTriggered,
  const std::set<int>&                      splitting,
  const std::set<int>&                      joinTriggered,
  const std::set<int>&                      joining,
  const std::set<int>&                      hasSplit,
  const tarch::la::Vector<DIMENSIONS, int>& relativePositionToFather,
  bool                                      spacetreeStateIsRunning
) const {
  GridTraversalEvent event;
  event.level = state.level;
  event.setX(state.x + state.h * 0.5);
  event.setH(state.h);

  event.setHasBeenRefined(GridVertex::haveVerticesBeenRefined(fineGridVertices).to_ulong());
  event.setWillBeRefined(GridVertex::willVerticesBeRefined(fineGridVertices).to_ulong());
  event.setRelativePositionToFather(relativePositionToFather);

  event.setIsCellLocal(
    SpacetreeHelper::
      isSpacetreeNodeLocal(fineGridVertices, _treeNumber, splitTriggered, splitting, joinTriggered, joining, true, true)
  );
  event.setIsParentCellLocal(
    state.level > 1
    and SpacetreeHelper::
      isSpacetreeNodeLocal(coarseGridVertices, _treeNumber, splitTriggered, splitting, joinTriggered, joining, true, true)
  );
  event.setIsFaceLocal(
    SpacetreeHelper::areFacesLocal(fineGridVertices, _treeNumber, splitTriggered, splitting, joinTriggered, joining)
      .to_ulong()
  );
  event.setIsVertexLocal(
    SpacetreeHelper::areVerticesLocal(fineGridVertices, _treeNumber, splitTriggered, splitting, joinTriggered, joining)
      .to_ulong()
  );
  event.setIsParentVertexLocal(
    SpacetreeHelper::areVerticesLocal(coarseGridVertices, _treeNumber, splitTriggered, splitting, joinTriggered, joining)
      .to_ulong()
  );
  event.setIsVertexAdjacentToParallelDomainBoundary(
    SpacetreeHelper::areVerticesAdjacentToParallelDomainBoundary(
      fineGridVertices,
      _treeNumber,
      splitTriggered,
      splitting,
      joinTriggered,
      joining
    )
      .to_ulong()
  );
  event.setIsFaceAdjacentToParallelDomainBoundary(
    SpacetreeHelper::areFacesAdjacentToParallelDomainBoundary(
      fineGridVertices,
      _treeNumber,
      splitTriggered,
      splitting,
      joinTriggered,
      joining,
      false
    )
      .to_ulong()
  );

  // 3^D adjacency flags
  event.setIsAdjacentCellLocal(0);
  const auto& idxTbl = SpacetreeHelper::getPatch3x3IndexTable().idx;

  for (int k = 0; k < TWO_POWER_D; ++k) {
    const GridVertex& v = fineGridVertices[k];
    const auto        s = v.getState();

    if (s != GridVertex::State::HangingVertex) {
      event.setIsVertexParentOfSubtree(v.getFlag(GridVertex::HasBeenParentOfSubtreeVertexInPreviousTreeSweep));

      const auto& adj = v.adjacentRanks;
      for (int j = 0; j < TWO_POWER_D; ++j) {
        const int idx        = idxTbl[k][j];
        const int owningTree = adj[j];
        event.setIsAdjacentCellLocal(idx, owningTree == _treeNumber);
      }
    }
  }

  const CellType cellType          = getCellType(vertexTypes);
  // If the cell is going away, we do not push hanging data keyed by this cell’s in/out stack number
  const bool     cellExistedBefore = spacetreeStateIsRunning
                                 and (cellType == CellType::Persistent or cellType == CellType::Delete);
  VertexType previousVertexTypes[TWO_POWER_D];
  for (int k = 0; k < TWO_POWER_D; ++k) {
    previousVertexTypes[k] = VertexType::Persistent;
  }
  if (cellExistedBefore) {
    getVertexTypesFromPreviousTreeSweep(coarseGridVertices, relativePositionToFather, previousVertexTypes);
  }

  const std::bitset<DIMENSIONS> coordinates = PeanoCurve::getFirstVertexIndex(state);
  for (int i = 0; i < TWO_POWER_D; i++) {
    const std::bitset<DIMENSIONS> vertexIndex(coordinates ^ std::bitset<DIMENSIONS>(i));
    const int                     stackNumber    = PeanoCurve::getVertexReadStackNumber(state, vertexIndex);
    const int                     vertexPosition = vertexIndex.to_ulong();

    event.setVertexDataTo(i, vertexPosition);
    event.setIsVertexHanging(vertexPosition, false);

    const auto type = fineGridVertices[vertexPosition].getState();

    if (type == GridVertex::State::HangingVertex) {
      event.setIsVertexHanging(vertexPosition, true);
      if (cellExistedBefore and previousVertexTypes[vertexPosition] == VertexType::Hanging) {
        event.setVertexDataFrom(i, PeanoCurve::getCellReadStackNumber(state));
      } else {
        event.setVertexDataFrom(i, GridTraversalObserver::CreateOrDestroyGridEntity);
      }
      continue;
    }

    bool willBeNoData = PeanoCurve::isInOutStack(stackNumber) and not event.getIsVertexLocal(vertexPosition);

    if (willBeNoData and not hasSplit.empty()) {
      const auto& adj = fineGridVertices[vertexPosition].adjacentRanks;
      for (int j = 0; j < TWO_POWER_D; ++j) {
        const int r = adj[j];
        if (hasSplit.count(r)) {
          willBeNoData = false;
          break;
        }
      }
    }

    if (willBeNoData) {
      event.setVertexDataFrom(i, GridTraversalObserver::NoData);
      continue;
    }

    switch (type) {
    case GridVertex::State::HangingVertex:
      assertion(false);
      break;
    case GridVertex::State::New: {
      if (PeanoCurve::isInOutStack(stackNumber)) {
        event.setVertexDataFrom(i, GridTraversalObserver::CreateOrDestroyGridEntity);
      } else {
        event.setVertexDataFrom(i, stackNumber);
      }
    } break;
    case GridVertex::State::Unrefined:
    case GridVertex::State::Refined:
    case GridVertex::State::RefinementTriggered:
    case GridVertex::State::Refining:
    case GridVertex::State::EraseTriggered:
    case GridVertex::State::Erasing:
    case GridVertex::State::Delete:
      event.setVertexDataFrom(i, stackNumber);
      break;
    }
  }

  for (int i = 0; i < TWO_TIMES_D; i++) {
    const int faceIndex   = PeanoCurve::getFaceNumberAlongCurve(state, i);
    const int stackNumber = PeanoCurve::getFaceReadStackNumber(state, faceIndex);

    event.setFaceDataTo(i, faceIndex);
    event.setIsFaceHanging(faceIndex, false);

    const FaceType type = getFaceType(vertexTypes, faceIndex);

    if (type == FaceType::Hanging) {
      event.setIsFaceHanging(faceIndex, true);
      const FaceType previousType = cellExistedBefore ? getFaceType(previousVertexTypes, faceIndex)
                                                      : FaceType::Persistent;
      if (cellExistedBefore and previousType == FaceType::Hanging) {
        event.setFaceDataFrom(i, PeanoCurve::getCellReadStackNumber(state));
      } else {
        event.setFaceDataFrom(i, GridTraversalObserver::CreateOrDestroyGridEntity);
      }
      continue;
    }

    bool willBeNoData = PeanoCurve::isInOutStack(stackNumber) and not event.getIsFaceLocal(faceIndex);

    if (willBeNoData and not hasSplit.empty()) {
      int adjacency[TWO_POWER_D];
      // Enter-event uses current adjacency (calledByReceivingProcess = false)
      SpacetreeHelper::
        getAdjacentRanksOfFace(fineGridVertices, faceIndex, /*calledByReceivingProcess*/ false, adjacency);
      for (int k = 0; k < TWO_POWER_D; ++k) {
        const int r = adjacency[k];
        if (r >= 0 and hasSplit.count(r)) {
          willBeNoData = false;
          break;
        }
      }
    }

    if (willBeNoData) {
      event.setFaceDataFrom(i, GridTraversalObserver::NoData);
      continue;
    }

    switch (type) {
    case FaceType::Hanging:
      assertion(false);
      break;
    case FaceType::New: {
      if (PeanoCurve::isInOutStack(stackNumber)) {
        event.setFaceDataFrom(i, GridTraversalObserver::CreateOrDestroyGridEntity);
      } else {
        event.setFaceDataFrom(i, stackNumber);
      }
    } break;
    case FaceType::Persistent:
    case FaceType::Delete:
      event.setFaceDataFrom(i, stackNumber);
      break;
    }
  }

  const int stackNumber = PeanoCurve::getCellReadStackNumber(state);
  switch (cellType) {
  case CellType::New:
    event.setCellData(GridTraversalObserver::CreateOrDestroyGridEntity);
    break;
  case CellType::Persistent:
    event.setCellData(stackNumber);
    break;
  case CellType::Delete:
    event.setCellData(stackNumber);
    break;
  }

  bool mayResetToNoData = PeanoCurve::isInOutStack(event.getCellData()) and not event.getIsCellLocal();
  if (mayResetToNoData and not hasSplit.empty()) {
    const auto owner = SpacetreeHelper::
      getTreeOwningSpacetreeNode(fineGridVertices, _treeNumber, splitTriggered, splitting, joinTriggered, joining);
    if (hasSplit.count(owner)) {
      mayResetToNoData = false;
    }
  }
  if (mayResetToNoData) {
    event.setCellData(GridTraversalObserver::NoData);
  }

#if PEANO_DEBUG >= 1
  for (int i = 0; i < TWO_POWER_D; i++) {
    assertion1(event.getIsVertexLocal(i) or not event.getIsVertexAdjacentToParallelDomainBoundary(i), event.toString());
  }
  for (int i = 0; i < TWO_TIMES_D; i++) {
    assertion1(event.getIsFaceLocal(i) or not event.getIsFaceAdjacentToParallelDomainBoundary(i), event.toString());
  }
#endif

  return event;
}

peano4::grid::GridTraversalEvent peano4::grid::GridTraversalEventGenerator::createLeaveNodeTraversalEvent(
  const GridTraversalEvent&                 enterNodeTraversalEvent,
  GridVertex                                coarseGridVertices[TWO_POWER_D],
  GridVertex                                fineGridVertices[TWO_POWER_D],
  const VertexType                          vertexTypes[TWO_POWER_D],
  const AutomatonState&                     state,
  const SplitSpecification&                 splitTriggered,
  const std::set<int>&                      splitting,
  const std::set<int>&                      joinTriggered,
  const std::set<int>&                      joining,
  const std::set<int>&                      hasSplit,
  const tarch::la::Vector<DIMENSIONS, int>& relativePositionToFather,
  bool                                      spacetreeStateIsRunning
) const {
  GridTraversalEvent leaveNodeTraversalEvent = enterNodeTraversalEvent;

  leaveNodeTraversalEvent.setIsVertexAdjacentToParallelDomainBoundary(
    SpacetreeHelper::areVerticesAdjacentToParallelDomainBoundary(
      fineGridVertices,
      _treeNumber,
      splitTriggered,
      splitting,
      joinTriggered,
      joining
    )
      .to_ulong()
  );
  leaveNodeTraversalEvent.setIsFaceAdjacentToParallelDomainBoundary(
    SpacetreeHelper::areFacesAdjacentToParallelDomainBoundary(
      fineGridVertices,
      _treeNumber,
      splitTriggered,
      splitting,
      joinTriggered,
      joining,
      true
    )
      .to_ulong()
  );

  const CellType cellType = getCellType(vertexTypes);

  const std::bitset<DIMENSIONS> coordinates = PeanoCurve::getFirstVertexIndex(state);
  for (int i = 0; i < TWO_POWER_D; i++) {
    const std::bitset<DIMENSIONS> vertexIndex(coordinates ^ std::bitset<DIMENSIONS>(i));
    const int                     stackNumber       = PeanoCurve::getVertexWriteStackNumber(state, vertexIndex);
    const int                     vertexPosition    = vertexIndex.to_ulong();
    const bool                    wasHangingOnEnter = enterNodeTraversalEvent.getIsVertexHanging(vertexPosition);

    leaveNodeTraversalEvent.setVertexDataFrom(i, vertexPosition);
    leaveNodeTraversalEvent.setIsVertexHanging(vertexPosition, false);

    const auto type         = fineGridVertices[vertexPosition].getState();
    const bool isHangingNow = (type == GridVertex::State::HangingVertex);

    if (wasHangingOnEnter and not isHangingNow) {
      leaveNodeTraversalEvent.setIsVertexHanging(vertexPosition, true);
      leaveNodeTraversalEvent.setVertexDataTo(i, GridTraversalObserver::CreateOrDestroyGridEntity);
      continue;
    }

    if (isHangingNow) {
      leaveNodeTraversalEvent.setIsVertexHanging(vertexPosition, true);
      if (cellType == CellType::Delete) {
        leaveNodeTraversalEvent.setVertexDataTo(i, GridTraversalObserver::CreateOrDestroyGridEntity);
      } else {
        leaveNodeTraversalEvent.setVertexDataTo(i, PeanoCurve::getCellWriteStackNumber(state));
      }
      continue;
    }

    if (PeanoCurve::isInOutStack(stackNumber)
        and not leaveNodeTraversalEvent.getIsVertexLocal(vertexIndex.to_ulong())) {
      leaveNodeTraversalEvent.setVertexDataTo(i, GridTraversalObserver::NoData);
      continue;
    }

    switch (type) {
    case GridVertex::State::HangingVertex:
      assertion(false);
      break;
    case GridVertex::State::New:
    case GridVertex::State::Unrefined:
    case GridVertex::State::Refined:
    case GridVertex::State::RefinementTriggered:
    case GridVertex::State::Refining:
    case GridVertex::State::EraseTriggered:
    case GridVertex::State::Erasing:
      leaveNodeTraversalEvent.setVertexDataTo(i, stackNumber);
      break;
    case GridVertex::State::Delete: {
      if (PeanoCurve::isInOutStack(stackNumber)) {
        leaveNodeTraversalEvent.setVertexDataTo(i, GridTraversalObserver::CreateOrDestroyGridEntity);
      } else {
        leaveNodeTraversalEvent.setVertexDataTo(i, stackNumber);
      }
    } break;
    }
  }

  for (int i = 0; i < TWO_TIMES_D; i++) {
    const int  faceIndex         = PeanoCurve::getFaceNumberAlongCurve(state, i); // facePositionWithinCell
    const int  stackNumber       = PeanoCurve::getFaceWriteStackNumber(state, faceIndex);
    const bool wasHangingOnEnter = enterNodeTraversalEvent.getIsFaceHanging(faceIndex);

    leaveNodeTraversalEvent.setFaceDataFrom(i, faceIndex);
    leaveNodeTraversalEvent.setIsFaceHanging(faceIndex, false);

    const FaceType type         = getFaceType(vertexTypes, faceIndex);
    const bool     isHangingNow = (type == FaceType::Hanging);

    // On leave, hanging-ness may have changed due to AMR effects in the DFS.
    // The 'vertexTypes' array is computed before we descend and is not updated.
    // Therefore, recompute the current face hanging-ness directly from the
    // current fineGridVertices states.
    // bool isHangingNow = true;
    // {
    //   const int normal = faceIndex % DIMENSIONS;
    //   const int side   = (faceIndex >= DIMENSIONS) ? 1 : 0;
    //   for (int v = 0; v < TWO_POWER_D; ++v) {
    //     if (((v >> normal) & 1) != side) continue; // only vertices adjacent to this face
    //     if (fineGridVertices[v].getState() != GridVertex::State::HangingVertex) {
    //       isHangingNow = false;
    //       break;
    //     }
    //   }
    // }

    if (wasHangingOnEnter and not isHangingNow) {
      leaveNodeTraversalEvent.setIsFaceHanging(faceIndex, true);
      leaveNodeTraversalEvent.setFaceDataTo(i, GridTraversalObserver::CreateOrDestroyGridEntity);
      continue;
    }

    if (isHangingNow) {
      leaveNodeTraversalEvent.setIsFaceHanging(faceIndex, true);
      if (cellType == CellType::Delete) {
        leaveNodeTraversalEvent.setFaceDataTo(i, GridTraversalObserver::CreateOrDestroyGridEntity);
      } else {
        leaveNodeTraversalEvent.setFaceDataTo(i, PeanoCurve::getCellWriteStackNumber(state));
      }
      continue;
    }

    if (PeanoCurve::isInOutStack(stackNumber) and not leaveNodeTraversalEvent.getIsFaceLocal(faceIndex)) {
      leaveNodeTraversalEvent.setFaceDataTo(i, GridTraversalObserver::NoData);
      continue;
    }

    switch (type) {
    case FaceType::Hanging:
      assertion(false);
      break;
    case FaceType::New:
    case FaceType::Persistent:
      leaveNodeTraversalEvent.setFaceDataTo(i, stackNumber);
      break;
    case FaceType::Delete:
      if (PeanoCurve::isInOutStack(stackNumber)) {
        leaveNodeTraversalEvent.setFaceDataTo(i, GridTraversalObserver::CreateOrDestroyGridEntity);
      } else {
        leaveNodeTraversalEvent.setFaceDataTo(i, stackNumber);
      }
      break;
    }
  }

  const int stackNumber = PeanoCurve::getCellWriteStackNumber(state);
  switch (cellType) {
  case CellType::New:
    leaveNodeTraversalEvent.setCellData(stackNumber);
    break;
  case CellType::Persistent:
    leaveNodeTraversalEvent.setCellData(stackNumber);
    break;
  case CellType::Delete:
    leaveNodeTraversalEvent.setCellData(GridTraversalObserver::CreateOrDestroyGridEntity);
    break;
  }
  if (PeanoCurve::isInOutStack(leaveNodeTraversalEvent.getCellData())
      and not leaveNodeTraversalEvent.getIsCellLocal()) {
    leaveNodeTraversalEvent.setCellData(GridTraversalObserver::NoData);
  }

#if PEANO_DEBUG >= 1
  for (int i = 0; i < TWO_POWER_D; i++) {
    assertion1(
      leaveNodeTraversalEvent.getIsVertexLocal(i)
        or not leaveNodeTraversalEvent.getIsVertexAdjacentToParallelDomainBoundary(i),
      leaveNodeTraversalEvent.toString()
    );
  }
  for (int i = 0; i < TWO_TIMES_D; i++) {
    assertion1(
      leaveNodeTraversalEvent.getIsFaceLocal(i)
        or not leaveNodeTraversalEvent.getIsFaceAdjacentToParallelDomainBoundary(i),
      leaveNodeTraversalEvent.toString()
    );
  }
#endif

  return leaveNodeTraversalEvent;
}
