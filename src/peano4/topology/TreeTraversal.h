// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Constants.h"
#include "peano4/grid/AutomatonState.h"
#include "peano4/grid/GridStatistics.h"
#include "peano4/grid/GridTraversalEvent.h"
#include "peano4/grid/GridTraversalObserver.h"
#include "peano4/grid/GridVertex.h"
#include "peano4/topology/Tree.h"
#include "tarch/la/Vector.h"

namespace peano4::topology {
  class TreeTraversal;
} // namespace peano4::topology

/**
 * Traverses a Tree along the active space-filling curve and emits grid traversal events
 * to the observer interface. A scope per subtree task lets disjoint tasks run
 * concurrently, each on its own traversal state.
 */
class peano4::topology::TreeTraversal final {
public:
  /**
   * Classifies a pool-dialect sweep. A creational sweep classifies every entity
   * as being created, so the observers allocate and initialise the data; a
   * standing sweep classifies by first and last touch.
   */
  enum class PoolSweepKind { Creational, Standing };

  /**
   * Stores per-entity classes of one transition sweep over the union tree, flagged
   * by union tree entity index. An entity only the old structure holds
   * destroys at its last touch, and an entity only the new structure holds
   * creates at its first touch. The refined-before and refined-after vertex
   * states drive the events' refinement masks.
   */
  struct RegridOverrides {
    std::vector<char> cellCreate{};
    std::vector<char> cellDestroy{};
    std::vector<char> faceCreate{};
    std::vector<char> faceDestroy{};
    std::vector<char> vertexCreate{};
    std::vector<char> vertexDestroy{};
    std::vector<char> vertexRefinedBefore{};
    std::vector<char> vertexRefinedAfter{};
    /** Marks frozen vertex and face slots that already hold migrated remote data. */
    std::vector<char> vertexFrozenSlotValid{};
    std::vector<char> faceFrozenSlotValid{};
  };

  /**
   * Stores the transition classes and pool-slot mappings required by one
   * regrid. In-place paths leave the migration-pair sets empty.
   */
  struct RegridTransition {
    RegridOverrides                  overrides{};
    std::vector<std::pair<int, int>> oldToUnionCellPairs{};
    std::vector<std::pair<int, int>> oldToUnionFacePairs{};
    std::vector<std::pair<int, int>> oldToUnionVertexPairs{};
    std::vector<std::pair<int, int>> unionToNewCellPairs{};
    std::vector<std::pair<int, int>> unionToNewFacePairs{};
    std::vector<std::pair<int, int>> unionToNewVertexPairs{};
    /** Orders repartitioned slots per remote rank by level, normal, and lattice key. */
    std::map<int, std::vector<int>> sendCellSlots{};
    std::map<int, std::vector<int>> receiveCellSlots{};
    std::map<int, std::vector<int>> sendFaceSlots{};
    std::map<int, std::vector<int>> receiveFaceSlots{};
    std::map<int, std::vector<int>> sendVertexSlots{};
    std::map<int, std::vector<int>> receiveVertexSlots{};
  };

  TreeTraversal(
    int                                          treeId,
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
    const tarch::la::Vector<DIMENSIONS, double>& domainWidth
  );

  TreeTraversal(const TreeTraversal&)            = delete;
  TreeTraversal(TreeTraversal&&)                 = delete;
  TreeTraversal& operator=(const TreeTraversal&) = delete;
  TreeTraversal& operator=(TreeTraversal&&)      = delete;
  ~TreeTraversal()                               = default;

  void traverseWithPools(
    const peano4::topology::Tree&        tree,
    peano4::grid::GridTraversalObserver& observer,
    PoolSweepKind                        sweepKind,
    const RegridOverrides*               regridOverrides = nullptr
  );

  void prepareRankTopology(
    const peano4::topology::Tree&          tree,
    const peano4::topology::Tree*          predecessorTree      = nullptr,
    const peano4::topology::TreeTraversal* predecessorTraversal = nullptr
  );

  /**
   * Records where an in-place regrid moved the frozen slots of the
   * rank-boundary entities that survive it, as pairs of the old and the new
   * frozen slot.
   */
  struct RankBoundaryRelocation {
    std::vector<std::pair<int, int>> vertexPairs{};
    std::vector<std::pair<int, int>> facePairs{};
  };

  RankBoundaryRelocation extendRankTopologyForAppendedEntities(
    const peano4::topology::Tree&                        tree,
    const peano4::topology::Tree::RefinementDeltaExtent& deltaExtent
  );

  RankBoundaryRelocation rederiveRankBoundary(const peano4::topology::Tree& tree);

  std::vector<int> extendInterfaceTopologyForAppendedEntities(
    const peano4::topology::Tree&                        tree,
    const peano4::topology::Tree::RefinementDeltaExtent& deltaExtent
  );

  void rederiveInterfaceTopology(const peano4::topology::Tree& tree, int taskLevel);

  std::vector<int> rederiveInterfaceTopologyForPartialRerecord(
    const peano4::topology::Tree&              tree,
    const peano4::topology::Tree::PruneExtent& pruneExtent
  );

  std::vector<int> shrinkInterfaceTopologyForPrunedEntities(
    const peano4::topology::Tree&              tree,
    const peano4::topology::Tree::PruneExtent& pruneExtent
  );

  bool standingCutsMatchFreshSelection(const peano4::topology::Tree& tree, int taskLevel);

  void discardRecordingsForFullRerecord();

  void retainRecordingsForPartialRerecord(const std::vector<int>& staleUnits);

  void snapshotStandingDecomposition();

  void restoreSnapshottedDecomposition();

  void commitPendingInterfaceFlushPairs();

  void prepareInterfaceTopology(
    const peano4::topology::Tree& tree,
    int                           taskLevel,
    bool                          splitHeavyBlocks = true,
    const RegridOverrides*        regridOverrides  = nullptr
  );

  void prepareTransitionTopology(
    const peano4::topology::Tree& tree,
    int                           taskLevel,
    const RegridOverrides&        regridOverrides
  );

  void restoreStandingTaskDecomposition(const peano4::topology::Tree& tree, int taskLevel);

  void prepareParallelSweep(const peano4::topology::Tree& tree, int taskLevel, bool requiresInterfaceTopology = true);

  void traverseWithPoolTasks(
    const peano4::topology::Tree&        tree,
    peano4::grid::GridTraversalObserver& observer,
    PoolSweepKind                        sweepKind,
    int                                  taskLevel,
    const std::function<void()>&         waitForRemoteData,
    const RegridOverrides*               regridOverrides = nullptr
  );

  void mergeTaskInterfaceData(
    peano4::grid::GridTraversalObserver& observer,
    const std::function<void()>&         waitForRemoteData
  ) const;

  void mergeTransitionRankBoundaryData(peano4::grid::GridTraversalObserver& observer) const;

  void runConsolidations(peano4::grid::GridTraversalObserver& observer) const;

  RegridTransition deriveRegridTransition(
    const peano4::topology::Tree&                 oldTree,
    const peano4::topology::Tree&                 unionTree,
    const peano4::topology::TreeTraversal&        unionTraversal,
    const peano4::topology::Tree&                 newTree,
    const peano4::topology::TreeTraversal&        newTraversal,
    const peano4::topology::Tree::RefinementPlan& changedFlags,
    bool                                          partitionMoved,
    bool                                          unionIsStandingStructure = false
  ) const;

  /**
   * Reports the cell load of the last decomposed sweep's subtree tasks. The cell
   * spread says what the decomposition allows; the two times say what the run
   * reached.
   */
  struct SubtreeTaskLoad {
    int numberOfTasks{0};
    int minimumCells{0};
    int maximumCells{0};
    int totalCells{0};
    /** Records task-body time and the enclosing execution window. */
    double taskSeconds{0.0};
    double windowSeconds{0.0};
  };

  int                    getNumberOfReplayTasks() const;
  int                    getNumberOfSubtreeTasks() const;
  const SubtreeTaskLoad& getSubtreeTaskLoad() const;
  double                 getLastSweepTaskSeconds() const;

  const std::vector<std::pair<int, int>>& getInterfaceCopyToFrozenVertexPairs() const;
  const std::vector<std::pair<int, int>>& getInterfaceCopyToFrozenFacePairs() const;
  const std::vector<std::pair<int, int>>& getCreationalCopyToEntitySlotVertexPairs() const;
  const std::vector<std::pair<int, int>>& getCreationalCopyToEntitySlotFacePairs() const;
  const std::vector<std::pair<int, int>>& getInterfaceFlushVertexPairs() const;
  const std::vector<std::pair<int, int>>& getInterfaceFlushFacePairs() const;
  const std::vector<std::pair<int, int>>& getInterfaceSeedVertexPairs() const;
  const std::vector<std::pair<int, int>>& getInterfaceSeedFacePairs() const;

  const std::vector<int>&              getNeighbourRanks() const;
  const std::vector<std::vector<int>>& getVertexPackLists() const;
  const std::vector<std::vector<int>>& getFacePackLists() const;
  const std::vector<int>&              getVertexFrozenSlotBases() const;
  const std::vector<int>&              getFaceFrozenSlotBases() const;
  int                                  getNumberOfVertexRankFrozenSlots() const;
  int                                  getNumberOfFaceRankFrozenSlots() const;
  int                                  getNumberOfLocalCellSlots() const;
  int                                  getCellSlotSpaceSize() const;
  int                                  getVertexSlotSpaceSize() const;
  int                                  getFaceSlotSpaceSize() const;
  int                                  getNumberOfVertexShadowSlots() const;
  int                                  getNumberOfFaceShadowSlots() const;

  const peano4::grid::GridStatistics& getGridStatistics() const;
  void                                adoptGridStatistics(const peano4::grid::GridStatistics& statistics);

private:
  const int _id;

  const peano4::grid::AutomatonState _root;

  peano4::grid::GridStatistics _statistics{};

  int _numberOfStationarySweeps{0};

  /**
   * Describes the merge recipe of one entity: The caller merges every source slot's copy
   * into the slot the event names at the position, through the observer's
   * generated merge routines. The event carries the marker of the copy the
   * merge accumulates into.
   */
  struct InterfaceMergeDescriptor {
    peano4::grid::GridTraversalEvent event{};
    int                              position{0};
    bool                             isVertex{true};
    std::vector<int>                 sourceSlots{};
    /** Counts source slots belonging to partner subtree tasks. */
    int numberOfPartnerSlots{0};
    /**
     * Marks a consolidation, whose first source overwrites the target slot.
     */
    bool rebuildsTargetSlot{false};
  };

  /**
   * Stores one standing sweep's event stream in call order. A standing
   * grid emits the identical stream every sweep, so further sweeps replay the
   * recording as a flat array and skip the descent and every per-cell
   * classification. Every enter entry records how many immediately preceding
   * interface merges belong to it, so replay preserves their call position.
   */
  struct ReplayEntry {
    peano4::grid::GridTraversalEvent event{};
    int                              numberOfMergesBefore{0};
    bool                             isEnter{true};
  };
  struct EventRecording {
    std::vector<ReplayEntry>              entries{};
    std::vector<InterfaceMergeDescriptor> remoteMerges{};
    peano4::grid::GridStatistics          statistics{};
    bool                                  recorded{false};
  };
  EventRecording _eventRecording{};

  /**
   * Stores one subtree's entry buffer with its interface merges and root
   * geometry. A task touching a neighbour rank's data replays only after the
   * receives have landed in the frozen slots its merges read.
   */
  struct SubtreeReplayTask {
    std::vector<ReplayEntry>              entries{};
    tarch::la::Vector<DIMENSIONS, double> x{0.0};
    tarch::la::Vector<DIMENSIONS, double> h{0.0};
    std::vector<InterfaceMergeDescriptor> interfaceMerges{};
    /** Lets a partial rerecord replace exactly this task's contribution. */
    peano4::grid::GridStatistics statistics{};
    bool                         touchesRankBoundary{false};
  };

  /**
   * Stores the parity-independent boundary topology of one rank-local tree
   * view. Slot maps translate local entity indices into dense pool slots. Every
   * entity a neighbour rank also holds owns one frozen slot per remote partner,
   * and pack lists order these entities by level, normal, and lattice key, so
   * both sides derive the same sequence from independently numbered views.
   */
  struct RankTopology {
    std::vector<int>              neighbourRanks{};
    std::vector<std::vector<int>> vertexPackLists{};
    std::vector<std::vector<int>> facePackLists{};
    std::vector<int>              vertexFrozenSlotBases{};
    std::vector<int>              faceFrozenSlotBases{};
    /** Lists remote frozen slots per rank-boundary entity in rank order. */
    std::vector<std::pair<int, std::vector<int>>> remoteVertexPartners{};
    std::vector<std::pair<int, std::vector<int>>> remoteFacePartners{};
    std::vector<int>                              cellSlotOfTreeCell{};
    std::vector<int>                              vertexSlotOfTreeVertex{};
    std::vector<int>                              faceSlotOfTreeFace{};
    int                                           numberOfLocalCellSlots{0};
    int                                           numberOfLocalVertexSlots{0};
    int                                           numberOfLocalFaceSlots{0};
    /**
     * Size of each entity slot index range. A topology inherited across a
     * regrid keeps every surviving entity's slot, so freed slots leave gaps
     * and the range can exceed the local count; the frozen and shadow slots
     * start behind the range.
     */
    int cellSlotSpaceSize{0};
    int vertexSlotSpaceSize{0};
    int faceSlotSpaceSize{0};
    int numberOfVertexFrozenSlots{0};
    int numberOfFaceFrozenSlots{0};
    /**
     * Whether every surviving entity kept its predecessor slot. A regrid
     * between two inherited topologies then needs no entity migration
     * pairs, only the frozen slot moves.
     */
    bool slotsInherited{false};
    bool derived{false};
  };
  RankTopology _rankTopology{};

  /**
   * Stores the parity-independent slot layout of the interface entities. Every subtree
   * task touching an entity from several tasks owns a persistent live copy
   * and a frozen copy of its previous-sweep state, appended behind the
   * entity slots; the entity slot itself keeps the last serial state. A
   * task's lifecycle runs on its live copy, a post-sweep pass copies live to
   * frozen, and the next sweep's pre-range merges read the partners' frozen
   * copies.
   */
  struct InterfaceSlotPair {
    int live{-1};
    int frozen{-1};
  };
  struct TaskTouchSlot {
    InterfaceSlotPair interfaceSlots{};
    int               unit{-1};
    int               counter{-1};
    std::uint8_t      numberOfAdjacentCells{0};
  };
  struct InterfaceTopology {
    std::vector<int>                                               vertexInterfaceOfSlot{};
    std::vector<int>                                               faceInterfaceOfSlot{};
    std::vector<std::array<TaskTouchSlot, peano4::CellsPerVertex>> vertexInterfaceTouchSlots{};
    std::vector<std::array<TaskTouchSlot, 2>>                      faceInterfaceTouchSlots{};
    std::vector<std::pair<int, int>>                               copyToFrozenVertexPairs{};
    std::vector<std::pair<int, int>>                               copyToFrozenFacePairs{};
    std::vector<std::pair<int, int>>                               creationalCopyToEntitySlotVertexPairs{};
    std::vector<std::pair<int, int>>                               creationalCopyToEntitySlotFacePairs{};
    /** Maps each first toucher's live slot to its entity slot for the pre-regrid flush. */
    std::vector<std::pair<int, int>> flushVertexPairs{};
    std::vector<std::pair<int, int>> flushFacePairs{};
    /** Maps each entity slot to every toucher's live slot for post-regrid seeding. */
    std::vector<std::pair<int, int>> seedVertexPairs{};
    std::vector<std::pair<int, int>> seedFacePairs{};
    /** Identifies the subtree-task unit emitting each cell's events. */
    std::vector<int> unitOfCell{};
    int              numberOfVertexShadowSlots{0};
    int              numberOfFaceShadowSlots{0};
    int              numberOfVertexTouchCounters{0};
    int              numberOfFaceTouchCounters{0};
    bool             splitHeavyBlocks{false};
    bool             derived{false};
  };
  InterfaceTopology _interfaceTopology{};

  /**
   * Stores a parallel event recording: Every subtree
   * task records its own entry buffer and interface merges as it descends,
   * the master records the spine. Interface touches run on the toucher's
   * live slot, so every task owns the complete first- and last-touch
   * lifecycle of its copy and no two tasks write the same data. A
   * rank-boundary entity several tasks touch consolidates into its entity slot
   * after the sweep, so the pack always sends the entity slot.
   */
  struct ParallelRecording {
    std::vector<SubtreeReplayTask> subtreeTasks{};
    /** Task numbers sorted by falling entry count; a replay spawns them in this order. */
    std::vector<int>                      replayOrder{};
    std::vector<ReplayEntry>              masterEntries{};
    std::vector<InterfaceMergeDescriptor> masterMerges{};
    std::vector<InterfaceMergeDescriptor> consolidations{};
    peano4::grid::GridStatistics          statistics{};
    /** Lets a partial rerecord retain the master's grid-count contribution. */
    peano4::grid::GridStatistics masterStatistics{};
    bool                         derived{false};
    bool                         replaysEntries{false};
  };
  ParallelRecording _parallelRecording{};

  /**
   * Stores the merge recipe of every rank-boundary entity a transition sweep
   * touches. A caller replays it once the post-transition receives have landed.
   * The migration hop afterwards deletes the boundary's exchange channel, so a
   * merge that has not run by then loses its data.
   */
  std::vector<InterfaceMergeDescriptor> _transitionRankBoundaryMerges{};

  /**
   * Stores the root of one subtree task, derived once by a structure-only descent and
   * reused by every sweep. The unit is the task's first cell index, and a direct
   * task descends from that cell's parent so its descent emits the cell itself.
   * A task stops at every deeper block root it meets, which is another task's
   * root.
   */
  struct SubtreeTask {
    peano4::grid::AutomatonState rootState{};
    peano4::grid::AutomatonState descentState{};
    int                          rootCell{-1};
    int                          unit{-1};
    bool                         touchesRankBoundary{false};
  };
  struct SubtreeDecomposition {
    std::vector<SubtreeTask> tasks{};
    bool                     derived{false};
  };
  SubtreeDecomposition _subtreeDecomposition{};

  /** Limits the next rerecord to tasks invalidated by an in-place regrid. */
  std::vector<int> _staleRerecordUnits{};

  bool _partialRerecordPending{false};

  /**
   * Holds flush pairs of interface entities an in-place regrid created. They join
   * the flush list only after the post-transition seeding has filled the new
   * live copies, since a flush of an unseeded copy would clear its entity.
   */
  std::vector<std::pair<int, int>> _pendingFlushVertexPairs{};
  std::vector<std::pair<int, int>> _pendingFlushFacePairs{};

  /** Stores the standing decomposition while a transition uses its own. */
  std::vector<char>    _snapshotCellIsBlockRoot{};
  std::vector<char>    _snapshotCellIsDirectTask{};
  SubtreeDecomposition _snapshotSubtreeDecomposition{};

  SubtreeTaskLoad _subtreeTaskLoad{};

  /** Stores the task time of the last sweep, including a transition. */
  double _lastSweepTaskSeconds{0.0};

  /**
   * Marks block-root cells by cell index. Spawn-level blocks
   * are always roots; selected refined cells below them join only when all
   * their corners are refined, so interpolation and restriction stay within
   * one subtree task. descendWithPools() reads this table for every cell it
   * enters, so the table stores one byte per cell.
   */
  std::vector<char> _cellIsBlockRoot{};

  /**
   * Marks cells that emit the first event of a task. The table fills only when
   * the spawn level is zero: The root has no parent from which a regular block
   * task could be collected, so each local child becomes a direct task.
   */
  std::vector<char> _cellIsDirectTask{};

  /**
   * Records the first touch of a rank-boundary interface entity by one unit. The seeds
   * of all tasks, visited in task order, assemble into the consolidations.
   */
  struct ConsolidationSeed {
    peano4::grid::GridTraversalEvent event{};
    int                              position{0};
    bool                             isVertex{true};
    int                              entitySlot{-1};
    int                              liveSlot{-1};
  };

  /**
   * Stores one descent's mutable state: Touch counters indexed by local slot, the
   * recording targets and the unit redirection. The serial whole-domain
   * traversal, the master's spine traversal and every subtree task own one
   * scope each, so no two concurrent descents write the same state.
   */
  struct TraversalScope {
    PoolSweepKind sweepKind{PoolSweepKind::Standing};
    /** Identifies a subtree-task unit, or the master with -1. */
    int                    unit{-1};
    int                    directTaskRoot{-1};
    const RegridOverrides* regridOverrides{nullptr};
    /** Marks whether the descent stops at block roots owned by other tasks. */
    bool skipsBlockRoots{false};
    /** Marks whether a transition writes the rank's entity slots directly. */
    bool                                   writesEntitySlotsDirectly{false};
    std::vector<std::uint8_t>*             vertexTouchesOfAllUnits{nullptr};
    std::vector<std::uint8_t>*             faceTouchesOfAllUnits{nullptr};
    std::vector<std::uint8_t>              vertexTouches{};
    std::vector<std::uint8_t>              faceTouches{};
    std::vector<ReplayEntry>*              entries{nullptr};
    std::vector<InterfaceMergeDescriptor>* inlineMerges{nullptr};
    std::vector<InterfaceMergeDescriptor>* transitionRankBoundaryMerges{nullptr};
    std::vector<ConsolidationSeed>*        consolidationSeeds{nullptr};
    peano4::grid::GridStatistics           statistics{};
  };

  static void replayEntries(
    const std::vector<ReplayEntry>&              entries,
    const std::vector<InterfaceMergeDescriptor>& merges,
    peano4::grid::GridTraversalObserver&         observer
  );

  static void runMerge(const InterfaceMergeDescriptor& merge, peano4::grid::GridTraversalObserver& observer);

  static void runMerges(
    const std::vector<InterfaceMergeDescriptor>& merges,
    peano4::grid::GridTraversalObserver&         observer
  );

  static const std::vector<int>* findRemoteFrozenSlots(
    const std::vector<std::pair<int, std::vector<int>>>& remotePartners,
    int                                                  entitySlot
  );

  void assertRankTopologyMatchesTree(const peano4::topology::Tree& tree) const;
  void assertRegridOverridesMatchTree(const peano4::topology::Tree& tree, const RegridOverrides& overrides) const;

  void deriveRankBoundary(const peano4::topology::Tree& tree);

  RankBoundaryRelocation relocateRankBoundary(const peano4::topology::Tree& tree);

  void descendWithPools(
    const peano4::topology::Tree&        tree,
    const peano4::grid::AutomatonState&  state,
    int                                  cellIndex,
    peano4::grid::GridVertex             coarseGridVertices[peano4::VerticesPerCell],
    peano4::grid::GridTraversalObserver& observer,
    TraversalScope&                      scope
  ) const;

  void deriveSubtreeDecomposition(const peano4::topology::Tree& tree);

  void deriveInterfaceMembership(const peano4::topology::Tree& tree);

  void assignInterfaceSlotsAndPairs();

  void reassignInterfaceTouchCounters();

  void rebuildInterfacePairLists();

  void markUnitsAdjacentToVertices(
    const peano4::topology::Tree&                                  tree,
    const std::vector<peano4::topology::Tree::ReclassifiedVertex>& vertices,
    std::vector<char>&                                             unitIsStale
  ) const;

#if PEANO_ASSERTIONS
  void validateShrunkInterfaceTopology(
    const peano4::topology::Tree&              tree,
    const peano4::topology::Tree::PruneExtent& pruneExtent,
    const InterfaceTopology&                   topologyBeforePrune,
    const std::vector<int>&                    staleUnits
  );
#endif

  peano4::grid::GridTraversalObserver& observerOfTask(
    peano4::grid::GridTraversalObserver& observer,
    int                                  taskNumber
  ) const;

  /** Stores persistent observer clones by observer type and task position. */
  mutable std::map<std::type_index, std::vector<std::unique_ptr<peano4::grid::GridTraversalObserver>>>
    _taskObserversOfType{};

  void deriveBlockRoots(
    const peano4::topology::Tree& tree,
    int                           taskLevel,
    bool                          splitHeavyBlocks,
    const RegridOverrides*        regridOverrides
  );

  void collectSubtreeTasks(
    const peano4::topology::Tree&       tree,
    const peano4::grid::AutomatonState& state,
    int                                 cellIndex,
    int                                 deepestTaskRootLevel,
    SubtreeDecomposition&               decomposition
  ) const;

  void deriveTransitionDependencies(
    const peano4::topology::Tree&    tree,
    const SubtreeDecomposition&      decomposition,
    std::vector<char>&               completionTokens,
    std::vector<std::vector<void*>>& dependencies,
    std::vector<int>&                spawnOrder
  ) const;

  void runParallelSweep(
    const peano4::topology::Tree&        tree,
    peano4::grid::GridTraversalObserver& observer,
    PoolSweepKind                        sweepKind,
    int                                  taskLevel,
    const std::function<void()>&         waitForRemoteData,
    const RegridOverrides*               regridOverrides
  );

  void runParallelReplay(peano4::grid::GridTraversalObserver& observer, const std::function<void()>& waitForRemoteData);
  void deriveReplayOrder();

  void runPartialRerecordSweep(
    const peano4::topology::Tree&        tree,
    peano4::grid::GridTraversalObserver& observer,
    const std::function<void()>&         waitForRemoteData
  );
};
