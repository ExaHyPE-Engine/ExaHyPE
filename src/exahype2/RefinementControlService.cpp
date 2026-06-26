// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RefinementControlService.h"

#include "exahype2/RefinementControl.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/mpi.h"
#include "tarch/multicore/RecursiveLock.h"
#include "tarch/services/ServiceRepository.h"

int exahype2::RefinementControlService::_reductionTag =
  tarch::mpi::Rank::reserveFreeTag("exahype2::RefinementControlService");

tarch::multicore::RecursiveSemaphore exahype2::RefinementControlService::_semaphore;

exahype2::RefinementControlService& exahype2::RefinementControlService::getInstance() {
  static exahype2::RefinementControlService result;
  return result;
}

exahype2::RefinementControlService::RefinementControlService() {
  tarch::services::ServiceRepository::getInstance().addService(this, "exahype2::RefinementControlService");

#ifdef PEANO_WITH_MPI
  _recvSizeRequest = MPI_REQUEST_NULL;
  _isShuttingDown  = false;
  postReceiveSize();
#endif
}

exahype2::RefinementControlService::~RefinementControlService() {
#ifdef PEANO_WITH_MPI
  int mpiFinalised = 0;
  MPI_Finalized(&mpiFinalised);
  if (not mpiFinalised) {
    shutdown();
  }
#endif
  tarch::services::ServiceRepository::getInstance().removeService(this);
}

void exahype2::RefinementControlService::shutdown() {
#ifdef PEANO_WITH_MPI
  tarch::multicore::RecursiveLock lock(_semaphore);

  if (_isShuttingDown) {
    return;
  }

  freeAllPendingSendRequests();

  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator(), [&]() -> void {
    tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
  });

  // Finish every outstanding receive so nothing is left in the queue at finalize, then stop re-posting
  // and cancel the persistent size receive.
  drainDanglingMessages();
  _isShuttingDown = true;
  cancelReceiveSizeRequest();
#endif
}

#ifdef PEANO_WITH_MPI
void exahype2::RefinementControlService::postReceiveSize() {
  if (not _isShuttingDown and _recvSizeRequest == MPI_REQUEST_NULL) {
    tarch::mpi::irecv(
      &_recvSizeBuffer,
      1,
      MPI_INT,
      MPI_ANY_SOURCE,
      _reductionTag,
      tarch::mpi::Rank::getInstance().getCommunicator(),
      &_recvSizeRequest
    );
  }
}

void exahype2::RefinementControlService::tryToReceiveDanglingMessage() {
  // Complete any finished body receives. Each in-flight body is an independent, source-specific irecv,
  // so a slow/large body from one rank never blocks receiving from another - no head-of-line blocking.
  for (auto body = _incomingBodies.begin(); body != _incomingBodies.end();) {
    int flag = 0;
    tarch::mpi::test(&body->request, &flag, MPI_STATUS_IGNORE);
    if (flag) {
      for (const auto& event : body->buffer) {
        _remoteNewEvents.push_back(event);
        _committedEvents.push_back(event);
      }
      body = _incomingBodies.erase(body);
    } else {
      ++body;
    }
  }

  // Drain every pending size header on the persistent ANY_SOURCE receive.
  // For each header post a source-specific body receive and then re-post
  // the size receive immediately, so the size channel never blocks behind a
  // body. The body irecv is posted before the size re-post, so MPI matches the incoming body to it
  // rather than to the (later-posted) size receive.
  postReceiveSize();
  int        flag = 0;
  MPI_Status status;
  tarch::mpi::test(&_recvSizeRequest, &flag, &status);
  while (flag) {
    _recvSizeRequest     = MPI_REQUEST_NULL;
    const int eventCount = _recvSizeBuffer;
    if (eventCount > 0) {
      _incomingBodies.emplace_back();
      IncomingBody& body = _incomingBodies.back();
      body.buffer.resize(eventCount);
      tarch::mpi::irecv(
        body.buffer.data(),
        static_cast<int>(eventCount * sizeof(peano4::grid::GridControlEvent)),
        MPI_BYTE,
        status.MPI_SOURCE,
        _reductionTag,
        tarch::mpi::Rank::getInstance().getCommunicator(),
        &body.request
      );
    }
    postReceiveSize();
    flag = 0;
    tarch::mpi::test(&_recvSizeRequest, &flag, &status);
  }
}

void exahype2::RefinementControlService::drainDanglingMessages() {
  // Shutdown helper: receive everything still pending, then block-complete all in-flight bodies so no
  // request crosses MPI_Finalize.
  tryToReceiveDanglingMessage();
  for (auto& body : _incomingBodies) {
    tarch::mpi::wait(&body.request, []() {}, MPI_STATUS_IGNORE);
    for (const auto& event : body.buffer) {
      _remoteNewEvents.push_back(event);
      _committedEvents.push_back(event);
    }
  }
  _incomingBodies.clear();
}

void exahype2::RefinementControlService::cancelReceiveSizeRequest() {
  if (_recvSizeRequest != MPI_REQUEST_NULL) {
    MPI_Status status;
    tarch::mpi::cancel(&_recvSizeRequest);
    tarch::mpi::wait(&_recvSizeRequest, []() {}, &status);

    int wasCancelled = 0;
    MPI_Test_cancelled(&status, &wasCancelled);
    _recvSizeRequest = MPI_REQUEST_NULL;

    if (not wasCancelled and _recvSizeBuffer > 0) {
      // A header arrived just before the cancel; receive and process its body so the sender's matching
      // body send completes and nothing is left in the queue.
      std::vector<peano4::grid::GridControlEvent> body(_recvSizeBuffer);
      MPI_Request                                 bodyRequest = MPI_REQUEST_NULL;
      tarch::mpi::irecv(
        body.data(),
        static_cast<int>(_recvSizeBuffer * sizeof(peano4::grid::GridControlEvent)),
        MPI_BYTE,
        status.MPI_SOURCE,
        _reductionTag,
        tarch::mpi::Rank::getInstance().getCommunicator(),
        &bodyRequest
      );
      tarch::mpi::wait(&bodyRequest, []() {}, MPI_STATUS_IGNORE);
      for (const auto& event : body) {
        _remoteNewEvents.push_back(event);
        _committedEvents.push_back(event);
      }
    }
  }
}
#endif

void exahype2::RefinementControlService::receiveDanglingMessages() {
#ifdef PEANO_WITH_MPI
  tarch::multicore::RecursiveLock lock(_semaphore);
  tryToReceiveDanglingMessage();
#endif
}

std::string exahype2::RefinementControlService::toString() const {
  std::ostringstream msg;
  msg << "("
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
    logDebug(
      "Activate " << _committedEvents.size()
                  << " refinement/erase instructions (can be taken into account in next grid sweep). Keep "
                  << _localNewEvents.size() << " local event(s) to be re-delivered later (max lifetime " << maxLifetime
                  << ") besides the " << _remoteNewEvents.size()
                  << " remote events which will also be re-delivered next"
    );
  }

#ifdef PEANO_WITH_MPI
  freeAllPendingSendRequests();
  _copyOfCommittedEvents = _committedEvents;
  triggerSendOfCopyOfCommittedEvents();
#endif

  for (const auto& p : _remoteNewEvents) {
    _committedEvents.push_back(p);
  }
  _remoteNewEvents.clear();

  peano4::grid::GridControlEvent::merge(_committedEvents);
}

#ifdef PEANO_WITH_MPI
void exahype2::RefinementControlService::freeAllPendingSendRequests() {
  for (MPI_Request* request : _sendRequests) {
    if (request != nullptr) {
      tarch::mpi::wait(request, [&]() -> void {
        tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
      }, MPI_STATUS_IGNORE);
      delete request;
    }
  }
  _sendRequests.clear();
}

void exahype2::RefinementControlService::triggerSendOfCopyOfCommittedEvents() {
  assertion(_sendRequests.empty());

  logDebug("Share my " << _copyOfCommittedEvents.size() << " event(s) with others");
  if (_copyOfCommittedEvents.size() > 0) {
    // Per peer: a small size header (so the receiver can size its body irecv without an ANY_SOURCE
    // probe) followed by the body. The receiver re-posts the size receive immediately and runs body
    // receives concurrently, so this does not head-of-line-block at scale.
    const int ranks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
    _sendSizeBuffers.resize(ranks);

    for (int rank = 0; rank < ranks; rank++) {
      if (rank != tarch::mpi::Rank::getInstance().getRank()) {
        _sendSizeBuffers[rank] = static_cast<int>(_copyOfCommittedEvents.size());
        MPI_Request* sizeReq   = new MPI_Request();
        tarch::mpi::isend(
          &_sendSizeBuffers[rank],
          1,
          MPI_INT,
          rank,
          _reductionTag,
          tarch::mpi::Rank::getInstance().getCommunicator(),
          sizeReq
        );
        _sendRequests.push_back(sizeReq);

        MPI_Request* bodyReq = new MPI_Request();
        tarch::mpi::isend(
          _copyOfCommittedEvents.data(),
          static_cast<int>(_copyOfCommittedEvents.size() * sizeof(peano4::grid::GridControlEvent)),
          MPI_BYTE,
          rank,
          _reductionTag,
          tarch::mpi::Rank::getInstance().getCommunicator(),
          bodyReq
        );
        _sendRequests.push_back(bodyReq);
      }
    }
  }
}
#endif

std::vector<peano4::grid::GridControlEvent> exahype2::RefinementControlService::getGridControlEvents() const {
  return _committedEvents;
}
