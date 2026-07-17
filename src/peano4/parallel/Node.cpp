// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Node.h"

#include "peano4/grid/AutomatonState.h"
#include "peano4/grid/SpacetreeHelper.h"
#include "peano4/grid/StackTopology.h"
#include "peano4/parallel/StartTraversalMessage.h"
#include "peano4/parallel/TreeManagementMessage.h"
#include "tarch/Assertions.h"
#include "tarch/Termination.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/mpi.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/Task.h"

peano4::parallel::Node peano4::parallel::Node::_singleton;

namespace {
  constexpr int TagsPerReceivingTree = 3;

  int getExchangeModeTagOffset(const peano4::parallel::Node::ExchangeMode exchange) {
    switch (exchange) {
    case peano4::parallel::Node::ExchangeMode::HorizontalData:
      return 0;
    case peano4::parallel::Node::ExchangeMode::VerticalData:
      return 1;
    case peano4::parallel::Node::ExchangeMode::PeriodicData:
      return 2;
    }
    assertion(false);
    return -1;
  }
} // namespace

/**
 * Compute how many sending-tree slots can share one communicator. The tag
 * stores the sender offset within its group, the receiving-tree slot, and the
 * exchange mode. A zero result means that one sender cannot be represented.
 */
int peano4::parallel::Node::getNumberOfSendingTreesPerDataExchangeCommunicator(int tagUpperBound) {
  constexpr std::int64_t TagsPerSendingTree = static_cast<std::int64_t>(TagsPerReceivingTree) * MAX_SPACETREES_PER_RANK;
  if (tagUpperBound < 0) {
    return 0;
  }

  const std::int64_t supportedSendingTrees = (static_cast<std::int64_t>(tagUpperBound) + 1) / TagsPerSendingTree;
  return static_cast<int>(std::min<std::int64_t>(MAX_SPACETREES_PER_RANK, supportedSendingTrees));
}

int peano4::parallel::Node::getDataExchangeCommunicatorIndex(int localSendingTreeId, int sendingTreesPerCommunicator) {
  assertion(localSendingTreeId >= 0 and localSendingTreeId < MAX_SPACETREES_PER_RANK, localSendingTreeId);
  assertion(sendingTreesPerCommunicator > 0, sendingTreesPerCommunicator);
  return localSendingTreeId / sendingTreesPerCommunicator;
}

int peano4::parallel::Node::getDataExchangeTag(
  int                                  localSendingTreeId,
  int                                  localReceivingTreeId,
  peano4::parallel::Node::ExchangeMode exchange,
  int                                  sendingTreesPerCommunicator
) {
  assertion(localSendingTreeId >= 0 and localSendingTreeId < MAX_SPACETREES_PER_RANK, localSendingTreeId);
  assertion(localReceivingTreeId >= 0 and localReceivingTreeId < MAX_SPACETREES_PER_RANK, localReceivingTreeId);
  assertion(sendingTreesPerCommunicator > 0, sendingTreesPerCommunicator);

  const int          localSendingTreeOffset = localSendingTreeId % sendingTreesPerCommunicator;
  const std::int64_t tag                    = TagsPerReceivingTree
      * (static_cast<std::int64_t>(localSendingTreeOffset) * MAX_SPACETREES_PER_RANK + localReceivingTreeId)
    + getExchangeModeTagOffset(exchange);
  assertion(tag <= std::numeric_limits<int>::max(), tag);
  return static_cast<int>(tag);
}

void peano4::parallel::Node::initMPIDatatypes() {
#ifdef PEANO_WITH_MPI
  StartTraversalMessage::initDatatype();
  TreeManagementMessage::initDatatype();

  peano4::grid::AutomatonState::initDatatype();
  peano4::grid::GridVertex::initDatatype();
#endif
}

void peano4::parallel::Node::shutdownMPIDatatypes() {
#ifdef PEANO_WITH_MPI
  StartTraversalMessage::shutdownDatatype();
  TreeManagementMessage::shutdownDatatype();

  peano4::grid::AutomatonState::shutdownDatatype();
  peano4::grid::GridVertex::shutdownDatatype();
#endif
}

void peano4::parallel::Node::init() {
  _currentProgramStep   = UndefProgramStep;
  _rankOrchestrationTag = tarch::mpi::Rank::reserveFreeTag("peano4::parallel::Node - rank orchestration");
  _sendingTreesPerDataExchangeCommunicator = MAX_SPACETREES_PER_RANK;
#ifdef PEANO_WITH_MPI
  int*      tagUpperBound  = nullptr;
  int       attributeFound = 0;
  const int attributeResult =
    MPI_Comm_get_attr(tarch::mpi::Rank::getInstance().getCommunicator(), MPI_TAG_UB, &tagUpperBound, &attributeFound);
  if (attributeResult != MPI_SUCCESS or not attributeFound or tagUpperBound == nullptr) {
    tarch::terminateWithError("Could not determine MPI_TAG_UB for the runner communicator.");
  }

  const int localTagUpperBound   = *tagUpperBound;
  int       minimumTagUpperBound = localTagUpperBound;
  tarch::mpi::allreduce(
    &localTagUpperBound,
    &minimumTagUpperBound,
    1,
    MPI_INT,
    MPI_MIN,
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
  _sendingTreesPerDataExchangeCommunicator =
    peano4::parallel::Node::getNumberOfSendingTreesPerDataExchangeCommunicator(minimumTagUpperBound);
  if (_sendingTreesPerDataExchangeCommunicator <= 0) {
    tarch::terminateWithError(
      "MPI_TAG_UB " + std::to_string(minimumTagUpperBound) + " cannot encode one sending tree and "
      + std::to_string(MAX_SPACETREES_PER_RANK) + " receiving trees."
    );
  }
#endif
  for (int i = 0; i < MAX_SPACETREES_PER_RANK; ++i) {
    _dataExchangeCommunicators[i] = MPI_COMM_NULL;
    _needCommunicator[i]          = false;
  }
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    registerId(0, -1);
  }
}

std::string peano4::parallel::Node::toString(ExchangeMode mode) {
  switch (mode) {
  case ExchangeMode::HorizontalData:
    return "horizontal-data";
  case ExchangeMode::VerticalData:
    return "vertical-data";
  case ExchangeMode::PeriodicData:
    return "periodic-data";
  }
  return "undef";
}

bool peano4::parallel::Node::isGlobalMaster(int treeId) { return treeId == 0; }

peano4::parallel::Node& peano4::parallel::Node::getInstance() { return _singleton; }

int peano4::parallel::Node::reserveId(int rank, int forTreeId) { return reserveIds(rank, forTreeId, 1).front(); }

/**
 * Reserve a deterministic batch of IDs in one pass over the local slots.
 *
 * Holding the node lock across discovery and insertion prevents another split
 * from claiming a slot between both operations. The single scan changes batch
 * allocation from quadratic to linear complexity in the number of local tree
 * slots.
 */
std::vector<int> peano4::parallel::Node::reserveIds(const int rank, const int forTreeId, const int numberOfIds) {
  if (numberOfIds < 1) {
    tarch::terminateWithError("The number of tree IDs to reserve has to be positive.");
  }

  logDebug("Trying to reserve " << numberOfIds << " global IDs for master ID " << forTreeId << " on rank " << rank);

  std::vector<int> result;
  result.reserve(numberOfIds);
  {
    tarch::multicore::Lock lock(_semaphore);
    for (int localTreeId = 0; localTreeId < MAX_SPACETREES_PER_RANK and static_cast<int>(result.size()) < numberOfIds;
         ++localTreeId) {
      const int treeId = getId(rank, localTreeId);
      if (_treeEntries.count(treeId) == 0) {
        registerIdWithoutLock(treeId, forTreeId);
        result.push_back(treeId);
      }
    }
  }

  if (static_cast<int>(result.size()) != numberOfIds) {
    tarch::terminateWithError(
      "Could not reserve " + std::to_string(numberOfIds) + " tree IDs because rank " + std::to_string(rank)
      + " has only " + std::to_string(result.size()) + " free local tree slots."
    );
  }

  return result;
}

void peano4::parallel::Node::registerId(int id, int masterId) {
  tarch::multicore::Lock lock(_semaphore);
  registerIdWithoutLock(id, masterId);
}

/** Register one tree ID while the caller holds _semaphore. */
void peano4::parallel::Node::registerIdWithoutLock(const int id, const int masterId) {
  logDebug("Register new global id " << id << " for master id " << masterId);
  assertion(_treeEntries.count(id) == 0, id, masterId);
  assertion(id != masterId, id, masterId);
#ifndef PEANO_WITH_MPI
  assertion(isGlobalMaster(id) or _treeEntries.count(masterId) == 1);
#endif

  TreeEntry newEntry;

  newEntry.setId(id);
  newEntry.setMaster(masterId);

  _treeEntries.insert(std::pair<int, TreeEntry>(id, newEntry));

  _needCommunicator[getLocalTreeId(id)] = true;
}

/**
 * Create communicator groups in the same order on every rank. Registrations
 * serviced while the allreduce progresses remain pending for the next call.
 */
void peano4::parallel::Node::createPendingCommunicators() {
#ifdef PEANO_WITH_MPI
  constexpr int                      NumberOfTreeSlots = MAX_SPACETREES_PER_RANK;
  std::array<int, NumberOfTreeSlots> localPendingTreeSlots{};
  std::array<int, NumberOfTreeSlots> globalPendingTreeSlots{};
  for (int localTreeId = 0; localTreeId < NumberOfTreeSlots; ++localTreeId) {
    localPendingTreeSlots[localTreeId] = _needCommunicator[localTreeId] ? 1 : 0;
  }

  tarch::mpi::allreduce(
    localPendingTreeSlots.data(),
    globalPendingTreeSlots.data(),
    NumberOfTreeSlots,
    MPI_INT,
    MPI_MAX,
    tarch::mpi::Rank::getInstance().getCommunicator()
  );

  for (int localTreeId = 0; localTreeId < NumberOfTreeSlots; ++localTreeId) {
    if (globalPendingTreeSlots[localTreeId]) {
      const int communicatorIndex =
        peano4::parallel::Node::getDataExchangeCommunicatorIndex(localTreeId, _sendingTreesPerDataExchangeCommunicator);
      if (_dataExchangeCommunicators[communicatorIndex] == MPI_COMM_NULL) {
        const int duplicateResult = MPI_Comm_dup(
          tarch::mpi::Rank::getInstance().getCommunicator(),
          &_dataExchangeCommunicators[communicatorIndex]
        );
        if (duplicateResult != MPI_SUCCESS) {
          tarch::terminateWithError("Could not create an MPI communicator for tree data exchange.");
        }
      }
    }
    if (localPendingTreeSlots[localTreeId]) {
      _needCommunicator[localTreeId] = false;
    }
  }
#endif
}

int peano4::parallel::Node::getNumberOfRegisteredTrees() const { return _treeEntries.size(); }

void peano4::parallel::Node::deregisterId(int id) {
  assertion(_treeEntries.count(id) == 1, id);

  tarch::multicore::Lock lock(_semaphore);
  _treeEntries.erase(id);

  logDebug("Removed tree " << id << " from list of trees");
}

bool peano4::parallel::Node::continueToRun() {
#ifdef PEANO_WITH_MPI
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    for (int i = 1; i < tarch::mpi::Rank::getInstance().getNumberOfRanks(); i++) {
      StartTraversalMessage message;
      message.setStepIdentifier(_currentProgramStep);
      logDebug("Send out " << message.toString() << " to rank " << i);
      StartTraversalMessage::sendAndPollDanglingMessages(message, i, _rankOrchestrationTag);
    }
  } else {
    StartTraversalMessage message;
    StartTraversalMessage::receiveAndPollDanglingMessages(
      message,
      tarch::mpi::Rank::getGlobalMasterRank(),
      _rankOrchestrationTag
    );
    logDebug("Received message " << message.toString());
    _currentProgramStep = message.getStepIdentifier();
  }
#endif

  return _currentProgramStep != Terminate;
}

void peano4::parallel::Node::setNextProgramStep(int number) {
  assertion(number == Terminate or number >= 0, number);
  _currentProgramStep = number;
}

int peano4::parallel::Node::getCurrentProgramStep() const { return _currentProgramStep; }

void peano4::parallel::Node::shutdown() {
#ifdef PEANO_WITH_MPI
  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator(), [&]() -> void {
    tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
  });
#endif

#ifdef PEANO_WITH_MPI
  for (int i = 0; i < MAX_SPACETREES_PER_RANK; i++) {
    if (_dataExchangeCommunicators[i] != MPI_COMM_NULL) {
      MPI_Comm_free(&(_dataExchangeCommunicators[i]));
      _dataExchangeCommunicators[i] = MPI_COMM_NULL;
      _needCommunicator[i]          = false;
    }
  }
#endif

#ifdef PEANO_WITH_MPI
  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
#endif
}

peano4::parallel::Node::GridDataExchangeMetaInformation peano4::parallel::Node::getGridDataExchangeMetaInformation(
  int          sendingTreeId,
  int          receivingTreeId,
  ExchangeMode exchange
) const {
  const int localSendingTreeId   = getLocalTreeId(sendingTreeId);
  const int localReceivingTreeId = getLocalTreeId(receivingTreeId);

  assertion(
    localSendingTreeId >= 0 and localSendingTreeId < MAX_SPACETREES_PER_RANK,
    sendingTreeId,
    MAX_SPACETREES_PER_RANK,
    getRank(sendingTreeId),
    localSendingTreeId,
    getNumberOfRegisteredTrees()
  );
  assertion(
    localReceivingTreeId >= 0 and localReceivingTreeId < MAX_SPACETREES_PER_RANK,
    receivingTreeId,
    MAX_SPACETREES_PER_RANK,
    getRank(receivingTreeId),
    localReceivingTreeId,
    getNumberOfRegisteredTrees()
  );

  const int communicatorIndex = peano4::parallel::Node::getDataExchangeCommunicatorIndex(
    localSendingTreeId,
    _sendingTreesPerDataExchangeCommunicator
  );
  const int tag = peano4::parallel::Node::getDataExchangeTag(
    localSendingTreeId,
    localReceivingTreeId,
    exchange,
    _sendingTreesPerDataExchangeCommunicator
  );

  return peano4::parallel::Node::GridDataExchangeMetaInformation(tag, _dataExchangeCommunicators[communicatorIndex]);
}

std::string peano4::parallel::Node::getSemanticsForTag(int tag) {
  if (tag == getInstance()._rankOrchestrationTag) {
    return "rank orchestration tag";
  }

  assertion(tag >= 0);

  std::string result = "data exchange tag for ";

  switch (tag % TagsPerReceivingTree) {
  case 0:
    result += "horizontal data";
    break;
  case 1:
    result += "vertical data";
    break;
  case 2:
    result += "periodic data";
    break;
  }

  return result;
}
