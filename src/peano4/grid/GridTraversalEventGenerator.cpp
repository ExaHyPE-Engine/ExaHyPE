// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridTraversalEventGenerator.h"

#include "peano4/utils/Loop.h"
#include "PeanoCurve.h"
#include "Spacetree.h"
#include "TraversalObserver.h"

tarch::logging::Log peano4::grid::GridTraversalEventGenerator::_log("peano4::grid::GridTraversalEventGenerator");

peano4::grid::GridTraversalEventGenerator::GridTraversalEventGenerator(int id):
  _id(id) {}

std::bitset<TWO_POWER_D> peano4::grid::GridTraversalEventGenerator::areVerticesLocal(
  GridVertex                vertices[TWO_POWER_D],
  const SplitSpecification& splitTriggered,
  const std::set<int>&      splitting,
  const std::set<int>&      joinTriggered,
  const std::set<int>&      joining
) const {
  std::bitset<TWO_POWER_D> bitset;
  for (int i = 0; i < TWO_POWER_D; i++) {
    bitset.set(
      i,
      isVertexAdjacentToLocalSpacetree(vertices[i], splitTriggered, splitting, joinTriggered, joining, true, true)
    );
  }
  return bitset;
}

std::bitset<TWO_TIMES_D> peano4::grid::GridTraversalEventGenerator::areFacesLocal(
  GridVertex                vertices[TWO_POWER_D],
  const SplitSpecification& splitTriggered,
  const std::set<int>&      splitting,
  const std::set<int>&      /*joinTriggered*/,
  const std::set<int>&      /*joining*/
) const {
  std::bitset<TWO_TIMES_D> result;

  for (int faceNumber = 0; faceNumber < TWO_TIMES_D; faceNumber++) {
    bool isLocal = false;

    const int normal = faceNumber % DIMENSIONS;
    const bool upper = faceNumber >= DIMENSIONS;

    for (int i = 0; i < TWO_POWER_D; i++) {
      std::bitset<DIMENSIONS> studiedVertex(i);
      studiedVertex.set(normal, upper);
      const int vertexIndex = static_cast<int>(studiedVertex.to_ulong());
      const GridVertex& vertex = vertices[vertexIndex];

      std::bitset<DIMENSIONS> studiedEntry = TWO_POWER_D - vertexIndex - 1;

      studiedEntry.set(normal, 0);
      int currentRank = vertex.getAdjacentRanks(static_cast<int>(studiedEntry.to_ulong()));
      if (vertex.getState() != GridVertex::State::HangingVertex or upper) {
        isLocal |= currentRank == _id or splitTriggered.count(currentRank) > 0 or splitting.count(currentRank) > 0;
      }

      studiedEntry.set(normal, 1);
      currentRank = vertex.getAdjacentRanks(static_cast<int>(studiedEntry.to_ulong()));
      if (vertex.getState() != GridVertex::State::HangingVertex or not upper) {
        isLocal |= currentRank == _id or splitTriggered.count(currentRank) > 0 or splitting.count(currentRank) > 0;
      }

      if (isLocal) break;
    }

    result.set(faceNumber, isLocal);
  }

  return result;
}

bool peano4::grid::GridTraversalEventGenerator::isVertexAdjacentToLocalSpacetree(
  GridVertex                vertex,
  const SplitSpecification& splitTriggered,
  const std::set<int>&      splitting,
  const std::set<int>&      /*joinTriggered*/,
  const std::set<int>&      joining,
  bool                      splittingIsConsideredLocal,
  bool                      joiningIsConsideredLocal
) const {
  for (int i = 0; i < TWO_POWER_D; i++) {
    const auto adjacentRank = vertex.getAdjacentRanks(i);

    assertion(splitTriggered.count(adjacentRank) <= 1);
    assertion(splitting.count(adjacentRank) <= 1);

    if (
      adjacentRank == _id
      or
      splitTriggered.count(adjacentRank) == 1
      or
      (splittingIsConsideredLocal and splitting.count(adjacentRank) == 1)
      or
      (joiningIsConsideredLocal and joining.count(adjacentRank) == 1)
    ) {
      return true;
    }
  }

  return false;
}

bool peano4::grid::GridTraversalEventGenerator::isSpacetreeNodeLocal(
  GridVertex                vertices[TWO_POWER_D],
  const SplitSpecification& splitTriggered,
  const std::set<int>&      splitting,
  const std::set<int>&      /*joinTriggered*/,
  const std::set<int>&      joining,
  bool                      splittingIsConsideredLocal,
  bool                      joiningIsConsideredLocal
) const {
  dfor2(k)
    const int rank = vertices[kScalar].getAdjacentRanks(TWO_POWER_D - kScalar - 1);
    const auto isHanging = vertices[kScalar].getState() == GridVertex::State::HangingVertex;

    if (
      not isHanging and rank != _id and splitTriggered.find(rank) == splitTriggered.end()
      and
      (not splittingIsConsideredLocal or splitting.find(rank) == splitting.end())
      and
      (not joiningIsConsideredLocal or joining.find(rank) == joining.end())
    ) {
      return false;
    }
  enddforx

  return true;
}

std::bitset<TWO_POWER_D> peano4::grid::GridTraversalEventGenerator::areVerticesAdjacentToParallelDomainBoundary(
  GridVertex                vertices[TWO_POWER_D],
  const SplitSpecification& splitTriggered,
  const std::set<int>&      splitting,
  const std::set<int>&      /*joinTriggered*/,
  const std::set<int>&      /*joining*/,
  bool                      /*calledByLeaveCell*/
) const {
  std::bitset<TWO_POWER_D> result;
  for (int j = 0; j < TWO_POWER_D; j++) {
    tarch::la::Vector<TWO_POWER_D, int> adjacency = vertices[j].getBackupOfAdjacentRanks();
    bool                              oneLocal  = false;
    bool                              oneRemote = false;
    for (int i = 0; i < TWO_POWER_D; i++) {
      bool valid = adjacency(i) >= 0 or adjacency(i) == peano4::grid::Spacetree::RankOfPeriodicBoundaryCondition;
      bool local = adjacency(i) == _id or splitting.count(adjacency(i)) > 0 or splitTriggered.count(adjacency(i)) > 0;
      oneLocal |= (valid and local);
      oneRemote |= (valid and not local);
    }
    result.set(j, oneLocal and oneRemote);
  }
  return result;
}

std::bitset<TWO_TIMES_D> peano4::grid::GridTraversalEventGenerator::areFacesAdjacentToParallelDomainBoundary(
  GridVertex                vertices[TWO_POWER_D],
  const SplitSpecification& splitTriggered,
  const std::set<int>&      splitting,
  const std::set<int>&      /*joinTriggered*/,
  const std::set<int>&      /*joining*/,
  bool                      calledByLeaveCell
) const {
  std::bitset<TWO_TIMES_D> result;
  for (int j = 0; j < TWO_TIMES_D; j++) {
    // @todo
    tarch::la::Vector<TWO_POWER_D, int> adjacency = getAdjacentRanksOfFace(vertices, j, calledByLeaveCell);
    // tarch::la::Vector< TWO_POWER_D, int > adjacency = getAdjacentRanksOfFace(vertices, i, false);
    bool oneLocal  = false;
    bool oneRemote = false;
    for (int i = 0; i < TWO_POWER_D; i++) {
      // @todo noch rueber auf vertices
      // So ganz stimmt es natuerlich net, weil das ist ja jetzt eins zu frueh
      bool valid = adjacency(i) >= 0 or adjacency(i) == peano4::grid::Spacetree::RankOfPeriodicBoundaryCondition;
      /*
            bool local = adjacency(i)==_id
                      or (splitting.count(adjacency(i))>0 and calledByEnterCell)
                      or splitTriggered.count(adjacency(i))>0;
            bool remote = adjacency(i)!=_id
                      and not splitTriggered.count(adjacency(i))>0
                      and (not splitting.count(adjacency(i))>0 or not calledByEnterCell);
      */
      bool local = adjacency(i) == _id
                   or splitTriggered.count(adjacency(i)) > 0
                   //                or splitting.count(adjacency(i))>0;
                   or (splitting.count(adjacency(i)) > 0 and not calledByLeaveCell);
      bool remote = adjacency(i) != _id and not(splitTriggered.count(adjacency(i)) > 0)
                    and (splitting.count(adjacency(i)) == 0 or calledByLeaveCell);
      //      and not (splitTriggered.count(adjacency(i))>0 and calledByLeaveCell);
      //      bool remote = not local;
      oneLocal |= (valid and local);
      oneRemote |= (valid and remote);
    }
    result.set(j, oneLocal and oneRemote);
  }
  return result;
}

peano4::grid::GridTraversalEvent peano4::grid::GridTraversalEventGenerator::createGenericCellTraversalEvent(
  GridVertex                                coarseGridVertices[TWO_POWER_D],
  GridVertex                                fineGridVertices[TWO_POWER_D],
  const AutomatonState&                     state,
  const SplitSpecification&                 splitTriggered,
  const std::set<int>&                      splitting,
  const std::set<int>&                      joinTriggered,
  const std::set<int>&                      joining,
  const tarch::la::Vector<DIMENSIONS, int>& relativePositionToFather,
  bool                                      spacetreeStateIsRunning
) const {
  GridTraversalEvent event;
  event.setX(state.getX() + state.getH() * 0.5);
  event.setH(state.getH());

  event.setHasBeenRefined(haveVerticesBeenRefined(fineGridVertices));
  event.setWillBeRefined(willVerticesBeRefined(fineGridVertices));
  event.setRelativePositionToFather(relativePositionToFather);

  event.setIsCellLocal(isSpacetreeNodeLocal(fineGridVertices, splitTriggered, splitting, joinTriggered, joining, true, true));
  event.setIsParentCellLocal(state.getLevel() > 1 and isSpacetreeNodeLocal(coarseGridVertices, splitTriggered, splitting, joinTriggered, joining, true, true));
  event.setIsFaceLocal(areFacesLocal(fineGridVertices, splitTriggered, splitting, joinTriggered, joining));
  event.setIsVertexLocal(areVerticesLocal(fineGridVertices, splitTriggered, splitting, joinTriggered, joining));

  event.setIsParentVertexLocal(areVerticesLocal(coarseGridVertices, splitTriggered, splitting, joinTriggered, joining));

  event.setInvokingSpacetree(_id);
  event.setInvokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing(
    spacetreeStateIsRunning and joinTriggered.empty() and joining.empty() and splitTriggered.empty()
    and splitting.empty()
  );

  event.setIsAdjacentCellLocal(0);
  dfor2(k) {
    if (fineGridVertices[kScalar].getState() != GridVertex::State::HangingVertex) {
      dfor2(j) {
        tarch::la::Vector<DIMENSIONS, int> entryIn3x3Patch = k + j;
        int                                owningTree      = fineGridVertices[kScalar].getAdjacentRanks(jScalar);
        event.setIsAdjacentCellLocal(peano4::utils::dLinearised(entryIn3x3Patch, 3), owningTree == _id);
        event.setIsVertexParentOfSubtree(
          fineGridVertices[kScalar].getHasBeenParentOfSubtreeVertexInPreviousTreeSweep()
        );
      }
      enddforx
    }
  }
  enddforx

  dfor2(k) {
    std::set<int> neighbourIds;

    for (int i = 0; i < TWO_POWER_D; i++) {
      if (fineGridVertices[kScalar].getAdjacentRanks(i) >= 0) {
        neighbourIds.insert(fineGridVertices[kScalar].getAdjacentRanks(i));
      }
    }

    event.setNumberOfAdjacentTreesPerVertex(kScalar, neighbourIds.size());
  }
  enddforx

  return event;
}

peano4::grid::GridTraversalEvent peano4::grid::GridTraversalEventGenerator::createLeaveCellTraversalEvent(
  GridVertex                                coarseGridVertices[TWO_POWER_D],
  GridVertex                                fineGridVertices[TWO_POWER_D],
  const AutomatonState&                     state,
  const SplitSpecification&                 splitTriggered,
  const std::set<int>&                      splitting,
  const std::set<int>&                      joinTriggered,
  const std::set<int>&                      joining,
  const std::set<int>&                      /*hasSplit*/,
  const tarch::la::Vector<DIMENSIONS, int>& relativePositionToFather,
  bool                                      spacetreeStateIsRunning
) const {
  GridTraversalEvent event = createGenericCellTraversalEvent(
    coarseGridVertices,
    fineGridVertices,
    state,
    splitTriggered,
    splitting,
    joinTriggered,
    joining,
    relativePositionToFather,
    spacetreeStateIsRunning
  );

  event.setIsVertexAdjacentToParallelDomainBoundary(areVerticesAdjacentToParallelDomainBoundary(
    fineGridVertices,
    splitTriggered,
    splitting,
    joinTriggered,
    joining,
    true
  ));
  event.setIsFaceAdjacentToParallelDomainBoundary(
    areFacesAdjacentToParallelDomainBoundary(fineGridVertices, splitTriggered, splitting, joinTriggered, joining, true)
  );

  const std::bitset<DIMENSIONS> coordinates = PeanoCurve::getFirstVertexIndex(state);
  for (int i = 0; i < TWO_POWER_D; i++) {
    const std::bitset<DIMENSIONS> vertexIndex(coordinates ^ std::bitset<DIMENSIONS>(i));
    const int                     stackNumber = PeanoCurve::getVertexWriteStackNumber(state, vertexIndex);

    event.setVertexDataFrom(i, vertexIndex.to_ulong());
    switch (fineGridVertices[vertexIndex.to_ulong()].getState()) {
    case GridVertex::State::HangingVertex:
      event.setVertexDataTo(i, TraversalObserver::CreateOrDestroyHangingGridEntity);
      break;
    case GridVertex::State::New:
    case GridVertex::State::Unrefined:
    case GridVertex::State::Refined:
    case GridVertex::State::RefinementTriggered:
    case GridVertex::State::Refining:
    case GridVertex::State::EraseTriggered:
    case GridVertex::State::Erasing:
      event.setVertexDataTo(i, stackNumber);
      break;
    case GridVertex::State::Delete: {
      if (PeanoCurve::isInOutStack(stackNumber)) {
        event.setVertexDataTo(i, TraversalObserver::CreateOrDestroyPersistentGridEntity);
      } else {
        event.setVertexDataTo(i, stackNumber);
      }
    } break;
    }

    if (PeanoCurve::isInOutStack(event.getVertexDataTo(i)) and not event.getIsVertexLocal(vertexIndex.to_ulong())) {
      logDebug(
        "createLeaveCellTraversalEvent(...)",
        "reset event entry "
          << i << " to no-data as we have " << event.toString()
          << ". vertex=" << fineGridVertices[vertexIndex.to_ulong()].toString()
      );
      event.setVertexDataTo(i, TraversalObserver::NoData);
    }
  }

  for (int i = 0; i < 2 * DIMENSIONS; i++) {
    int       faceIndex   = PeanoCurve::getFaceNumberAlongCurve(state, i);
    FaceType  type        = getFaceType(coarseGridVertices, relativePositionToFather, faceIndex);
    const int stackNumber = PeanoCurve::getFaceWriteStackNumber(state, faceIndex);

    event.setFaceDataFrom(i, faceIndex);

    switch (type) {
    case FaceType::Hanging:
      event.setFaceDataTo(i, TraversalObserver::CreateOrDestroyHangingGridEntity);
      break;
    case FaceType::New:
    case FaceType::Persistent:
      event.setFaceDataTo(i, stackNumber);
      break;
    case FaceType::Delete:
      if (PeanoCurve::isInOutStack(stackNumber)) {
        event.setFaceDataTo(i, TraversalObserver::CreateOrDestroyPersistentGridEntity);
      } else {
        event.setFaceDataTo(i, stackNumber);
      }
      break;
    }

    if (PeanoCurve::isInOutStack(event.getFaceDataTo(i)) and not event.getIsFaceLocal(faceIndex)) {
      event.setFaceDataTo(i, TraversalObserver::NoData);
    }
  }

  {
    CellType  type        = getCellType(coarseGridVertices, relativePositionToFather);
    const int stackNumber = PeanoCurve::getCellWriteStackNumber(state);

    switch (type) {
    case CellType::New:
      event.setCellData(stackNumber);
      break;
    case CellType::Persistent:
      event.setCellData(stackNumber);
      break;
    case CellType::Delete:
      event.setCellData(TraversalObserver::CreateOrDestroyPersistentGridEntity);
      break;
    }

    if (
      // always true here, but if I write it here explicitly, then it is consistent with faces/vertices
      PeanoCurve::isInOutStack(event.getCellData())
      and
      not event.getIsCellLocal()
    ) {
      event.setCellData(TraversalObserver::NoData);
    }
  }

#if !defined(WITH_MULTITHREADING)
  for (int i = 0; i < TWO_POWER_D; i++)
    assertion1(event.getIsVertexLocal(i) or not event.getIsVertexAdjacentToParallelDomainBoundary(i), event.toString());
  for (int i = 0; i < TWO_TIMES_D; i++)
    assertion1(event.getIsFaceLocal(i) or not event.getIsFaceAdjacentToParallelDomainBoundary(i), event.toString());
#endif

  return event;
}

peano4::grid::GridTraversalEvent peano4::grid::GridTraversalEventGenerator::createPrunedEnterCellTraversalEvent(
  SpacetreeState            spacetreeState,
  const GridTraversalEvent& event
) const {
  GridTraversalEvent result = event;

  if (spacetreeState == SpacetreeState::EmptyRun or spacetreeState == SpacetreeState::NewFromSplit
      or spacetreeState == SpacetreeState::Joining) {
    result.setIsCellLocal(false);
    result.setIsParentCellLocal(false);
    result.setIsFaceLocal(0);
    result.setIsVertexLocal(0);
  }

  /*
    if(
      spacetreeState==SpacetreeState::EmptyRun or
      spacetreeState==SpacetreeState::NewFromSplit
    ) {
      result.setVertexDataFrom(TraversalObserver::NoData);
      result.setFaceDataFrom(TraversalObserver::NoData);
      result.setCellData(TraversalObserver::NoData);
    }
  */

  return result;
}

peano4::grid::GridTraversalEvent peano4::grid::GridTraversalEventGenerator::createPrunedLeaveCellTraversalEvent(
  SpacetreeState            spacetreeState,
  const GridTraversalEvent& event
) const {
  GridTraversalEvent result = event;

  if (spacetreeState == SpacetreeState::EmptyRun or spacetreeState == SpacetreeState::NewFromSplit
      or spacetreeState == SpacetreeState::Joining) {
    result.setIsCellLocal(false);
    result.setIsParentCellLocal(false);
    result.setIsFaceLocal(0);
    result.setIsVertexLocal(0);
  }

  /*
  if(
    spacetreeState==SpacetreeState::EmptyRun or
//    spacetreeState==SpacetreeState::NewFromSplit or
    spacetreeState==SpacetreeState::Joining
  ) {
    result.setVertexDataTo(TraversalObserver::NoData);
    result.setFaceDataTo(TraversalObserver::NoData);
    result.setCellData(TraversalObserver::NoData);
  }
*/

  return result;
}

peano4::grid::GridTraversalEvent peano4::grid::GridTraversalEventGenerator::createEnterCellTraversalEvent(
  GridVertex                                coarseGridVertices[TWO_POWER_D],
  GridVertex                                fineGridVertices[TWO_POWER_D],
  const AutomatonState&                     state,
  const SplitSpecification&                 splitTriggered,
  const std::set<int>&                      splitting,
  const std::set<int>&                      joinTriggered,
  const std::set<int>&                      joining,
  const std::set<int>&                      hasSplit,
  const tarch::la::Vector<DIMENSIONS, int>& relativePositionToFather,
  bool                                      spacetreeStateIsRunning
) const {
  GridTraversalEvent event = createGenericCellTraversalEvent(
    coarseGridVertices,
    fineGridVertices,
    state,
    splitTriggered,
    splitting,
    joinTriggered,
    joining,
    relativePositionToFather,
    spacetreeStateIsRunning
  );

  event.setIsVertexAdjacentToParallelDomainBoundary(areVerticesAdjacentToParallelDomainBoundary(
    fineGridVertices,
    splitTriggered,
    splitting,
    joinTriggered,
    joining,
    false
  ));
  event.setIsFaceAdjacentToParallelDomainBoundary(
    areFacesAdjacentToParallelDomainBoundary(fineGridVertices, splitTriggered, splitting, joinTriggered, joining, false)
  );

#if !defined(WITH_MULTITHREADING)
  for (int i = 0; i < TWO_POWER_D; i++)
    assertion1(event.getIsVertexLocal(i) or not event.getIsVertexAdjacentToParallelDomainBoundary(i), event.toString());
  for (int i = 0; i < TWO_TIMES_D; i++)
    assertion1(event.getIsFaceLocal(i) or not event.getIsFaceAdjacentToParallelDomainBoundary(i), event.toString());
#endif

  const std::bitset<DIMENSIONS> coordinates = PeanoCurve::getFirstVertexIndex(state);
  for (int i = 0; i < TWO_POWER_D; i++) {
    const std::bitset<DIMENSIONS> vertexIndex(coordinates ^ std::bitset<DIMENSIONS>(i));
    const int                     stackNumber    = PeanoCurve::getVertexReadStackNumber(state, vertexIndex);
    const int                     vertexPosition = vertexIndex.to_ulong();

    switch (fineGridVertices[vertexPosition].getState()) {
    case GridVertex::State::HangingVertex:
      event.setVertexDataFrom(i, TraversalObserver::CreateOrDestroyHangingGridEntity);
      break;
    case GridVertex::State::New: {
      if (PeanoCurve::isInOutStack(stackNumber)) {
        event.setVertexDataFrom(i, TraversalObserver::CreateOrDestroyPersistentGridEntity);
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
    event.setVertexDataTo(i, vertexIndex.to_ulong());

    bool mayResetToNoData = PeanoCurve::isInOutStack(event.getVertexDataFrom(i))
                            and not event.getIsVertexLocal(vertexPosition);

    for (auto p : hasSplit) {
      mayResetToNoData &= not tarch::la::contains(fineGridVertices[vertexPosition].getAdjacentRanks(), p);
    }

    if (mayResetToNoData) {
      event.setVertexDataFrom(i, TraversalObserver::NoData);
    }

    logDebug(
      "createEnterCellTraversalEvent(...)",
      "vertex "
        << i << " on on tree " << _id << ": " << event.toString() << ", mayResetToNoData=" << mayResetToNoData
        << ", is-local=" << event.getIsVertexLocal(vertexPosition)
        << ", is in-out=" << PeanoCurve::isInOutStack(event.getVertexDataFrom(i))
        << ", vertex[0]=" << fineGridVertices[0].toString() << ", vertex[1]=" << fineGridVertices[1].toString()
        << ", vertex[2]=" << fineGridVertices[2].toString() << ", vertex[3]=" << fineGridVertices[3].toString()
        << ", vertex[4]=" << fineGridVertices[4].toString() << ", vertex[5]=" << fineGridVertices[5].toString()
        << ", vertex[6]=" << fineGridVertices[6].toString() << ", vertex[7]=" << fineGridVertices[7].toString()
    );
  }

  for (int i = 0; i < 2 * DIMENSIONS; i++) {
    int       faceIndex   = PeanoCurve::getFaceNumberAlongCurve(state, i);
    FaceType  type        = getFaceType(coarseGridVertices, relativePositionToFather, faceIndex);
    const int stackNumber = PeanoCurve::getFaceReadStackNumber(state, faceIndex);

    switch (type) {
    case FaceType::New: {
      if (PeanoCurve::isInOutStack(stackNumber)) {
        event.setFaceDataFrom(i, TraversalObserver::CreateOrDestroyPersistentGridEntity);
      } else {
        event.setFaceDataFrom(i, stackNumber);
      }
    } break;
    case FaceType::Hanging:
      event.setFaceDataFrom(i, TraversalObserver::CreateOrDestroyHangingGridEntity);
      break;
    case FaceType::Persistent:
    case FaceType::Delete:
      event.setFaceDataFrom(i, stackNumber);
      break;
    }
    event.setFaceDataTo(i, faceIndex);

    bool mayResetToNoData = PeanoCurve::isInOutStack(event.getFaceDataFrom(i)) and not event.getIsFaceLocal(faceIndex);

    for (auto p : hasSplit) {
      mayResetToNoData &= not tarch::la::contains(getAdjacentRanksOfFace(fineGridVertices, faceIndex, false), p);
    }

    if (mayResetToNoData) {
      event.setFaceDataFrom(i, TraversalObserver::NoData);
    }
  }

  {
    CellType  type        = getCellType(coarseGridVertices, relativePositionToFather);
    const int stackNumber = PeanoCurve::getCellReadStackNumber(state);
    switch (type) {
    case CellType::New:
      event.setCellData(TraversalObserver::CreateOrDestroyPersistentGridEntity);
      break;
    case CellType::Persistent:
      event.setCellData(stackNumber);
      break;
    case CellType::Delete:
      event.setCellData(stackNumber);
      break;
    }
  }

  bool mayResetToNoData =
    // always true here, but if I write it here explicitly, then it is consistent with faces/vertices
    PeanoCurve::isInOutStack(event.getCellData()) and not event.getIsCellLocal();

  for (auto p : hasSplit) {
    mayResetToNoData &= getTreeOwningSpacetreeNode(fineGridVertices, splitTriggered, splitting, joinTriggered, joining)
                        != p;
  }

  if (mayResetToNoData) {
    event.setCellData(TraversalObserver::NoData);
  }

#if !defined(WITH_MULTITHREADING)
  for (int i = 0; i < TWO_POWER_D; i++)
    assertion1(event.getIsVertexLocal(i) or not event.getIsVertexAdjacentToParallelDomainBoundary(i), event.toString());
  for (int i = 0; i < TWO_TIMES_D; i++)
    assertion1(event.getIsFaceLocal(i) or not event.getIsFaceAdjacentToParallelDomainBoundary(i), event.toString());
#endif

  return event;
}

peano4::grid::FaceType peano4::grid::GridTraversalEventGenerator::getFaceType(
  GridVertex                         coarseGridVertices[TWO_POWER_D],
  tarch::la::Vector<DIMENSIONS, int> positionOfCell,
  int                                faceNumber
) {
  bool allVerticesAreHanging         = true;
  bool allVerticesAreDeleteOrHanging = true;
  bool allVerticesAreNewOrHanging    = true;

  const int normal = faceNumber % DIMENSIONS;
  for (int i = 0; i < TWO_POWER_D; i++) {
    std::bitset<DIMENSIONS> studiedVertex = i;
    studiedVertex.set(normal, faceNumber >= DIMENSIONS);
    switch (getVertexType(coarseGridVertices, positionOfCell + tarch::la::Vector<DIMENSIONS, int>(studiedVertex))) {
    case VertexType::Hanging:
      break;
    case VertexType::New:
      allVerticesAreDeleteOrHanging = false;
      allVerticesAreHanging         = false;
      break;
    case VertexType::Persistent:
      allVerticesAreHanging         = false;
      allVerticesAreDeleteOrHanging = false;
      allVerticesAreNewOrHanging    = false;
      break;
    case VertexType::Delete:
      allVerticesAreHanging      = false;
      allVerticesAreNewOrHanging = false;
      break;
    }
  }

  FaceType result = FaceType::Persistent;
  if (allVerticesAreHanging) {
    result = FaceType::Hanging;
  } else if (allVerticesAreNewOrHanging) {
    result = FaceType::New;
  } else if (allVerticesAreDeleteOrHanging) {
    result = FaceType::Delete;
  }

  return result;
}

peano4::grid::CellType peano4::grid::GridTraversalEventGenerator::getCellType(
  GridVertex                         coarseGridVertices[TWO_POWER_D],
  tarch::la::Vector<DIMENSIONS, int> positionOfCell
) {
  bool allVerticesAreDelete = true;
  bool allVerticesAreNew    = true;

  for (int i = 0; i < TWO_POWER_D; i++) {
    std::bitset<DIMENSIONS> vectorPosition = i;
    switch (getVertexType(coarseGridVertices, positionOfCell + tarch::la::Vector<DIMENSIONS, int>(vectorPosition))) {
    case VertexType::Hanging:
      break;
    case VertexType::Persistent:
      allVerticesAreDelete = false;
      allVerticesAreNew    = false;
      break;
    case VertexType::New:
      allVerticesAreDelete = false;
      break;
    case VertexType::Delete:
      allVerticesAreNew = false;
      break;
    }
  }

  assertion(not(allVerticesAreDelete and allVerticesAreNew));

  if (allVerticesAreDelete) {
    return CellType::Delete;
  } else if (allVerticesAreNew) {
    return CellType::New;
  } else {
    return CellType::Persistent;
  }
}

peano4::grid::VertexType peano4::grid::GridTraversalEventGenerator::
  getVertexType(GridVertex coarseGridVertices[TWO_POWER_D], tarch::la::Vector<DIMENSIONS, int> position, int dimension) {
  // Helper lambda to determine the base case
  auto baseCase = [](const GridVertex& vertex) -> VertexType {
    switch (vertex.getState()) {
    case GridVertex::State::Refined:
    case GridVertex::State::EraseTriggered:
      return VertexType::Persistent;
    case GridVertex::State::Refining:
      return VertexType::New;
    case GridVertex::State::RefinementTriggered:
    case GridVertex::State::Delete:
    case GridVertex::State::New:
    case GridVertex::State::Unrefined:
    case GridVertex::State::HangingVertex:
      return VertexType::Hanging;
    case GridVertex::State::Erasing:
      return VertexType::Delete;
    }
    assertion(false);           // Should never happen
    return VertexType::Hanging; // Default fallback
  };

  if (dimension < 0) {
    int linearIndex = utils::dLinearised(position, 2);
    return baseCase(coarseGridVertices[linearIndex]);
  }

  if (position(dimension) == 0) {
    // position(dimension) = 0;
    return getVertexType(coarseGridVertices, position, dimension - 1);
  } else if (position(dimension) == 3) {
    position(dimension) = 1;
    return getVertexType(coarseGridVertices, position, dimension - 1);
  }

  tarch::la::Vector<DIMENSIONS, int> positionCopy = position;
  position(dimension)                             = 0;
  VertexType lhs                                  = getVertexType(coarseGridVertices, position, dimension - 1);
  positionCopy(dimension)                         = 1;
  VertexType rhs                                  = getVertexType(coarseGridVertices, positionCopy, dimension - 1);

  // Merge table to avoid branching
  // Table rows: lhs
  // Table columns: rhs
  constexpr VertexType MergeTable[4][4] = {
    {VertexType::New, VertexType::New, VertexType::Persistent, VertexType::Persistent},
    {VertexType::New, VertexType::Hanging, VertexType::Persistent, VertexType::Delete},
    {VertexType::Persistent, VertexType::Persistent, VertexType::Persistent, VertexType::Persistent},
    {VertexType::Persistent, VertexType::Delete, VertexType::Persistent, VertexType::Delete},
  };
  return MergeTable[static_cast<int>(lhs)][static_cast<int>(rhs)];
}

peano4::grid::VertexType peano4::grid::GridTraversalEventGenerator::getVertexType(
  GridVertex                         coarseGridVertices[TWO_POWER_D],
  tarch::la::Vector<DIMENSIONS, int> position
) {
#if DIMENSIONS == 2 or DIMENSIONS == 3
  if (
    coarseGridVertices[0].getState() == GridVertex::State::Refined
    and coarseGridVertices[1].getState() == GridVertex::State::Refined
    and coarseGridVertices[2].getState() == GridVertex::State::Refined
    and coarseGridVertices[3].getState() == GridVertex::State::Refined
#if DIMENSIONS == 3
    and coarseGridVertices[4].getState() == GridVertex::State::Refined
    and coarseGridVertices[5].getState() == GridVertex::State::Refined
    and coarseGridVertices[6].getState() == GridVertex::State::Refined
    and coarseGridVertices[7].getState() == GridVertex::State::Refined
#endif
  ) {
    return VertexType::Persistent;
  }
#endif
  return getVertexType(coarseGridVertices, position, DIMENSIONS - 1);
}

int peano4::grid::GridTraversalEventGenerator::getTreeOwningSpacetreeNode(
  GridVertex                vertices[TWO_POWER_D],
  const SplitSpecification& splitTriggered,
  const std::set<int>&      splitting,
  const std::set<int>&      joinTriggered,
  const std::set<int>&      joining
) const {
  constexpr int NotSetYet = -1;
  int           id        = NotSetYet;

  int weakId = NotSetYet;
  dfor2(
    k
  ) if (vertices[kScalar].getState() != GridVertex::State::HangingVertex
        and vertices[kScalar].getAdjacentRanks(TWO_POWER_D - kScalar - 1) != InvalidRank) {
    weakId = vertices[kScalar].getAdjacentRanks(TWO_POWER_D - kScalar - 1);
  }
  if (vertices[kScalar].getState() != GridVertex::State::HangingVertex
      and isVertexAdjacentToLocalSpacetree(
        vertices[kScalar],
        splitTriggered,
        splitting,
        joinTriggered,
        joining,
        true,
        false
      )) {
    assertion8(
      id == NotSetYet or vertices[kScalar].getAdjacentRanks(TWO_POWER_D - kScalar - 1) == id,
      id,
      kScalar,
      vertices[kScalar].toString(),
      vertices[0].toString(),
      vertices[1].toString(),
      vertices[2].toString(),
      vertices[3].toString(),
      _id
    );
    id = vertices[kScalar].getAdjacentRanks(TWO_POWER_D - kScalar - 1);
  }
  enddforx assertion1(
    id != NotSetYet
      or not isSpacetreeNodeLocal(vertices, splitTriggered, splitting, joinTriggered, joining, false, false),
    id
  );

  if (id == NotSetYet) {
    id = weakId;
  }

  return id;
}

tarch::la::Vector<TWO_POWER_D, int> peano4::grid::GridTraversalEventGenerator::
  getAdjacentRanksOfFace(GridVertex fineGridVertices[TWO_POWER_D], int faceNumber, bool calledByReceivingProcess) const {
  tarch::la::Vector<TWO_POWER_D, int> adjacentRanksOfFace(InvalidRank);

  int counter = 0;
  const int normal  = faceNumber % DIMENSIONS;
  const bool isLowerFace = faceNumber < DIMENSIONS;

  dfore(i, 2, normal, isLowerFace ? 0 : 1) {
    const int currentVertexIndex = peano4::utils::dLinearised(i, 2);
    const GridVertex& vertex = fineGridVertices[currentVertexIndex];
    const GridVertex::State state = vertex.getState();

    bool isInvalid = (state == GridVertex::State::HangingVertex)
      or (calledByReceivingProcess and state == GridVertex::State::New)
      or (not calledByReceivingProcess and state == GridVertex::State::Delete);

    for (int bit = 0; bit <= 1; ++bit) {
      std::bitset<DIMENSIONS> studiedEntry = TWO_POWER_D - currentVertexIndex - 1;
      studiedEntry.set(normal, bit);

      const int entryIndex = static_cast<int>(studiedEntry.to_ulong());
      int rank = calledByReceivingProcess
               ? vertex.getBackupOfAdjacentRanks(entryIndex)
               : vertex.getAdjacentRanks(entryIndex);

      adjacentRanksOfFace(counter++) = isInvalid ? InvalidRank : rank;
    }
  }

  return adjacentRanksOfFace;
}
