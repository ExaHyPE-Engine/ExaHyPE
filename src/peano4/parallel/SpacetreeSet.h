// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "peano4/grid/BoundaryDataAggregation.h"
#include "peano4/grid/GridVertex.h"
#include "peano4/grid/PeanoCurve.h"
#include "peano4/grid/PeriodicTopology.h"
#include "peano4/grid/Spacetree.h"
#include "peano4/grid/SplitInstruction.h"
#include "peano4/grid/StackTopology.h"
#include "peano4/parallel/ExchangeChannels.h"
#include "peano4/parallel/TreeIdMapping.h"
#include "peano4/stacks/Stack.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/mpi/Diagnostics.h"
#include "tarch/mpi/PollPacing.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/Request.h"
#include "tarch/mpi/RequestGroup.h"
#include "tarch/multicore/Task.h"

namespace peano4::grid {
  class GridTraversalObserver;
  struct GridStatistics;
} // namespace peano4::grid

namespace peano4::parallel {
  class SpacetreeSet;
} // namespace peano4::parallel

namespace peano4::parallel::tests {
  class SpacetreeSetTest;
} // namespace peano4::parallel::tests

class peano4::parallel::SpacetreeSet final {
private:
  friend class peano4::grid::Spacetree;
  friend class peano4::parallel::tests::SpacetreeSetTest;

  static SpacetreeSet _singleton;

  /**
   * One entry of the collective tree-management exchange.
   *
   * All entries have the same width, so one integer per rank pair describes a
   * whole outgoing batch and no entry needs a length negotiation of its own.
   * The layout is free of padding because the entries travel as raw bytes.
   */
  struct TreeManagementRecord final {
    enum class Kind : int { CreateTree, RemoveEmptyChild };

    tarch::la::Vector<DIMENSIONS, double> rootOffset{};
    tarch::la::Vector<DIMENSIONS, double> rootWidth{};
    Kind                                  kind{Kind::CreateTree};
    int                                   masterTreeId{-1};
    int                                   workerTreeId{-1};
    int                                   rootTraversalInverted{0};
  };

  /** One split registered on a local tree whose worker ID is not known yet. */
  struct PendingSplit final {
    peano4::grid::SplitInstruction instruction;
    int                            sourceTreeId{-1};
    int                            targetRank{-1};
  };

  std::vector<PendingSplit>         _pendingSplits;
  std::vector<TreeManagementRecord> _pendingRemoteRecords;

  static int getRecordDestinationRank(const TreeManagementRecord& record);

  void exchangeTreeManagementRecords();
  void createTree(const TreeManagementRecord& record);

  stacks::Stack<grid::GridVertex>&  getLocalExchangeStack(int treeId, int localStackId);
  std::vector<grid::PeriodicStamp>& getLocalExchangeStamps(int treeId, int localStackId);
  void pushLocalExchangeStamp(int treeId, int localStackId, const grid::PeriodicStamp& stamp);
  stacks::Stack<grid::GridVertex>&
  getParallelExchangeStack(int treeId, int slot, peano4::grid::StackTopology::ParallelExchangeStack which);
  peano4::grid::BoundaryDataAggregation<stacks::Stack<grid::GridVertex>>& getBoundaryDataAggregation();
  int                                                                     getContainerId() const;
  void registerLocalSpacetree(peano4::grid::Spacetree& tree);
  void unregisterLocalSpacetree(int id);

  void stageCrossRankPeriodicEntry(
    int                        sendTree,
    int                        recvTree,
    const grid::PeriodicStamp& stamp,
    int                        depositStack,
    grid::GridVertex           data
  );
  std::vector<grid::CrossRankPeriodicEntry<grid::GridVertex>>& getCrossRankPeriodicStaging();
  void                                                         clearCrossRankPeriodicStaging();

  /**
   * Each task triggers the traversal of one specific spacetree. After
   * that, we might directly trigger the data exchanges. Yet, this is not a
   * good idea as other tasks might linger in the background not have sent
   * the data out yet. So we don't to anything here.
   */
  class TraverseTask final : public tarch::multicore::Task {
  private:
    grid::Spacetree&             _spacetree;
    grid::GridTraversalObserver* _observer;
    const bool                   _invertTreeTraversalDirectionBeforeWeStart;

  public:
    TraverseTask(
      grid::Spacetree&             tree,
      grid::GridTraversalObserver& observer,
      bool                         invertTreeTraversalDirectionBeforeWeStart
    );

    bool run() override;
  };

public:
  /**
   * In its role as the grid-vertex data container, the set mirrors the
   * StackContainer interface, so the templated exchange routines run on grid
   * vertices and user data alike.
   */
  using Stack       = stacks::Stack<grid::GridVertex>;
  using ElementType = grid::GridVertex;

  /**
   * Container id of the grid vertices in the aggregated boundary exchange. The
   * data repository numbers the user containers from the next value up.
   */
  static constexpr int GridVertexContainerId = 0;

  template <class Container>
  static void resizeParallelExchangeStacks(Container& stackContainer);

  template <class Container>
  static void setLocalTrees(Container& stackContainer);

  template <class Container>
  static void exchangeAllHorizontalDataExchangeStacks(Container& stackContainer, int spacetreeId);

  template <class Container>
  static void postAggregatedBoundaryExchange(Container& stackContainer, int remoteRank);

  template <class Container>
  static void unpackAggregatedBoundaryExchange(Container& stackContainer, int spacetreeId, int remoteRank);

  template <class Container>
  static void purgePeriodicBoundaryDataStacks(Container& stackContainer, int spacetreeId);

  template <class Container>
  static void exchangeAllPeriodicBoundaryDataStacks(Container& stackContainer, int spacetreeId);

  template <class Container>
  static void exchangeCrossRankPeriodicData(Container& stackContainer);

  template <class Container>
  static void finishAllOutstandingSendsAndReceives(Container& stackContainer, int spacetreeId);

  template <class Container>
  static void streamDataFromSplittingTreeToNewTree(Container& stackContainer, int master, int worker);

  template <class Container>
  static void streamDataFromJoiningTreeToMasterTree(Container& stackContainer, int master, int worker);

private:
  /**
   * These are the local spacetrees.
   */
  std::list<peano4::grid::Spacetree>                            _spacetrees;
  std::array<peano4::grid::Spacetree*, MAX_SPACETREES_PER_RANK> _spacetreeByLocalTreeId{{nullptr}};

  /**
   * Global periodic-axis configuration, cached from init(). Bit d set means axis
   * d is periodic.
   */
  std::bitset<DIMENSIONS> _periodicBC;

  /**
   * Resolved periodic partner topology: per key and side the trees touching
   * that side of the wrap. Data produced at one side is routed to every tree
   * of the opposite side. Rebuilt only on a topology change (see
   * rebuildPeriodicAssociations); stable in-between and drives the per-sweep
   * exchange of grid vertices and user data alike.
   */
  peano4::grid::PeriodicRouting _periodicRouting;

  /**
   * Every cross-rank periodic routing as (producerTree, consumerTree) pairs.
   * Identical on all ranks (derived from the globally gathered descriptors), so a
   * receiver knows which channels to post even when a channel carries no payload
   * this sweep. Used by exchangeCrossRankPeriodicData to drive the non-blocking
   * point-to-point exchange in lockstep.
   */
  std::set<std::pair<int, int>> _crossRankPeriodicChannels;

  /**
   * Sorted snapshots of this rank's periodic producer/consumer descriptors as
   * of the last routing rebuild. Compared each sweep to detect a topology
   * change so the (collective) discovery only runs on a regrid, not every
   * sweep. Sorted so the comparison is order-independent (the SFC direction
   * alternates).
   */
  std::vector<peano4::grid::PeriodicDescriptor> _lastPeriodicProducerDescriptors;
  std::vector<peano4::grid::PeriodicDescriptor> _lastPeriodicConsumerDescriptors;

  /**
   * Cross-rank grid vertices staged by the set in its container role.
   */
  std::vector<peano4::grid::CrossRankPeriodicEntry<peano4::grid::GridVertex>> _crossRankPeriodicGridVertexStaging;

  /**
   * Aggregated boundary buffers of the grid vertices, again in the set's
   * container role.
   */
  peano4::grid::BoundaryDataAggregation<stacks::Stack<grid::GridVertex>> _gridVertexBoundaryAggregation;

  /**
   * Remote ranks this rank exchanges boundary data with, sorted, and per entry
   * the number of local trees that have at least one neighbour on that rank.
   * Both follow from the tree topology alone, so every container aggregates
   * along the same layout and the set derives it once per sweep.
   */
  std::vector<int> _boundaryAggregationRemoteRanks;
  std::vector<int> _boundaryAggregationContributors;

  /**
   * Per local tree slot the sorted remote ranks that tree touches. A tree walks
   * this list to hand on to the contributor countdown and, later, to look for
   * the data addressed to it.
   */
  std::array<std::vector<int>, MAX_SPACETREES_PER_RANK> _boundaryAggregationRanksPerTree;

  /**
   * Names the sweep the topology above describes. A container compares it
   * against the sweep its own buffers were prepared for, so the first tree to
   * reach the exchange prepares them and the others find them ready.
   */
  int _boundaryAggregationGeneration{0};

  /**
   * Whether any rank creates a communicator this sweep. The tree-management
   * headers carry every rank's demand, so all ranks take the same value and
   * the collective communicator creation is skipped in lockstep on a settled
   * partition. The first sweep always builds.
   */
  bool _communicatorRebuildRequired{true};

  void rebuildBoundaryAggregationTopology();

  bool hasPeriodicBoundaryConditions() const { return _periodicBC.any(); }

  void rebuildPeriodicAssociations();

  /**
   * I create/clone one observer per local tree. This is an
   * on-the-fly process. At the end of the set traversal, we delete all of
   * clones. Originally, I did this delete right after the creation and the
   * subsequent traversal. However, we need the observer clone once more for
   * the data exchange. To avoid reclones, I store all clones in this map and
   * then delete them en bloc.
   *
   * @see deleteClonedObservers
   */
  std::map<int, peano4::grid::GridTraversalObserver*> _clonedObserver;

  void exchangeHorizontalDataBetweenTrees(peano4::grid::GridTraversalObserver& observer);
  void streamDataFromSplittingTreesToNewTrees(peano4::grid::GridTraversalObserver& observer);

  void createNewTrees();

  void deleteClonedObservers();

  void cleanUpTrees();

  std::set<int> getLocalTreesMergingWithWorkers() const;

  void createObserverCloneIfRequired(peano4::grid::GridTraversalObserver& observer, int treeId);

  SpacetreeSet() = default;

public:
  /** One exact split instruction and the rank on which its worker will live. */
  struct SplitRequest final {
    peano4::grid::SplitInstruction instruction;
    int                            targetRank;
  };

  SpacetreeSet(const SpacetreeSet&)            = delete;
  SpacetreeSet(SpacetreeSet&&)                 = delete;
  SpacetreeSet& operator=(const SpacetreeSet&) = delete;
  SpacetreeSet& operator=(SpacetreeSet&&)      = delete;
  ~SpacetreeSet()                              = default;

  static SpacetreeSet& getInstance();

  void init(
    const tarch::la::Vector<DIMENSIONS, double>& offset,
    const tarch::la::Vector<DIMENSIONS, double>& width,
    const std::bitset<DIMENSIONS>&               periodicBC = 0
  );

  void shutdown();

  void traverse(peano4::grid::GridTraversalObserver& observer);

  peano4::grid::GridStatistics        getGridStatistics() const;
  const peano4::grid::GridStatistics& getGridStatistics(int treeId) const;

  peano4::grid::Spacetree&       getSpacetree(int id);
  const peano4::grid::Spacetree& getSpacetree(int id) const;
  bool                           maySplit(int treeId) const;

  /** Offset and width of one local tree's root cell. */
  struct TreeExtent {
    tarch::la::Vector<DIMENSIONS, double> offset;
    tarch::la::Vector<DIMENSIONS, double> width;
  };

  std::vector<TreeExtent> getLocalTreeExtents() const;

  /** Register one exact split. This convenience overload uses the batched implementation. */
  bool split(int treeId, const grid::SplitInstruction& instruction, int targetRank);

  /**
   * Register all exact splits together. Remote IDs are reserved once per target rank and requests are committed in
   * input order. The method returns false without reserving an ID when the source tree cannot currently split.
   */
  bool split(int treeId, const std::vector<SplitRequest>& requests);

  bool isLocalSpacetree(int treeId) const;

  std::set<int> getLocalSpacetrees() const;
};

#include "SpacetreeSet.cpph"
