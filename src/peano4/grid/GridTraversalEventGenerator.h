// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "AutomatonState.h"
#include "FaceType.h"
#include "GridTraversalEvent.h"
#include "GridVertex.h"
#include "SpacetreeState.h"
#include "SplitInstruction.h"
#include "TreeIdSet.h"
#include "VertexLookupTable.h"
#include "VertexType.h"

namespace peano4::grid {
  class GridTraversalEventGenerator;
} // namespace peano4::grid

class peano4::grid::GridTraversalEventGenerator {
private:
  const int _treeNumber;

public:
  GridTraversalEventGenerator(int id);
  ~GridTraversalEventGenerator() = default;

  GridTraversalEvent createEnterNodeTraversalEvent(
    GridVertex                                coarseGridVertices[TWO_POWER_D],
    GridVertex                                fineGridVertices[TWO_POWER_D],
    const VertexType                          vertexTypes[TWO_POWER_D],
    const FaceType                            precomputedFaceTypes[TWO_TIMES_D],
    const AutomatonState&                     state,
    const SplitSpecification&                 splitTriggered,
    const TreeIdSet&                          splitting,
    const TreeIdSet&                          joinTriggered,
    const TreeIdSet&                          joining,
    const TreeIdSet&                          hasSplit,
    const tarch::la::Vector<DIMENSIONS, int>& relativePositionToFather,
    const VertexLookupTable&                  lut,
    bool                                      noParallelTopologyChanges,
    bool                                      spacetreeStateIsRunning
  ) const;

  GridTraversalEvent createLeaveNodeTraversalEvent(
    const GridTraversalEvent&                 enterNodeTraversalEvent,
    GridVertex                                coarseGridVertices[TWO_POWER_D],
    GridVertex                                fineGridVertices[TWO_POWER_D],
    const VertexType                          vertexTypes[TWO_POWER_D],
    const FaceType                            precomputedFaceTypes[TWO_TIMES_D],
    const AutomatonState&                     state,
    const SplitSpecification&                 splitTriggered,
    const TreeIdSet&                          splitting,
    const TreeIdSet&                          joinTriggered,
    const TreeIdSet&                          joining,
    const TreeIdSet&                          hasSplit,
    const tarch::la::Vector<DIMENSIONS, int>& relativePositionToFather,
    const VertexLookupTable&                  lut,
    bool                                      noParallelTopologyChanges,
    bool                                      spacetreeStateIsRunning
  ) const;
};
