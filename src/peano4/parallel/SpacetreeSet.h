// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "peano4/grid/GridVertex.h"
#include "peano4/grid/PeanoCurve.h"
#include "peano4/grid/PeriodicTopology.h"
#include "peano4/grid/Spacetree.h"
#include "peano4/grid/SplitInstruction.h"
#include "peano4/grid/StackTopology.h"
#include "peano4/parallel/Node.h"
#include "peano4/parallel/TreeManagementMessage.h"
#include "peano4/stacks/Stack.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/instrumentation/Watch.h"
#include "tarch/mpi/IntegerMessage.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/Task.h"
#include "tarch/services/Service.h"

#include <algorithm>
#include <array>
#include <list>
#include <map>
#include <set>
#include <vector>

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

class peano4::parallel::SpacetreeSet final : public tarch::services::Service {
private:
  friend class peano4::grid::Spacetree;
  friend class peano4::parallel::tests::SpacetreeSetTest;

  static SpacetreeSet _singleton;

  std::vector<peano4::parallel::TreeManagementMessage> _unansweredMessages;

#ifdef PEANO_WITH_MPI
  MPI_Request           _treeManagementReceiveRequest;
  TreeManagementMessage _treeManagementReceiveBuffer;
  bool                  _treeManagementReceiveIsPosted;

  void postTreeManagementReceive();
#endif

  stacks::Stack<grid::GridVertex>&  getLocalExchangeStack(int treeId, int localStackId);
  std::vector<grid::PeriodicStamp>& getLocalExchangeStamps(int treeId, int localStackId);
  void pushLocalExchangeStamp(int treeId, int localStackId, const grid::PeriodicStamp& stamp);
  stacks::Stack<grid::GridVertex>&
       getParallelExchangeStack(int treeId, int slot, peano4::grid::StackTopology::ParallelExchangeStack which);
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

  template <class Container>
  static void resizeParallelExchangeStacks(Container& stackContainer);

  template <class Container>
  static void setLocalTrees(Container& stackContainer);

  template <class Container>
  static void exchangeAllHorizontalDataExchangeStacks(Container& stackContainer, int spacetreeId);

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
   * Semaphore to protect container holding all the local trees.
   */
  static tarch::multicore::BooleanSemaphore _semaphore;

  /**
   * I use this tag to identify messages send from one tree to another rank.
   * All answers go through an answer tag. To identify the right one, please
   * use getAnswerTag(). The tag should be const, but I set it in init() and
   * therefore I had to remove the const - even though its logically
   * not possible to change it.
   */
  int _requestMessageTag;

  /**
   * Never use this tag directly. It is the first tag of a series fo answer
   * tags. To find the right one for a particular application context, use
   * getAnswerTag().
   */
  int _answerMessageTag;

  /**
   * Dedicated tag for the AutomatonState handshake payload. It must not share _requestMessageTag,
   * which carries a persistent MPI_ANY_SOURCE TreeManagementMessage irecv: a pumped receive could
   * otherwise steal/truncate the (larger) AutomatonState message into the small TreeManagementMessage
   * buffer.
   */
  int _stateMessageTag;

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

  int getAnswerTag(int targetSpacetreeId) const;

  void exchangeHorizontalDataBetweenTrees(peano4::grid::GridTraversalObserver& observer);
  void streamDataFromSplittingTreesToNewTrees(peano4::grid::GridTraversalObserver& observer);

  void createNewTrees();

  void deleteClonedObservers();

  void addSpacetree(int masterId, int newTreeId);

  void cleanUpTrees(peano4::grid::GridTraversalObserver& observer);

  std::set<int> getLocalTreesMergingWithWorkers() const;

  void createObserverCloneIfRequired(peano4::grid::GridTraversalObserver& observer, int treeId);

  SpacetreeSet()                               = default;
  SpacetreeSet(const SpacetreeSet&)            = delete;
  SpacetreeSet& operator=(const SpacetreeSet&) = delete;
  SpacetreeSet(SpacetreeSet&&)                 = delete;
  SpacetreeSet& operator=(SpacetreeSet&&)      = delete;

public:
  ~SpacetreeSet() override = default;

  static SpacetreeSet& getInstance();

  void answerQuestions();

  void receiveDanglingMessages() override;

  void init(
    const tarch::la::Vector<DIMENSIONS, double>& offset,
    const tarch::la::Vector<DIMENSIONS, double>& width,
    const std::bitset<DIMENSIONS>&               periodicBC = 0
  );

  void shutdown() override;

  void traverse(peano4::grid::GridTraversalObserver& observer);

  peano4::grid::GridStatistics        getGridStatistics() const;
  const peano4::grid::GridStatistics& getGridStatistics(int treeId) const;

  peano4::grid::Spacetree&       getSpacetree(int id);
  const peano4::grid::Spacetree& getSpacetree(int id) const;

  bool split(int treeId, const grid::SplitInstruction& instruction, int targetRank);

  bool isLocalSpacetree(int treeId) const;

  std::set<int> getLocalSpacetrees() const;
};

#include "SpacetreeSet.cpph"
