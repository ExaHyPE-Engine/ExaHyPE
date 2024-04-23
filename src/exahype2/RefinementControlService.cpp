#include "RefinementControlService.h"

#include "RefinementControl.h"
#include "peano4/grid/grid.h"
#include "tarch/multicore/RecursiveLock.h"
#include "tarch/services/ServiceRepository.h"

tarch::logging::Log exahype2::RefinementControlService::_log("exahype2::RefinementControlService");

int exahype2::RefinementControlService::_reductionTag(tarch::mpi::Rank::reserveFreeTag("exahype2::"
                                                                                       "RefinementControlService"));

tarch::multicore::RecursiveSemaphore exahype2::RefinementControlService::_semaphore;

exahype2::RefinementControlService& exahype2::RefinementControlService::getInstance() {
  static exahype2::RefinementControlService result;
  return result;
}

exahype2::RefinementControlService::RefinementControlService() {
  tarch::services::ServiceRepository::getInstance().addService(this, "exahype2::RefinementControlService");
}

exahype2::RefinementControlService::~RefinementControlService() {
  tarch::services::ServiceRepository::getInstance().removeService(this);
}

void exahype2::RefinementControlService::shutdown() {}

void exahype2::RefinementControlService::receiveDanglingMessages() {
#ifdef Parallel
  int guardFlag;
  MPI_Iprobe(
    MPI_ANY_SOURCE, _reductionTag, tarch::mpi::Rank::getInstance().getCommunicator(), &guardFlag, MPI_STATUS_IGNORE
  );

  if (guardFlag) {
    tarch::multicore::RecursiveLock lock(_semaphore);

    MPI_Status status;
    int        flag;
    MPI_Iprobe(MPI_ANY_SOURCE, _reductionTag, tarch::mpi::Rank::getInstance().getCommunicator(), &flag, &status);
    if (flag) {
      const int rank = status.MPI_SOURCE;
      int       numberOfMessages;
      MPI_Get_count(&status, peano4::grid::GridControlEvent::getGlobalCommunciationDatatype(), &numberOfMessages);

      logDebug("receiveDanglingMessages()", "got " << numberOfMessages << " event(s) from rank " << rank);

      std::vector<peano4::grid::GridControlEvent> receiveBuffer;
      receiveBuffer.resize(numberOfMessages);
      MPI_Recv(
        receiveBuffer.data(),
        numberOfMessages,
        peano4::grid::GridControlEvent::getGlobalCommunciationDatatype(),
        rank,
        _reductionTag,
        tarch::mpi::Rank::getInstance().getCommunicator(),
        MPI_STATUS_IGNORE
      );

      for (auto p : receiveBuffer) {
        _remoteNewEvents.push_back(p);
        _committedEvents.push_back(p);
      }

      logDebug(
        "receiveDanglingMessages()",
        "now hold "
          << _remoteNewEvents.size() << " new remote events and " << _committedEvents.size()
          << " committed events (valid for this traversal)"
      );
    }
  }
#endif
}

std::string exahype2::RefinementControlService::toString() const {
  std::ostringstream msg;
  msg
    << "("
    << "#new-local-events=" << _localNewEvents.size() << "#new-remote-events=" << _remoteNewEvents.size()
    << ",#committed-events=" << _committedEvents.size() << ")";
  return msg.str();
}

void exahype2::RefinementControlService::merge(const RefinementControl& control) {
  tarch::multicore::RecursiveLock lock(_semaphore);
  _localNewEvents.insert(_localNewEvents.end(), control._newEvents.begin(), control._newEvents.end());
}

void exahype2::RefinementControlService::finishStep() {
  tarch::multicore::RecursiveLock lock(_semaphore);

  _committedEvents.clear();

  int                                    maxLifetime = 0;
  RefinementControl::NewEvents::iterator p           = _localNewEvents.begin();
  while (p != _localNewEvents.end()) {
    _committedEvents.push_back(p->first);
    p->second--;

    if (p->second <= 0) {
      p = _localNewEvents.erase(p);
    } else {
      maxLifetime = std::max(maxLifetime, p->second);
      p++;
    }
  }

  if (not _committedEvents.empty()) {
    logInfo(
      "finishStep()",
      "activate "
        << _committedEvents.size()
        << " refinement/erase instructions (can be taken into account in next grid sweep). Keep "
        << _localNewEvents.size() << " local event(s) to be re-delivered later (max lifetime " << maxLifetime
        << ") besides the " << _remoteNewEvents.size() << " remote events which will also be re-delivered next"
    );
  }

#ifdef Parallel
  freeAllPendingSendRequests();
  _copyOfCommittedEvents = _committedEvents;
  triggerSendOfCopyOfCommittedEvents();
#endif

  for (auto p : _remoteNewEvents) {
    _committedEvents.push_back(p);
  }
  _remoteNewEvents.clear();

  _committedEvents = ::peano4::grid::merge(_committedEvents);
}

#ifdef Parallel
void exahype2::RefinementControlService::freeAllPendingSendRequests() {
  while (not _sendRequests.empty()) {
    std::vector<MPI_Request*>::iterator p = _sendRequests.begin();
    while (p != _sendRequests.end()) {
      if (*p == nullptr) {
        p = _sendRequests.erase(p);
      } else {
        int flag;
        MPI_Test(*p, &flag, MPI_STATUS_IGNORE);
        if (flag) {
          *p = nullptr;
        }
        p++;
      }
    }
    tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
  }
}

void exahype2::RefinementControlService::triggerSendOfCopyOfCommittedEvents() {
  _sendRequests.clear();

  logInfo(
    "triggerSendOfCopyOfCommittedEvents()", "share my " << _copyOfCommittedEvents.size() << " event(s) with others"
  );
  if (_copyOfCommittedEvents.size() > 0) {
    _sendRequests = std::vector<MPI_Request*>(tarch::mpi::Rank::getInstance().getNumberOfRanks(), nullptr);
    for (int rank = 0; rank < tarch::mpi::Rank::getInstance().getNumberOfRanks(); rank++) {
      if (rank != tarch::mpi::Rank::getInstance().getRank()) {
        _sendRequests[rank] = new MPI_Request;
        MPI_Isend(
          _copyOfCommittedEvents.data(),
          _copyOfCommittedEvents.size(),
          peano4::grid::GridControlEvent::getGlobalCommunciationDatatype(),
          rank,
          _reductionTag,
          tarch::mpi::Rank::getInstance().getCommunicator(),
          _sendRequests[rank]
        );
      }
    }
  }
}
#endif

std::vector<peano4::grid::GridControlEvent> exahype2::RefinementControlService::getGridControlEvents() const {
  return _committedEvents;
}
