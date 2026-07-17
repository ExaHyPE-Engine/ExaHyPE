// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "AutomatonState.h"
#include "GridControlEvent.h"
#include "GridStatistics.h"
#include "GridTraversalEventGenerator.h"
#include "GridVertex.h"
#include "PeriodicTopology.h"
#include "SpacetreeState.h"
#include "SplitInstruction.h"
#include "StackTopology.h"
#include "TreeIdSet.h"
#include "VertexLookupTable.h"

#include "peano4/stacks/Stack.h"

namespace peano4::grid {
  class Spacetree;
  class GridTraversalObserver;
  struct GridTraversalEvent;
} // namespace peano4::grid

namespace peano4::grid::tests {
  class SpacetreeTest;
} // namespace peano4::grid::tests

namespace peano4::parallel {
  class SpacetreeSet;
} // namespace peano4::parallel

namespace peano4::parallel::tests {
  class SpacetreeSetTest;
} // namespace peano4::parallel::tests

class peano4::grid::Spacetree final {
private:
  friend class peano4::parallel::SpacetreeSet;
  friend class peano4::parallel::tests::SpacetreeSetTest;
  friend class peano4::grid::tests::SpacetreeTest;

  const int _id;

  SpacetreeState _spacetreeState;

  /**
   * The root of a spacetree corresponds to the initial state of the tree
   * traversal automaton, so we reuse the object here. It spans the bounding
   * box of the whole domain on every tree.
   */
  AutomatonState _root;

  GridStatistics _statistics;

  /**
   * Identifies the parent tree during split and join operations. Root trees
   * use -1.
   */
  const int               _masterId;
  std::unordered_set<int> _childrenIds;

  /** Indicates which axes have periodic boundary conditions. */
  const std::bitset<DIMENSIONS> _periodicBC;

  /**
   * Split requests remain registered until the following traversal applies
   * them to the grid.
   */
  peano4::grid::SplitSpecification           _splitTriggered;
  peano4::grid::SplitSpecification::iterator _nextSplitInstruction;

  TreeIdSet _splitting;

  GridTraversalEventGenerator _gridTraversalEventGenerator;

  /** Records completed splits for replication routing. */
  TreeIdSet _hasSplit;

  /**
   * Join requests move to _joining after traversal so each traversal uses a
   * stable transition set.
   */
  TreeIdSet _joinTriggered;
  TreeIdSet _joining;

  bool _noParallelTopologyChanges;

  // Cache entries live in a thread-local unordered_map; element references stay valid across rehashes.
  std::deque<const VertexLookupTable*> _vertexLookupTables;

  std::vector<int>                                                                       _neighbourTreeIds;
  std::array<stacks::Stack<GridVertex>, StackTopology::TotalNumberOfLocalExchangeStacks> _localExchangeVertexStacks;
  std::vector<std::array<
    stacks::Stack<GridVertex>,
    static_cast<std::size_t>(StackTopology::ParallelExchangeStack::NumberOfStacks)>>
    _parallelExchangeVertexStacks;

  /**
   * Periodic boundary faces and vertex images this tree produced (sent)
   * respectively consumed (read) in the current traversal, recorded as
   * integer-index descriptors. Cleared at the start of each traversal;
   * SpacetreeSet::rebuildPeriodicAssociations() resolves them into the
   * routing table that pairs the two sides of every wrap. See
   * PeriodicTopology.h.
   */
  std::vector<PeriodicDescriptor> _periodicProducerDescriptors;
  std::vector<PeriodicDescriptor> _periodicConsumerDescriptors;

  /**
   * Stamps kept in lockstep with the periodic boundary entries of
   * _localExchangeVertexStacks, so a partner tree pops a grid vertex by
   * identity instead of by traversal order. Only the periodic stacks carry
   * entries; all other slots stay empty.
   */
  std::array<std::vector<PeriodicStamp>, StackTopology::TotalNumberOfLocalExchangeStacks>
    _localExchangeVertexStackStamps;

  int getOrCreateNeighbourSlot(int neighbourTreeId);
  int getNeighbourSlot(int neighbourTreeId) const;

  std::vector<GridControlEvent> _gridControlEvents;

  void evaluateGridControlEvents(
    const AutomatonState& fineGridState,
    GridVertex            coarseGridVertices[TWO_POWER_D],
    GridVertex            fineGridVertices[TWO_POWER_D]
  );

  void markVerticesAroundForkedCell(
    GridVertex coarseGridVertices[TWO_POWER_D],
    GridVertex fineGridVertices[TWO_POWER_D]
  ) const;

  void descend(
    const AutomatonState&                     state,
    const tarch::la::Vector<DIMENSIONS, int>& cellIndex,
    GridVertex                                vertices[TWO_POWER_D],
    GridTraversalObserver&                    observer
  );

  static void incrementNumberOfAdjacentRefinedLocalCells(GridVertex vertices[TWO_POWER_D]);

  void loadVertices(
    const AutomatonState&                     fineGridState,
    const tarch::la::Vector<DIMENSIONS, int>& cellIndex,
    GridVertex                                coarseGridVertices[TWO_POWER_D],
    GridVertex                                fineGridVertices[TWO_POWER_D],
    const VertexType                          vertexTypes[TWO_POWER_D],
    const tarch::la::Vector<DIMENSIONS, int>& cellPositionWithin3x3Patch
  );

  void storeVertices(
    const tarch::la::Vector<DIMENSIONS, int>& cellIndex,
    GridVertex                                coarseGridVertices[TWO_POWER_D],
    GridVertex                                fineGridVertices[TWO_POWER_D],
    const VertexType                          vertexTypes[TWO_POWER_D],
    const tarch::la::Vector<DIMENSIONS, int>& cellPositionWithin3x3Patch
  );

  void updateVertexAfterLoad(GridVertex& vertex, const tarch::la::Vector<DIMENSIONS, int>& vertexIndex);

  void receiveAndMergeUserData(
    const AutomatonState&                     state,
    const tarch::la::Vector<DIMENSIONS, int>& cellIndex,
    GridTraversalObserver&                    observer,
    const GridTraversalEvent&                 enterCellTraversalEvent,
    GridVertex                                fineGridVertices[TWO_POWER_D]
  );

  void sendUserData(
    const AutomatonState&                     state,
    const tarch::la::Vector<DIMENSIONS, int>& cellIndex,
    GridTraversalObserver&                    observer,
    const GridTraversalEvent&                 leaveCellTraversalEvent,
    GridVertex                                fineGridVertices[TWO_POWER_D]
  );

  /**
   * Stores child markers until bottom-up splitting has visited all siblings.
   * Top-down splitting uses the same stack to carry the parent's target tree.
   */
  std::vector<int> _splittedCells;

  void splitOrJoinCellBottomUp(GridVertex coarseGridVertices[TWO_POWER_D], GridVertex fineGridVertices[TWO_POWER_D]);

  void splitCellTopDown(GridVertex coarseGridVertices[TWO_POWER_D], GridVertex fineGridVertices[TWO_POWER_D]);

  void mergeCellFromWorkerWithMasterThroughoutJoin(
    GridVertex coarseGridVertices[TWO_POWER_D],
    GridVertex fineGridVertices[TWO_POWER_D]
  );

  bool shouldEraseAdjacencyInformation(
    const GridVertex&                  vertex,
    GridVertex                         coarseGridVertices[TWO_POWER_D],
    tarch::la::Vector<DIMENSIONS, int> fineVertexPositionWithinPatch
  ) const;

  void updateVertexBeforeStore(
    GridVertex&                               vertex,
    GridVertex                                coarseGridVertices[TWO_POWER_D],
    const tarch::la::Vector<DIMENSIONS, int>& fineVertexPositionWithinPatch,
    const tarch::la::Vector<DIMENSIONS, int>& vertexIndex
  );

  static bool restrictToCoarseGrid(
    const tarch::la::Vector<DIMENSIONS, int>& coarseVertexPosition,
    const tarch::la::Vector<DIMENSIONS, int>& fineVertexPosition
  );

  static void updateVertexRanksWithinCell(GridVertex fineGridVertices[TWO_POWER_D], int newTreeId);

  std::array<int, TWO_POWER_D> getAdjacentRanksForNewVertex(
    GridVertex                                coarseGridVertices[TWO_POWER_D],
    const tarch::la::Vector<DIMENSIONS, int>& vertexPositionWithin3x3Patch
  ) const;

  void split(int newSpacetreeId, const SplitInstruction& instruction);

  bool isFaceAlongPeriodicBoundaryCondition(
    GridVertex vertex[TWO_POWER_D],
    int        faceNumber,
    bool       calledByReceivingProcess
  ) const;

  void sendGridVertex(const GridVertex& vertex, const tarch::la::Vector<DIMENSIONS, int>& vertexIndex);

  void receiveAndMergeGridVertexAtHorizontalBoundary(
    GridVertex&                               vertex,
    const tarch::la::Vector<DIMENSIONS, int>& vertexIndex,
    bool                                      vertexIsAdjacentToLocalTree
  );

  void receiveAndMergeGridVertexAtVerticalBoundary(GridVertex& vertex);

  void mergeGridVertexRefinementStateAtHorizontalDomainBoundary(
    GridVertex&       vertex,
    const GridVertex& incomingVertex,
    int               neighbourTreeId
  );
  void mergeGridVertexAdjacencyListsAtHorizontalDomainBoundary(
    GridVertex&       vertex,
    const GridVertex& incomingVertex,
    int               neighbourTreeId
  );

  void joinWithMaster();
  void joinWithWorker(int workerId);

  bool mayJoinWithMaster() const;
  bool mayJoinWithWorker() const;

  bool maySplit() const;
  int  getSplittingTree();

  void updateSplittingCounter(int splittingTreeId);
  void resetSplittingTreeCursor();

public:
  Spacetree(
    int                                          newRootId,
    const tarch::la::Vector<DIMENSIONS, double>& offset,
    const tarch::la::Vector<DIMENSIONS, double>& width,
    const std::bitset<DIMENSIONS>&               periodicBC
  );

  Spacetree(
    int                                          newId,
    int                                          masterId,
    const tarch::la::Vector<DIMENSIONS, double>& offset,
    const tarch::la::Vector<DIMENSIONS, double>& width,
    bool                                         traversalInverted,
    const std::bitset<DIMENSIONS>&               periodicBC
  );

  ~Spacetree()                           = default;
  Spacetree(const Spacetree&)            = delete;
  Spacetree& operator=(const Spacetree&) = delete;
  Spacetree(Spacetree&&)                 = delete;
  Spacetree& operator=(Spacetree&&)      = delete;

  stacks::Stack<GridVertex>&       getLocalExchangeVertexStack(int localStackId);
  const stacks::Stack<GridVertex>& getLocalExchangeVertexStack(int localStackId) const;

  std::vector<PeriodicStamp>& getLocalExchangeVertexStamps(int localStackId);

  stacks::Stack<GridVertex>&
  getParallelExchangeVertexStack(int neighbourSlot, StackTopology::ParallelExchangeStack exchangeStack);
  const stacks::Stack<GridVertex>&
  getParallelExchangeVertexStack(int neighbourSlot, StackTopology::ParallelExchangeStack exchangeStack) const;

  int getNumberOfNeighbourTrees() const;
  int getNeighbourTreeIdForSlot(int neighbourSlot) const;

  void traverse(GridTraversalObserver& observer);

  const GridStatistics& getGridStatistics() const;

  int getId() const;
  int getMasterId() const;

  std::string toString() const;

  bool isInvolvedInJoinOrFork() const;
};
