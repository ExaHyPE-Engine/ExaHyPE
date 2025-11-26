// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "AutomatonState.h"
#include "GridTraversalEvent.h"
#include "GridVertex.h"
#include "SpacetreeState.h"
#include "SplitInstruction.h"
#include "VertexType.h"
#include "tarch/logging/Log.h"
#include "tarch/tests/TestCase.h"

#include <set>

namespace peano4::grid {
  class GridTraversalEventGenerator;
} // namespace peano4::grid

class peano4::grid::GridTraversalEventGenerator {
private:
  const int _treeNumber;

  static tarch::logging::Log _log;

public:
  GridTraversalEventGenerator(int id);
  ~GridTraversalEventGenerator() = default;

  GridTraversalEvent createEnterNodeTraversalEvent(
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
  ) const;

  GridTraversalEvent createLeaveNodeTraversalEvent(
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
  ) const;
};
