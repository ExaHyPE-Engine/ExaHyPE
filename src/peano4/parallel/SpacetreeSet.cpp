// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SpacetreeSet.h"

#include "peano4/grid/GridStatistics.h"
#include "peano4/grid/GridTraversalObserver.h"
#include "peano4/grid/PeanoCurve.h"
#include "peano4/grid/Spacetree.h"
#include "peano4/grid/StackTopology.h"
#include "peano4/parallel/ExchangeChannels.h"
#include "peano4/parallel/TreeIdMapping.h"
#include "tarch/Termination.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/multicore.h"

peano4::parallel::SpacetreeSet peano4::parallel::SpacetreeSet::_singleton;

peano4::parallel::SpacetreeSet& peano4::parallel::SpacetreeSet::getInstance() { return _singleton; }

peano4::stacks::Stack<peano4::grid::GridVertex>&
peano4::parallel::SpacetreeSet::getLocalExchangeStack(int treeId, int localStackId) {
  return getSpacetree(treeId).getLocalExchangeVertexStack(localStackId);
}

std::vector<peano4::grid::PeriodicStamp>&
peano4::parallel::SpacetreeSet::getLocalExchangeStamps(int treeId, int localStackId) {
  return getSpacetree(treeId).getLocalExchangeVertexStamps(localStackId);
}

void peano4::parallel::SpacetreeSet::pushLocalExchangeStamp(
  int                        treeId,
  int                        localStackId,
  const grid::PeriodicStamp& stamp
) {
  getLocalExchangeStamps(treeId, localStackId).push_back(stamp);
}

void peano4::parallel::SpacetreeSet::stageCrossRankPeriodicEntry(
  int                        sourceTreeId,
  int                        destinationTreeId,
  const grid::PeriodicStamp& stamp,
  int                        destinationStackId,
  grid::GridVertex           vertexData
) {
  _crossRankPeriodicGridVertexStaging.push_back(
    grid::CrossRankPeriodicEntry<grid::GridVertex>{
      sourceTreeId,
      destinationTreeId,
      stamp,
      destinationStackId,
      vertexData
    }
  );
}

std::vector<peano4::grid::CrossRankPeriodicEntry<peano4::grid::GridVertex>>&
peano4::parallel::SpacetreeSet::getCrossRankPeriodicStaging() {
  return _crossRankPeriodicGridVertexStaging;
}

void peano4::parallel::SpacetreeSet::clearCrossRankPeriodicStaging() { _crossRankPeriodicGridVertexStaging.clear(); }

peano4::stacks::Stack<peano4::grid::GridVertex>& peano4::parallel::SpacetreeSet::getParallelExchangeStack(
  int                                                treeId,
  int                                                neighbourSlot,
  peano4::grid::StackTopology::ParallelExchangeStack exchangeStack
) {
  return getSpacetree(treeId).getParallelExchangeVertexStack(neighbourSlot, exchangeStack);
}

peano4::grid::BoundaryDataAggregation<peano4::stacks::Stack<peano4::grid::GridVertex>>&
peano4::parallel::SpacetreeSet::getBoundaryDataAggregation() {
  return _gridVertexBoundaryAggregation;
}

int peano4::parallel::SpacetreeSet::getContainerId() const { return GridVertexContainerId; }

void peano4::parallel::SpacetreeSet::registerLocalSpacetree(peano4::grid::Spacetree& tree) {
  const int localTreeId = peano4::parallel::TreeIdMapping::getLocalTreeId(tree._id);
  assertion(
    localTreeId >= 0 and localTreeId < MAX_SPACETREES_PER_RANK,
    "Invalid local tree id.",
    tree._id,
    localTreeId,
    tarch::mpi::Rank::getInstance().getRank()
  );
  assertion(
    _spacetreeByLocalTreeId[localTreeId] == nullptr,
    "Duplicate local tree id.",
    tree._id,
    localTreeId,
    tarch::mpi::Rank::getInstance().getRank()
  );
  _spacetreeByLocalTreeId[localTreeId] = &tree;
}

void peano4::parallel::SpacetreeSet::unregisterLocalSpacetree(int treeId) {
  const int localTreeId = peano4::parallel::TreeIdMapping::getLocalTreeId(treeId);
  if (localTreeId >= 0 and localTreeId < MAX_SPACETREES_PER_RANK) {
    [[maybe_unused]] auto* const tree = _spacetreeByLocalTreeId[localTreeId];
    assertion(
      tree == nullptr or tree->_id == treeId,
      "Spacetree lookup mismatch.",
      treeId,
      (tree == nullptr ? -1 : tree->_id),
      localTreeId
    );
    _spacetreeByLocalTreeId[localTreeId] = nullptr;
  }
}

void peano4::parallel::SpacetreeSet::init(
  const tarch::la::Vector<DIMENSIONS, double>& offset,
  const tarch::la::Vector<DIMENSIONS, double>& width,
  const std::bitset<DIMENSIONS>&               periodicBC
) {
  _spacetreeByLocalTreeId.fill(nullptr);

  assertion(
    (peano4::parallel::TreeIdMapping::getInstance().getNumberOfRegisteredTrees() == 1
     and tarch::mpi::Rank::getInstance().getRank() == 0)
      or (peano4::parallel::TreeIdMapping::getInstance().getNumberOfRegisteredTrees() == 0
          and tarch::mpi::Rank::getInstance().getRank() != 0),
    peano4::parallel::TreeIdMapping::getInstance().getNumberOfRegisteredTrees(),
    offset,
    width,
    tarch::mpi::Rank::getInstance().getRank()
  );

  _periodicBC = periodicBC;

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    _spacetrees.emplace_back(0, offset, width, periodicBC);
    registerLocalSpacetree(_spacetrees.back());
  }
}

void peano4::parallel::SpacetreeSet::shutdown() {
  _pendingSplits.clear();
  _pendingRemoteRecords.clear();
}

/** @return Rank that has to act on the record. */
int peano4::parallel::SpacetreeSet::getRecordDestinationRank(const TreeManagementRecord& record) {
  switch (record.kind) {
  case TreeManagementRecord::Kind::CreateTree:
    return peano4::parallel::TreeIdMapping::getRank(record.workerTreeId);
  case TreeManagementRecord::Kind::RemoveEmptyChild:
    return peano4::parallel::TreeIdMapping::getRank(record.masterTreeId);
  }
  assertion(false, static_cast<int>(record.kind));
  return -1;
}

/** Instantiate one worker tree from the record its master issued. */
void peano4::parallel::SpacetreeSet::createTree(const TreeManagementRecord& record) {
  // The worker inherits its master's bounding box and traversal inversion. The
  // periodic configuration is global, so it comes from this rank's own copy.
  _spacetrees.emplace_back(
    record.workerTreeId,
    record.masterTreeId,
    record.rootOffset,
    record.rootWidth,
    record.rootTraversalInverted != 0,
    _periodicBC
  );
  registerLocalSpacetree(_spacetrees.back());
}

/**
 * Settle all pending tree management in three collectives.
 *
 * Every rank reaches all three unconditionally, so no rank ever waits for a
 * decision that a peer is not yet in a position to make. The first exchange
 * publishes, per pair of ranks, how many records travel and how many tree IDs
 * the sender wants reserved. The second carries the records. The third returns
 * the reserved IDs, whose counts both sides already know from the first
 * exchange.
 *
 * A rank owns its tree slots, so it reserves IDs without asking anybody. The
 * IDs come back in the order in which the requester registered its splits,
 * which keeps a batched split deterministic. Registering those splits is part
 * of this routine, so it has to run before a traversal reads a tree's split
 * state.
 */
void peano4::parallel::SpacetreeSet::exchangeTreeManagementRecords() {
  const int  numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  const auto communicator  = tarch::mpi::Rank::getInstance().getCommunicator();

  std::vector<std::vector<TreeManagementRecord>> outgoingRecords(numberOfRanks);
  for (const auto& record : _pendingRemoteRecords) {
    outgoingRecords[getRecordDestinationRank(record)].push_back(record);
  }
  _pendingRemoteRecords.clear();

  // Three headers per peer: the number of records, the number of tree IDs
  // this rank wants that peer to reserve, and whether this rank knows of a
  // communicator that has to be created this sweep. Every tree that registers
  // this sweep is known before the exchange to the rank that initiates it, so
  // the maximum over all flags is a globally uniform decision on whether the
  // collective communicator creation has any work.
  bool localCommunicatorDemand =
    peano4::parallel::ExchangeChannels::getInstance().hasPendingCommunicatorRequests() or not _pendingSplits.empty();
  for (const auto& records : outgoingRecords) {
    localCommunicatorDemand = localCommunicatorDemand or not records.empty();
  }

  constexpr int    HeadersPerRank = 3;
  std::vector<int> sendHeaders(HeadersPerRank * numberOfRanks, 0);
  std::vector<int> receiveHeaders(HeadersPerRank * numberOfRanks, 0);
  for (int rank = 0; rank < numberOfRanks; rank++) {
    sendHeaders[HeadersPerRank * rank]     = static_cast<int>(outgoingRecords[rank].size());
    sendHeaders[HeadersPerRank * rank + 2] = localCommunicatorDemand ? 1 : 0;
  }
  for (const auto& pendingSplit : _pendingSplits) {
    sendHeaders[HeadersPerRank * pendingSplit.targetRank + 1]++;
  }
  tarch::mpi::alltoall(
    sendHeaders.data(),
    HeadersPerRank,
    MPI_INT,
    receiveHeaders.data(),
    HeadersPerRank,
    MPI_INT,
    communicator
  );

  _communicatorRebuildRequired = localCommunicatorDemand;
  for (int rank = 0; rank < numberOfRanks; rank++) {
    _communicatorRebuildRequired = _communicatorRebuildRequired or receiveHeaders[HeadersPerRank * rank + 2] != 0;
  }

  constexpr auto   RecordSize = static_cast<int>(sizeof(TreeManagementRecord));
  std::vector<int> sendCounts(numberOfRanks, 0);
  std::vector<int> sendDisplacements(numberOfRanks, 0);
  std::vector<int> receiveCounts(numberOfRanks, 0);
  std::vector<int> receiveDisplacements(numberOfRanks, 0);
  std::size_t      numberOfSentRecords     = 0;
  std::size_t      numberOfReceivedRecords = 0;
  for (int rank = 0; rank < numberOfRanks; rank++) {
    sendDisplacements[rank]    = static_cast<int>(numberOfSentRecords * RecordSize);
    receiveDisplacements[rank] = static_cast<int>(numberOfReceivedRecords * RecordSize);
    sendCounts[rank]           = sendHeaders[HeadersPerRank * rank] * RecordSize;
    receiveCounts[rank]        = receiveHeaders[HeadersPerRank * rank] * RecordSize;
    numberOfSentRecords += static_cast<std::size_t>(sendHeaders[HeadersPerRank * rank]);
    numberOfReceivedRecords += static_cast<std::size_t>(receiveHeaders[HeadersPerRank * rank]);
  }
  assertion(
    std::max(numberOfSentRecords, numberOfReceivedRecords) * RecordSize
      <= static_cast<std::size_t>(std::numeric_limits<int>::max()),
    numberOfSentRecords,
    numberOfReceivedRecords,
    RecordSize
  );

  std::vector<TreeManagementRecord> sendBuffer;
  sendBuffer.reserve(numberOfSentRecords);
  for (const auto& records : outgoingRecords) {
    sendBuffer.insert(sendBuffer.end(), records.begin(), records.end());
  }

  std::vector<TreeManagementRecord> receiveBuffer(numberOfReceivedRecords);
  tarch::mpi::alltoallv(
    sendBuffer.data(),
    sendCounts.data(),
    sendDisplacements.data(),
    MPI_BYTE,
    receiveBuffer.data(),
    receiveCounts.data(),
    receiveDisplacements.data(),
    MPI_BYTE,
    communicator
  );

  for (const auto& record : receiveBuffer) {
    switch (record.kind) {
    case TreeManagementRecord::Kind::CreateTree:
      createTree(record);
      break;
    case TreeManagementRecord::Kind::RemoveEmptyChild:
      logInfo(
        "Remote child tree " << record.workerTreeId << " of local tree " << record.masterTreeId
                             << " became empty and was removed."
      );
      break;
    }
  }

  std::vector<int> reservedIds;
  std::vector<int> reservedIdCounts(numberOfRanks, 0);
  std::vector<int> reservedIdDisplacements(numberOfRanks, 0);
  std::vector<int> requestedIdCounts(numberOfRanks, 0);
  std::vector<int> requestedIdDisplacements(numberOfRanks, 0);
  int              numberOfRequestedIds = 0;
  for (int rank = 0; rank < numberOfRanks; rank++) {
    reservedIdDisplacements[rank]  = static_cast<int>(reservedIds.size());
    reservedIdCounts[rank]         = receiveHeaders[HeadersPerRank * rank + 1];
    requestedIdCounts[rank]        = sendHeaders[HeadersPerRank * rank + 1];
    requestedIdDisplacements[rank] = numberOfRequestedIds;
    numberOfRequestedIds += requestedIdCounts[rank];
    if (reservedIdCounts[rank] > 0) {
      const auto ids = peano4::parallel::TreeIdMapping::getInstance().reserveIds(reservedIdCounts[rank]);
      reservedIds.insert(reservedIds.end(), ids.begin(), ids.end());
    }
  }

  std::vector<int> requestedIds(static_cast<std::size_t>(numberOfRequestedIds));
  tarch::mpi::alltoallv(
    reservedIds.data(),
    reservedIdCounts.data(),
    reservedIdDisplacements.data(),
    MPI_INT,
    requestedIds.data(),
    requestedIdCounts.data(),
    requestedIdDisplacements.data(),
    MPI_INT,
    communicator
  );

  std::vector<int> nextIdPerRank(requestedIdDisplacements);
  for (const auto& pendingSplit : _pendingSplits) {
    const int workerTreeId = requestedIds[nextIdPerRank[pendingSplit.targetRank]++];
    if (peano4::parallel::TreeIdMapping::getRank(workerTreeId) != pendingSplit.targetRank) {
      tarch::terminateWithError("Received a worker tree ID that is not owned by the requested target rank.");
    }
    getSpacetree(pendingSplit.sourceTreeId).split(workerTreeId, pendingSplit.instruction);
    logDebug(
      "Trigger split of tree " << pendingSplit.sourceTreeId << " into tree " << workerTreeId << " on rank "
                               << pendingSplit.targetRank << " with instruction " << pendingSplit.instruction
    );
  }
  _pendingSplits.clear();
}

peano4::parallel::SpacetreeSet::TraverseTask::TraverseTask(
  grid::Spacetree&             tree,
  grid::GridTraversalObserver& observer,
  bool                         invertTreeTraversalDirectionBeforeWeStart
) :
  Task(Task::NoTaskType, Task::DefaultPriority),
  _spacetree(tree),
  _observer(&observer),
  _invertTreeTraversalDirectionBeforeWeStart(invertTreeTraversalDirectionBeforeWeStart) {}

bool peano4::parallel::SpacetreeSet::TraverseTask::run() {
  if (_invertTreeTraversalDirectionBeforeWeStart) {
    _spacetree._root.setInverted(not _spacetree._root.getInverted());
  }
  assertion(_observer);
  _spacetree.traverse(*_observer);
  return false;
}

/**
 * Create one observer clone for the tree. Repeated calls are idempotent so
 * tree creation during MPI progress can safely prepare a missing clone.
 */
void peano4::parallel::SpacetreeSet::createObserverCloneIfRequired(
  peano4::grid::GridTraversalObserver& observer,
  int                                  treeId
) {
  const auto observerIterator = _clonedObserver.find(treeId);
  if (observerIterator != _clonedObserver.end() and observerIterator->second != nullptr) {
    return;
  }

  auto& observerSlot = _clonedObserver[treeId];
  if (observerSlot == nullptr) {
    observerSlot = observer.clone(treeId);
  }
  assertion(
    _clonedObserver.count(treeId) == 1 and _clonedObserver[treeId] != nullptr,
    treeId,
    _clonedObserver.count(treeId)
  );
}

/**
 * Copy data from splitting trees to their workers.
 *
 * A split spans two sweeps. The first sweep announces the split and clones the
 * grid to the worker. The next sweep streams user data through vertical stacks,
 * lets the worker incorporate it, and restores the traversal order.
 *
 * Spacetree::traverse() inverts the master's traversal direction before this
 * routine runs, so the master's input stack already has the order required by
 * the worker. Both source and destination invoke the exchange to post the MPI
 * send and receive; the local counterpart is empty for a same-rank split.
 * Only vertical stream stacks are transferred. Persistent local stacks remain
 * with their owning tree.
 *
 * Grid-stack clones target disjoint worker stacks and therefore share the
 * available threads. Their task group completes before the ordered MPI and
 * observer streams begin, so message ordering remains unchanged.
 *
 * @see streamDataFromSplittingTreeToNewTree()
 * @see peano4::grid::Spacetree::sendUserData()
 */
void peano4::parallel::SpacetreeSet::streamDataFromSplittingTreesToNewTrees(
  peano4::grid::GridTraversalObserver& observer
) {
  struct GridCloneJob final {
    const peano4::stacks::Stack<peano4::grid::GridVertex>* source;
    peano4::stacks::Stack<peano4::grid::GridVertex>*       target;
  };

  std::vector<GridCloneJob> gridCloneJobs;
  for (auto& parent : _spacetrees) {
    const int sourceStackNumber = grid::PeanoCurve::getInputStackNumber(parent._root);
    for (auto& worker : parent._hasSplit) {
      const int neighbourSlot = parent.getNeighbourSlot(worker);
      assertion(neighbourSlot >= 0, neighbourSlot, parent._id, worker, parent.toString());

      auto& targetStack = parent.getParallelExchangeVertexStack(
        neighbourSlot,
        grid::StackTopology::ParallelExchangeStack::VerticalOutput
      );
      assertion(targetStack.empty(), parent._id, worker, sourceStackNumber);
      gridCloneJobs.push_back({&parent.getLocalExchangeVertexStack(sourceStackNumber), &targetStack});
    }
  }

  const auto numberOfCloneTasks = std::min(
    gridCloneJobs.size(),
    static_cast<std::size_t>(std::max(1, tarch::multicore::Core::getInstance().getNumberOfThreads()))
  );
  if (numberOfCloneTasks <= 1) {
    for (const auto& cloneJob : gridCloneJobs) {
      cloneJob.target->clone(*cloneJob.source);
    }
  } else {
    std::vector<tarch::multicore::Task*> cloneTasks;
    cloneTasks.reserve(numberOfCloneTasks);
    for (std::size_t taskNumber = 0; taskNumber < numberOfCloneTasks; ++taskNumber) {
      cloneTasks.push_back(new tarch::multicore::TaskWithCopyOfFunctor(
        tarch::multicore::Task::NoTaskType,
        tarch::multicore::Task::DefaultPriority,
        [cloneJobs = &gridCloneJobs, taskNumber, numberOfCloneTasks]() -> bool {
          for (std::size_t jobNumber = taskNumber; jobNumber < cloneJobs->size(); jobNumber += numberOfCloneTasks) {
            const auto& cloneJob = (*cloneJobs)[jobNumber];
            cloneJob.target->clone(*cloneJob.source);
          }
          return false;
        }
      ));
    }
    tarch::multicore::spawnTasks(cloneTasks);
  }

  // Keep stream setup ordered: the MPI metadata handshakes and observer
  // stacks have to match on sender and receiver even though grid cloning does
  // not.
  for (auto& parent : _spacetrees) {
    for (auto& worker : parent._hasSplit) {
      streamDataFromSplittingTreeToNewTree(*this, parent._id, worker);
      // MPI progress can create a remote tree after observer preparation, so
      // the idempotent clone call must precede every dereference here.
      createObserverCloneIfRequired(observer, parent._id);
      _clonedObserver[parent._id]->streamDataFromSplittingTreeToNewTree(worker);
    }
  }

  for (auto& tree : _spacetrees) {
    if (tree._spacetreeState == peano4::grid::SpacetreeState::EmptyRun) {
      streamDataFromSplittingTreeToNewTree(*this, tree._masterId, tree._id);
      createObserverCloneIfRequired(observer, tree._masterId);
      _clonedObserver[tree._masterId]->streamDataFromSplittingTreeToNewTree(tree._id);
    }
  }

  for (auto& tree : _spacetrees) {
    finishAllOutstandingSendsAndReceives(*this, tree._id);
    createObserverCloneIfRequired(observer, tree._id);
    _clonedObserver[tree._id]->finishAllOutstandingSendsAndReceives();
  }
}

/**
 * Collect the periodic descriptors recorded by all local trees during the
 * last traversal and resolve them into the routing table. Under MPI a
 * rendezvous completes the picture across ranks: every descriptor travels to
 * a broker rank derived from its key, and each rank receives back the full
 * record set of exactly the keys it participates in. The routing table and
 * the traffic therefore scale with the rank's own periodic surface.
 *
 * The routine reads every tree's recorded descriptors, so it has to run
 * after all local traversals and before any payload routing. With periodic
 * boundary conditions it contains a per-sweep collective, so all ranks reach
 * it in lockstep. On a stationary grid it degenerates to the snapshot
 * comparison plus the one-int allreduce.
 */
void peano4::parallel::SpacetreeSet::rebuildPeriodicAssociations() {
  if (not hasPeriodicBoundaryConditions()) {
    return;
  }

  std::vector<peano4::grid::PeriodicDescriptor> producers;
  std::vector<peano4::grid::PeriodicDescriptor> consumers;
  for (const auto& tree : _spacetrees) {
    producers
      .insert(producers.end(), tree._periodicProducerDescriptors.begin(), tree._periodicProducerDescriptors.end());
    consumers
      .insert(consumers.end(), tree._periodicConsumerDescriptors.begin(), tree._periodicConsumerDescriptors.end());
  }

  const auto descriptorLess = [](
                                const peano4::grid::PeriodicDescriptor& leftDescriptor,
                                const peano4::grid::PeriodicDescriptor& rightDescriptor
                              ) {
    if (leftDescriptor.stamp.key != rightDescriptor.stamp.key) {
      return leftDescriptor.stamp.key < rightDescriptor.stamp.key;
    }
    if (leftDescriptor.stamp.side != rightDescriptor.stamp.side) {
      return static_cast<int>(leftDescriptor.stamp.side) < static_cast<int>(rightDescriptor.stamp.side);
    }
    return leftDescriptor.treeId < rightDescriptor.treeId;
  };
  const auto descriptorEqual = [](
                                 const peano4::grid::PeriodicDescriptor& leftDescriptor,
                                 const peano4::grid::PeriodicDescriptor& rightDescriptor
                               ) {
    return leftDescriptor.stamp.key == rightDescriptor.stamp.key
      and leftDescriptor.stamp.side == rightDescriptor.stamp.side and leftDescriptor.treeId == rightDescriptor.treeId;
  };

  std::sort(producers.begin(), producers.end(), descriptorLess);
  producers.erase(std::unique(producers.begin(), producers.end(), descriptorEqual), producers.end());
  std::sort(consumers.begin(), consumers.end(), descriptorLess);
  consumers.erase(std::unique(consumers.begin(), consumers.end(), descriptorEqual), consumers.end());

  // Sorting keeps the alternating traversal direction from appearing as a
  // topology change.
  const auto descriptorsChanged = [&descriptorEqual](
                                    const std::vector<peano4::grid::PeriodicDescriptor>& currentDescriptors,
                                    const std::vector<peano4::grid::PeriodicDescriptor>& cachedDescriptors
                                  ) {
    return currentDescriptors.size() != cachedDescriptors.size()
      or not std::equal(
             currentDescriptors.begin(),
             currentDescriptors.end(),
             cachedDescriptors.begin(),
             descriptorEqual
      );
  };
  const int localTopologyChanged = descriptorsChanged(producers, _lastPeriodicProducerDescriptors)
      or descriptorsChanged(consumers, _lastPeriodicConsumerDescriptors)
    ? 1
    : 0;

  // A change on one rank can move another rank's partner, so every rank uses
  // the same collective rebuild decision.
  int       globalTopologyChanged = localTopologyChanged;
  const int numberOfRanks         = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  if (numberOfRanks > 1) {
    tarch::mpi::allreduce(
      &localTopologyChanged,
      &globalTopologyChanged,
      1,
      MPI_INT,
      MPI_MAX,
      tarch::mpi::Rank::getInstance().getCommunicator()
    );
  }

  if (not globalTopologyChanged) {
    return;
  }
  _lastPeriodicProducerDescriptors = producers;
  _lastPeriodicConsumerDescriptors = consumers;

  // Rendezvous discovery: every descriptor travels to a broker rank derived
  // from its key, the broker collects both sides of each key, and each
  // participating rank gets back exactly the records of the keys it produces
  // or consumes. Traffic and per-rank table size scale with the local
  // periodic surface. The two alltoall rounds are reached in lockstep by all
  // ranks because globalTopologyChanged is a collective decision, so they
  // cannot deadlock. Tree IDs are global, so getRank()
  // tells each side which rank to route to.
  if (numberOfRanks > 1) {
    constexpr int RecordSize   = peano4::grid::PackedPeriodicDescriptorSize + 1;
    const auto    communicator = tarch::mpi::Rank::getInstance().getCommunicator();

    const auto exchangeBuffers = [&](const std::vector<std::vector<int>>& outgoing) {
      std::vector<int> sendCounts(numberOfRanks, 0);
      std::vector<int> sendDisplacements(numberOfRanks, 0);
      int              totalSendCount = 0;
      for (int rank = 0; rank < numberOfRanks; rank++) {
        sendCounts[rank]        = static_cast<int>(outgoing[rank].size());
        sendDisplacements[rank] = totalSendCount;
        totalSendCount += sendCounts[rank];
      }
      std::vector<int> sendBuffer;
      sendBuffer.reserve(totalSendCount);
      for (const auto& block : outgoing) {
        sendBuffer.insert(sendBuffer.end(), block.begin(), block.end());
      }

      std::vector<int> receiveCounts(numberOfRanks, 0);
      tarch::mpi::alltoall(sendCounts.data(), 1, MPI_INT, receiveCounts.data(), 1, MPI_INT, communicator);

      std::vector<int> receiveDisplacements(numberOfRanks, 0);
      int              totalReceiveCount = 0;
      for (int rank = 0; rank < numberOfRanks; rank++) {
        receiveDisplacements[rank] = totalReceiveCount;
        totalReceiveCount += receiveCounts[rank];
      }
      std::vector<int> receiveBuffer(static_cast<std::size_t>(totalReceiveCount));
      tarch::mpi::alltoallv(
        sendBuffer.data(),
        sendCounts.data(),
        sendDisplacements.data(),
        MPI_INT,
        receiveBuffer.data(),
        receiveCounts.data(),
        receiveDisplacements.data(),
        MPI_INT,
        communicator
      );
      return std::make_pair(std::move(receiveBuffer), std::move(receiveDisplacements));
    };

    std::vector<std::vector<int>> outgoingBrokerRecords(numberOfRanks);
    const auto                    stageDescriptors =
      [&](const std::vector<peano4::grid::PeriodicDescriptor>& descriptors, int descriptorKind) {
        for (const auto& descriptor : descriptors) {
          auto& block = outgoingBrokerRecords[peano4::grid::periodicKeyBroker(descriptor.stamp.key, numberOfRanks)];
          block.resize(block.size() + RecordSize);
          peano4::grid::packPeriodicDescriptor(descriptor, block.data() + block.size() - RecordSize);
          block.back() = descriptorKind;
        }
      };
    stageDescriptors(producers, 0);
    stageDescriptors(consumers, 1);

    auto [brokeredRecords, brokeredDisplacements] = exchangeBuffers(outgoingBrokerRecords);

    // Group the records of each key and remember which ranks reported it;
    // each of those ranks receives the key's complete record set back.
    struct KeyGroup {
      std::vector<std::array<int, RecordSize>> records;
      std::set<int>                            sourceRanks;
    };
    std::map<peano4::grid::PeriodicKey, KeyGroup> groupsByKey;
    for (int sourceRank = 0; sourceRank < numberOfRanks; sourceRank++) {
      const int beginOffset = brokeredDisplacements[sourceRank];
      const int endOffset   = sourceRank + 1 < numberOfRanks ? brokeredDisplacements[sourceRank + 1]
                                                             : static_cast<int>(brokeredRecords.size());
      for (int recordOffset = beginOffset; recordOffset + RecordSize <= endOffset; recordOffset += RecordSize) {
        const auto descriptor = peano4::grid::unpackPeriodicDescriptor(brokeredRecords.data() + recordOffset);
        auto&      group      = groupsByKey[descriptor.stamp.key];
        auto&      record     = group.records.emplace_back();
        std::copy(
          brokeredRecords.begin() + recordOffset,
          brokeredRecords.begin() + recordOffset + RecordSize,
          record.begin()
        );
        group.sourceRanks.insert(sourceRank);
      }
    }

    std::vector<std::vector<int>> replyRecords(numberOfRanks);
    for (const auto& groupEntry : groupsByKey) {
      const auto& group = groupEntry.second;
      for (const int sourceRank : group.sourceRanks) {
        for (const auto& record : group.records) {
          replyRecords[sourceRank].insert(replyRecords[sourceRank].end(), record.begin(), record.end());
        }
      }
    }

    auto receivedReplies = exchangeBuffers(replyRecords).first;

    producers.clear();
    consumers.clear();
    for (std::size_t recordOffset = 0; recordOffset + RecordSize <= receivedReplies.size();
         recordOffset += RecordSize) {
      auto& descriptorList = receivedReplies[recordOffset + RecordSize - 1] == 0 ? producers : consumers;
      descriptorList.push_back(peano4::grid::unpackPeriodicDescriptor(receivedReplies.data() + recordOffset));
    }
    std::sort(producers.begin(), producers.end(), descriptorLess);
    producers.erase(std::unique(producers.begin(), producers.end(), descriptorEqual), producers.end());
    std::sort(consumers.begin(), consumers.end(), descriptorLess);
    consumers.erase(std::unique(consumers.begin(), consumers.end(), descriptorEqual), consumers.end());
  }

  _periodicRouting = peano4::grid::buildPeriodicRouting(producers, consumers);

  _crossRankPeriodicChannels.clear();
  for (const auto& producer : producers) {
    const auto routingEntry = _periodicRouting.find(producer.stamp.key);
    assertion(routingEntry != _periodicRouting.end(), producer.toString());
    const int producerRank = peano4::parallel::TreeIdMapping::getRank(producer.treeId);
    for (const int targetTreeId :
         routingEntry->second[peano4::grid::toIndex(peano4::grid::opposite(producer.stamp.side))]) {
      if (peano4::parallel::TreeIdMapping::getRank(targetTreeId) != producerRank) {
        _crossRankPeriodicChannels.emplace(producer.treeId, targetTreeId);
      }
    }
  }

  logDebug(
    "Resolved " << producers.size() << " producer / " << consumers.size() << " consumer descriptors into "
                << _periodicRouting.size() << " keys and " << _crossRankPeriodicChannels.size()
                << " cross-rank channels"
  );
}

/**
 * Exchange all boundary data between the trees, after all traversals of a
 * sweep have finished. The routine runs serially and realises four ordered
 * phases per sweep:
 *
 * 1. Resolve the periodic partner associations (rebuildPeriodicAssociations()),
 *    as the routing of every tree depends on them.
 * 2. Per tree, trigger the horizontal sends and receives and purge the
 *    tree's periodic input stacks. Triggering has to precede any wait, since
 *    MPI's rendezvous protocol would otherwise deadlock: a rank must not
 *    block for one tree's data while a peer still waits for it to post
 *    another tree's send.
 * 3. Per tree, route the periodic boundary data. Every purge has to precede
 *    the first routing, because the routing deposits into the partner
 *    trees' input stacks.
 * 4. Wait for all outstanding MPI operations, then run the cross-rank
 *    periodic exchange: once per rank, grid vertices first, then the
 *    user-data containers, in the same order on every rank so the shared
 *    periodic tag stays matched.
 *
 * The data transfer routines modify the underlying stack containers - they
 * create containers and move data around - so the phases must not run
 * concurrently.
 */
/**
 * Collect the remote ranks this rank exchanges boundary data with, and per rank
 * the number of local trees that contribute to it.
 *
 * A tree counts once per remote rank however many of its neighbours live there,
 * because it packs its data into that rank's buffer once. The result drives the
 * contributor countdown, so a tree that turns out to have nothing to send this
 * sweep still has to be counted: it contributes an empty segment and hands on.
 */
void peano4::parallel::SpacetreeSet::rebuildBoundaryAggregationTopology() {
  const int localRank = tarch::mpi::Rank::getInstance().getRank();

  _boundaryAggregationRemoteRanks.clear();
  _boundaryAggregationContributors.clear();

  for (auto& ranksOfOneTree : _boundaryAggregationRanksPerTree) {
    ranksOfOneTree.clear();
  }

  std::vector<int> allContributions;
  for (const auto& tree : _spacetrees) {
    auto& ranksOfThisTree = _boundaryAggregationRanksPerTree[peano4::parallel::TreeIdMapping::getLocalTreeId(tree._id)];
    for (int slot = 0; slot < tree.getNumberOfNeighbourTrees(); slot++) {
      const int neighbourRank = peano4::parallel::TreeIdMapping::getRank(tree.getNeighbourTreeIdForSlot(slot));
      if (neighbourRank != localRank) {
        ranksOfThisTree.push_back(neighbourRank);
      }
    }
    std::sort(ranksOfThisTree.begin(), ranksOfThisTree.end());
    ranksOfThisTree.erase(std::unique(ranksOfThisTree.begin(), ranksOfThisTree.end()), ranksOfThisTree.end());
    allContributions.insert(allContributions.end(), ranksOfThisTree.begin(), ranksOfThisTree.end());
  }

  std::sort(allContributions.begin(), allContributions.end());
  for (const int remoteRank : allContributions) {
    if (_boundaryAggregationRemoteRanks.empty() or _boundaryAggregationRemoteRanks.back() != remoteRank) {
      _boundaryAggregationRemoteRanks.push_back(remoteRank);
      _boundaryAggregationContributors.push_back(0);
    }
    _boundaryAggregationContributors.back()++;
  }
}

void peano4::parallel::SpacetreeSet::exchangeHorizontalDataBetweenTrees(peano4::grid::GridTraversalObserver& observer) {
  rebuildPeriodicAssociations();
  rebuildBoundaryAggregationTopology();

  for (auto& tree : _spacetrees) {
    exchangeAllHorizontalDataExchangeStacks(*this, tree._id);
    purgePeriodicBoundaryDataStacks(*this, tree._id);
    createObserverCloneIfRequired(observer, tree._id);
    _clonedObserver[tree._id]->exchangeAllHorizontalDataExchangeStacks();
    _clonedObserver[tree._id]->purgePeriodicBoundaryDataStacks();
  }

  for (auto& tree : _spacetrees) {
    exchangeAllPeriodicBoundaryDataStacks(*this, tree._id);
    _clonedObserver[tree._id]->exchangeAllPeriodicBoundaryDataStacks();
  }

  for (auto& tree : _spacetrees) {
    finishAllOutstandingSendsAndReceives(*this, tree._id);
    createObserverCloneIfRequired(observer, tree._id);
    _clonedObserver[tree._id]->finishAllOutstandingSendsAndReceives();
  }

  exchangeCrossRankPeriodicData(*this);
  observer.exchangeCrossRankPeriodicData();
}

/**
 * Snapshot masters that already merge workers before traversal. Traversal can
 * trigger further joins, so callers use this snapshot to distinguish those
 * requests from joins being completed in the current sweep.
 */
std::set<int> peano4::parallel::SpacetreeSet::getLocalTreesMergingWithWorkers() const {
  std::set<int> result;
  for (const auto& tree : _spacetrees) {
    if (not tree._joining.empty()) {
      result.insert(tree._id);
    }
  }
  return result;
}

/**
 * Instantiate workers after local data exchange and tree cleanup.
 *
 * A worker inherits its master's bounding box and traversal inversion, and a
 * remote worker gets them through the record its rank picks up at the start of
 * the next sweep. Collecting the masters before the first instantiation keeps
 * the loop off the tree list while that list grows.
 *
 * @see exchangeTreeManagementRecords()
 */
void peano4::parallel::SpacetreeSet::createNewTrees() {
  std::vector<TreeManagementRecord> newTrees;
  for (const auto& master : _spacetrees) {
    for (const int workerTreeId : master._splitting) {
      auto& record                 = newTrees.emplace_back();
      record.kind                  = TreeManagementRecord::Kind::CreateTree;
      record.masterTreeId          = master._id;
      record.workerTreeId          = workerTreeId;
      record.rootOffset            = master._root.x;
      record.rootWidth             = master._root.h;
      record.rootTraversalInverted = master._root.getInverted() ? 1 : 0;
    }
  }

  const int localRank = tarch::mpi::Rank::getInstance().getRank();
  for (const auto& record : newTrees) {
    if (peano4::parallel::TreeIdMapping::getRank(record.workerTreeId) == localRank) {
      createTree(record);
    } else {
      _pendingRemoteRecords.push_back(record);
    }
  }
}

void peano4::parallel::SpacetreeSet::deleteClonedObservers() {
  for (auto& observerEntry : _clonedObserver) {
    if (observerEntry.second != nullptr) {
      delete observerEntry.second;
      observerEntry.second = nullptr;
    }
  }
}

/**
 * Traverse local trees in dependency order.
 *
 * The tree-management exchange settles every rank's pending splits, worker
 * creations and removals before task lists and communicators are created, and
 * it is collective, so it also keeps the ranks in step. Existing and joining
 * trees run first, new workers perform their empty traversal next, and
 * dependent joins finish last. Data exchange, cleanup, and worker creation run
 * only after all traversal tasks complete.
 */
void peano4::parallel::SpacetreeSet::traverse(peano4::grid::GridTraversalObserver& observer) {
  // Names this sweep's aggregated boundary buffers. Bumping it here, before any
  // data moves, marks every container's buffers as belonging to an earlier
  // sweep, so the phases that run ahead of the boundary exchange do not mistake
  // them for their own.
  _boundaryAggregationGeneration++;

  exchangeTreeManagementRecords();

  if (_communicatorRebuildRequired) {
    peano4::parallel::ExchangeChannels::getInstance().createPendingCommunicators();
  }

  std::vector<tarch::multicore::Task*> primaryTasks;
  std::vector<tarch::multicore::Task*> secondaryTasks;
  std::vector<tarch::multicore::Task*> tertiaryTasks;

  for (auto& tree : _spacetrees) {
    createObserverCloneIfRequired(observer, tree._id);
    createObserverCloneIfRequired(observer, tree._masterId);
    switch (tree._spacetreeState) {
    case peano4::grid::SpacetreeState::NewRoot:
    case peano4::grid::SpacetreeState::JoinTriggered:
    case peano4::grid::SpacetreeState::Joining:
      primaryTasks.push_back(new TraverseTask(tree, *_clonedObserver[tree._id], false));
      break;
    case peano4::grid::SpacetreeState::Running:
      (tree._joining.empty() ? primaryTasks : tertiaryTasks)
        .push_back(new TraverseTask(tree, *_clonedObserver[tree._id], false));
      break;
    case peano4::grid::SpacetreeState::EmptyRun:
      secondaryTasks.push_back(new TraverseTask(tree, *_clonedObserver[tree._id], true));
      tertiaryTasks.push_back(new TraverseTask(tree, *_clonedObserver[tree._id], false));
      break;
    default:
      break;
    }
  }

  observer.setLocalTrees();

  tarch::multicore::spawnTasks(primaryTasks);
  streamDataFromSplittingTreesToNewTrees(observer);
  tarch::multicore::spawnTasks(secondaryTasks);
  tarch::multicore::spawnTasks(tertiaryTasks);
  exchangeHorizontalDataBetweenTrees(observer);

  cleanUpTrees();
  createNewTrees();
  deleteClonedObservers();
}

/**
 * Remove joined and empty trees after traversal. Joining workers run before
 * their masters so vertical buffers contain the data when the master consumes
 * them; the traversal phases therefore keep a worker and its master out of the
 * same parallel task group.
 */
void peano4::parallel::SpacetreeSet::cleanUpTrees() {
  for (auto treeIterator = _spacetrees.begin(); treeIterator != _spacetrees.end();) {
    if (treeIterator->_spacetreeState == peano4::grid::SpacetreeState::Joined) {
      logInfo("Tree " << treeIterator->_id << " has successfully joined");
      const int treeId = treeIterator->_id;
      unregisterLocalSpacetree(treeId);
      peano4::parallel::TreeIdMapping::getInstance().deregisterId(treeIterator->_id);
      treeIterator = _spacetrees.erase(treeIterator);
      continue;
    } else if (treeIterator->mayJoinWithMaster()
               and treeIterator->getGridStatistics().numberOfLocalUnrefinedCells == 0) {
      logInfo("Remove empty tree " << treeIterator->_id << " with master " << treeIterator->_masterId);
      const int treeId = treeIterator->_id;
      unregisterLocalSpacetree(treeId);
      peano4::parallel::TreeIdMapping::getInstance().deregisterId(treeIterator->_id);

      if (peano4::parallel::TreeIdMapping::getRank(treeIterator->_masterId)
          != tarch::mpi::Rank::getInstance().getRank()) {
        logDebug("Parent tree " << treeIterator->_masterId << " is not local on this rank. Remove child reference");
        auto& record        = _pendingRemoteRecords.emplace_back();
        record.kind         = TreeManagementRecord::Kind::RemoveEmptyChild;
        record.masterTreeId = treeIterator->_masterId;
        record.workerTreeId = treeIterator->_id;
      } else {
        logDebug("Parent tree " << treeIterator->_masterId << " is local on this rank");
        getSpacetree(treeIterator->_masterId)._statistics.setRemovedEmptySubtree(true);
      }
      treeIterator = _spacetrees.erase(treeIterator);
      continue;
    } else if (treeIterator->mayJoinWithMaster()) {
      const int localRank  = peano4::parallel::TreeIdMapping::getRank(treeIterator->_id);
      const int masterRank = peano4::parallel::TreeIdMapping::getRank(treeIterator->_masterId);
      if (localRank == masterRank
          and getSpacetree(treeIterator->_masterId).getGridStatistics().getCoarseningHasBeenVetoed()) {
        logError(
          "Join tree " << treeIterator->_id << " as it is deteriorated (encodes no hierarchical data) while master "
                       << treeIterator->_masterId << " resides on same rank and can't coarsen"
        );
        logError("Not implemented yet");
      } else if (localRank == masterRank) {
        logDebug(
          "Tree " << treeIterator->_id
                  << " is deteriorated (encodes no hierarchical data) yet seems not to constrain its master"
        );
      } else {
        logDebug(
          "Tree " << treeIterator->_id << " should join to reduce synchronisation: " << treeIterator->toString()
        );
        logDebug("Not implemented yet");
      }
    }
    ++treeIterator;
  }
}

const peano4::grid::GridStatistics& peano4::parallel::SpacetreeSet::getGridStatistics(int treeId) const {
  assertion(isLocalSpacetree(treeId), treeId, tarch::mpi::Rank::getInstance().getRank());
  return getSpacetree(treeId).getGridStatistics();
}

peano4::grid::GridStatistics peano4::parallel::SpacetreeSet::getGridStatistics() const {
  grid::GridStatistics result;
  bool                 hasStatistics = false;

  for (const auto& tree : _spacetrees) {
    if (not hasStatistics) {
      result        = tree._statistics;
      hasStatistics = true;
    } else {
      result += tree._statistics;
    }
  }

  return hasStatistics ? result : grid::GridStatistics{};
}

bool peano4::parallel::SpacetreeSet::isLocalSpacetree(int treeId) const {
  const int localTreeId = peano4::parallel::TreeIdMapping::getLocalTreeId(treeId);
  if (localTreeId < 0 or localTreeId >= MAX_SPACETREES_PER_RANK) {
    return false;
  }
  const auto* const tree = _spacetreeByLocalTreeId[localTreeId];
  return tree != nullptr and tree->_id == treeId;
}

/**
 * @return Set of local tree IDs.
 */
std::set<int> peano4::parallel::SpacetreeSet::getLocalSpacetrees() const {
  std::set<int> result;
  for (const auto& tree : _spacetrees) {
    result.insert(tree._id);
  }
  return result;
}

/** Report whether a local tree can register another split. */
bool peano4::parallel::SpacetreeSet::maySplit(const int treeId) const { return getSpacetree(treeId).maySplit(); }

/**
 * Register an exact split on a local tree.
 *
 * Peano rejects the request and returns false if maySplit() on the underlying
 * tree returns false.
 *
 * @param treeId Tree that gives cells to the worker.
 * @param instruction Split mode and exact number of current leaf cells to transfer.
 * @param targetRank Rank that hosts the worker.
 *
 * @return Whether the exact split request was registered successfully. Once
 *   registered, failure to assign the requested cells terminates the run.
 */
bool peano4::parallel::SpacetreeSet::split(
  const int                     treeId,
  const grid::SplitInstruction& instruction,
  const int                     targetRank
) {
  return split(treeId, std::vector<SplitRequest>{{instruction, targetRank}});
}

/**
 * Register a batch of exact splits.
 *
 * The worker IDs are handed out by the ranks that will own them, so the
 * requests wait here until the next tree-management exchange. They keep their
 * order, which is the order in which the source tree receives its
 * instructions, so a batched split stays deterministic. The next traversal
 * starts with that exchange and therefore sees every request registered here.
 *
 * @see exchangeTreeManagementRecords()
 */
bool peano4::parallel::SpacetreeSet::split(const int treeId, const std::vector<SplitRequest>& requests) {
  if (requests.empty()) {
    return true;
  }

  if (not getSpacetree(treeId).maySplit()) {
    return false;
  }

  const int numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  for (const auto& request : requests) {
    if (request.targetRank < 0 or request.targetRank >= numberOfRanks) {
      tarch::terminateWithError("Cannot split a tree onto an invalid target rank.");
    }
    _pendingSplits.push_back({request.instruction, treeId, request.targetRank});
  }

  return true;
}

peano4::grid::Spacetree& peano4::parallel::SpacetreeSet::getSpacetree(int treeId) {
  const int localTreeId = peano4::parallel::TreeIdMapping::getLocalTreeId(treeId);
  assertion(
    localTreeId >= 0 and localTreeId < MAX_SPACETREES_PER_RANK,
    "Invalid local tree id.",
    treeId,
    localTreeId,
    tarch::mpi::Rank::getInstance().getRank()
  );
  auto* const tree = _spacetreeByLocalTreeId[localTreeId];
  assertion(tree != nullptr, "No spacetree found.", treeId, localTreeId, tarch::mpi::Rank::getInstance().getRank());
  assertion(tree->_id == treeId, "Spacetree lookup mismatch.", treeId, tree->_id, localTreeId);
  return *tree;
}

const peano4::grid::Spacetree& peano4::parallel::SpacetreeSet::getSpacetree(int treeId) const {
  const int localTreeId = peano4::parallel::TreeIdMapping::getLocalTreeId(treeId);
  assertion(
    localTreeId >= 0 and localTreeId < MAX_SPACETREES_PER_RANK,
    "Invalid local tree id.",
    treeId,
    localTreeId,
    tarch::mpi::Rank::getInstance().getRank()
  );
  const auto* const tree = _spacetreeByLocalTreeId[localTreeId];
  assertion(tree != nullptr, "No spacetree found.", treeId, localTreeId, tarch::mpi::Rank::getInstance().getRank());
  assertion(tree->_id == treeId, "Spacetree lookup mismatch.", treeId, tree->_id, localTreeId);
  return *tree;
}

std::vector<peano4::parallel::SpacetreeSet::TreeExtent> peano4::parallel::SpacetreeSet::getLocalTreeExtents() const {
  std::vector<TreeExtent> result;
  result.reserve(_spacetrees.size());
  for (const auto& tree : _spacetrees) {
    result.push_back(TreeExtent{tree._root.x, tree._root.h});
  }
  return result;
}
