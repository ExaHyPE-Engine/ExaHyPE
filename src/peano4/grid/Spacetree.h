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
#include "tarch/instrumentation/Log.h"

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

class peano4::grid::Spacetree final {
private:
  friend class peano4::parallel::SpacetreeSet;
  friend class peano4::grid::tests::SpacetreeTest;

  int _id;

  SpacetreeState _spacetreeState;

  /**
   * The root of a spacetree corresponds to the initial state of the tree
   * traversal automaton, so we reuse the object here. It spans the bounding
   * box of the whole domain on every tree.
   */
  AutomatonState _root;

  GridStatistics _statistics;

  /**
   * This is not a static master. There's only a real master-worker relation
   * built into this part of the code when we actually split or join.
   *
   * This field should be const (it never changes), but in the MPI case, I
   * wanna be able to construct a tree first and then to set the master.
   * Actually, I should introduce a special constructor for this.
   */
  int                     _masterId;
  std::unordered_set<int> _childrenIds;

  /**
   * Indicate per axis whether we have periodic boundary conditions.
   */
  const std::bitset<DIMENSIONS> _periodicBC;

  /**
   * A split is identified by a tuple of id and cell count which tells the
   * code how many cells should go to a particular id. The actual split then
   * is done in a second iteration, i.e. we first bookmark all split requests
   * and then we roll over the requests in the subsequent iteration to be
   * actually performed.
   */
  SplitSpecification _splitTriggered;

  TreeIdSet _splitting;

  GridTraversalEventGenerator _gridTraversalEventGenerator;

  /**
   * I need this post-mortem list to identify which tree structures have to be
   * copied/replicated where.
   */
  TreeIdSet _hasSplit;

  constexpr static int NoJoin = -1;
  /**
   * This should be -1 if no join is triggered. Otherwise it holds the id of
   * the joining rank.
   */
  TreeIdSet _joinTriggered;

  /**
   * If master: Set of workers that should join
   */
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
   * @see splitOrJoinCellBottomUp() For both the usage and a description how we
   *   hijack the routine.
   */
  std::vector<int> _splittedCells;

  void splitOrJoinCellBottomUp(GridVertex vertex[TWO_POWER_D], GridVertex fineGridVertices[TWO_POWER_D]);

  void splitCellTopDown(GridVertex vertex[TWO_POWER_D], GridVertex fineGridVertices[TWO_POWER_D]);

  void
  mergeCellFromWorkerWithMasterThroughoutJoin(GridVertex vertex[TWO_POWER_D], GridVertex fineGridVertices[TWO_POWER_D]);

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

  static void updateVertexRanksWithinCell(GridVertex fineGridVertices[TWO_POWER_D], int newId);

  std::array<int, TWO_POWER_D> getAdjacentRanksForNewVertex(
    GridVertex                                coarseGridVertices[TWO_POWER_D],
    const tarch::la::Vector<DIMENSIONS, int>& vertexPositionWithin3x3Patch
  ) const;

  void split(int treeId, const SplitInstruction& instruction);

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
    const GridVertex& inVertex,
    int               neighbour
  );
  void mergeGridVertexAdjacencyListsAtHorizontalDomainBoundary(
    GridVertex&       vertex,
    const GridVertex& inVertex,
    int               neighbour
  );

  void joinWithMaster();
  void joinWithWorker(int id);

  bool mayJoinWithMaster() const;
  bool mayJoinWithWorker() const;

  bool maySplit() const;
  int  getSplittingTree() const;

  void updateSplittingCounter(int treeId);

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

  stacks::Stack<GridVertex>& getParallelExchangeVertexStack(int slot, StackTopology::ParallelExchangeStack which);
  const stacks::Stack<GridVertex>&
  getParallelExchangeVertexStack(int slot, StackTopology::ParallelExchangeStack which) const;

  int getNumberOfNeighbourTrees() const;
  int getNeighbourTreeIdForSlot(int slot) const;

  void traverse(GridTraversalObserver& observer);

  const GridStatistics& getGridStatistics() const;

  int getId() const;
  int getMasterId() const;

  std::string toString() const;

  bool isInvolvedInJoinOrFork() const;
};
