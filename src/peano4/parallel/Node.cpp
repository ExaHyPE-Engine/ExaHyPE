// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Node.h"

#include "peano4/grid/AutomatonState.h"
#include "peano4/grid/SpacetreeHelper.h"
#include "peano4/grid/StackTopology.h"
#include "peano4/parallel/StartTraversalMessage.h"
#include "peano4/parallel/TreeManagementMessage.h"
#include "tarch/Assertions.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/mpi.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/Task.h"

#include <chrono> // std::chrono::seconds
#include <thread> // std::this_thread::sleep_for
#include <unordered_set>

peano4::parallel::Node peano4::parallel::Node::_singleton;

namespace {
  constexpr int TagsPerReceivingTree = 3;

  int getExchangeModeTagOffset(peano4::parallel::Node::ExchangeMode exchange) {
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
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    registerId(0, -1);
  }
  for (int i = 0; i < MAX_SPACETREES_PER_RANK; ++i) {
    _dataExchangeCommunicators[i] = MPI_COMM_NULL;
    _needCommunicator[i]          = false;
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

int peano4::parallel::Node::reserveId(int rank, int forTreeId) {
  logDebug("Trying to find and reserve new global id for master id " << forTreeId << " on rank " << rank);

  int localTreeId = 0;
  int result      = -1;
  while (result == -1) {
    if (_treeEntries.count(getId(rank, localTreeId)) == 0) {
      result = getId(rank, localTreeId);
    } else {
      logDebug(
        "Local tree " << localTreeId << " (global id=" << getId(rank, localTreeId) << ") on rank " << rank
                      << " is already in use"
      );
      localTreeId++;
    }

    if (localTreeId >= MAX_SPACETREES_PER_RANK) {
      logWarning("Exceeded limit for maximal available spacetrees per rank (" << MAX_SPACETREES_PER_RANK << ")");
      result = -1;
      break;
    }
  }

  if (result != -1) {
    registerId(result, forTreeId);
  } else {
    logError("Could not reserve a new tree id. No free slot found or limit reached. Abort.");
    std::abort();
  }

  return result;
}

void peano4::parallel::Node::registerId(int id, int masterId) {
  logDebug("Register new global id " << id << " for master id " << masterId);
  tarch::multicore::Lock lock(_semaphore);
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

void peano4::parallel::Node::createPendingCommunicators() {
#ifdef PEANO_WITH_MPI
  const int        n = MAX_SPACETREES_PER_RANK;
  std::vector<int> local(n), global(n);
  for (int i = 0; i < n; ++i) {
    local[i] = (_needCommunicator[i] and _dataExchangeCommunicators[i] == MPI_COMM_NULL) ? 1 : 0;
  }

  tarch::mpi::allreduce(
    local.data(),
    global.data(),
    n,
    MPI_INT,
    MPI_MAX,
    tarch::mpi::Rank::getInstance().getCommunicator()
  );

  for (int i = 0; i < n; ++i) {
    if (global[i] and _dataExchangeCommunicators[i] == MPI_COMM_NULL) {
      MPI_Comm_dup(tarch::mpi::Rank::getInstance().getCommunicator(), &_dataExchangeCommunicators[i]);
    }
    // Only clear the request for slots in this round's snapshot. A slot freshly set by a pumped tree
    // registration during the allreduce (local[i]==0 here) keeps its flag and is honoured next round.
    if (local[i]) {
      _needCommunicator[i] = false;
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

  assertion(localSendingTreeId >= 0);
  assertion(
    localSendingTreeId < MAX_SPACETREES_PER_RANK,
    sendingTreeId,
    MAX_SPACETREES_PER_RANK,
    getRank(sendingTreeId),
    localSendingTreeId,
    getNumberOfRegisteredTrees()
  );

  const int tag = TagsPerReceivingTree * localReceivingTreeId + getExchangeModeTagOffset(exchange);

  return peano4::parallel::Node::GridDataExchangeMetaInformation(tag, _dataExchangeCommunicators[localSendingTreeId]);
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
