// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/NonCriticalAssertions.h"

#include "tarch/instrumentation/Log.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/mpi.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/Lock.h"
#include "tarch/services/Service.h"
#include "tarch/services/ServiceRepository.h"

namespace {
  std::atomic<int>                   rankWhichHasSetNonCriticalAssertion{-1};
  tarch::multicore::BooleanSemaphore assertionSemaphore;
  bool                               useNonCriticalAssertions = true;

#ifdef PEANO_WITH_MPI
  // Event-driven cross-rank propagation of a non-critical assertion.
  //
  // A non-critical assertion is a one-off, single-rank event: one rank discovers a problem, sets
  // its local flag, the run dumps its state and winds down. The other ranks only need to learn that
  // it happened. They must not learn it through a per-sweep collective: the previous design ran an
  // MPI_Allreduce of one int after every grid sweep, which acted as an implicit barrier and
  // perturbed the deliberately asynchronous, one-sweep-lagged load-balancing reduction, regressing
  // the decomposition. Instead the originating rank announces itself once over the regular
  // dangling-message channel, and every other rank picks it up while it polls that channel anyway -
  // no extra collective, no barrier, nothing on the hot path unless an assertion actually fires.
  class NonCriticalAssertionService: public tarch::services::Service {
  private:
    int                      _tag           = -1;
    bool                     _haveAnnounced = false;
    int                      _announcedRank = -1; // stable send buffer; written once before the sends
    std::vector<MPI_Request> _sendRequests;

  public:
    static NonCriticalAssertionService& getInstance() {
      static NonCriticalAssertionService singleton;
      return singleton;
    }

    void init() {
      _tag = tarch::mpi::Rank::reserveFreeTag("tarch::NonCriticalAssertions");
      tarch::services::ServiceRepository::getInstance().addService(this, "tarch::NonCriticalAssertions");
    }

    void receiveDanglingMessages() override {
      auto& rank = tarch::mpi::Rank::getInstance();
      if (_tag < 0 or rank.getNumberOfRanks() <= 1) {
        return;
      }

      // Pick up announcements from other ranks and adopt the first asserting rank we hear about.
      while (rank.isMessageInQueue(_tag)) {
        int remoteRank = -1;
        tarch::mpi::receive(&remoteRank, 1, MPI_INT, MPI_ANY_SOURCE, _tag, rank.getCommunicator());
        int expected = -1;
        rankWhichHasSetNonCriticalAssertion.compare_exchange_strong(expected, remoteRank);
      }

      // Announce our own assertion exactly once. Only the rank whose own id sits in the flag (the
      // originator) announces, so the notification storm is bounded to a single round.
      if (not _haveAnnounced and rankWhichHasSetNonCriticalAssertion.load() == rank.getRank()) {
        _haveAnnounced = true;
        _announcedRank = rank.getRank();
        for (int targetRank = 0; targetRank < rank.getNumberOfRanks(); targetRank++) {
          if (targetRank != rank.getRank()) {
            _sendRequests.emplace_back(MPI_REQUEST_NULL);
            tarch::mpi::isend(&_announcedRank, 1, MPI_INT, targetRank, _tag, rank.getCommunicator(), &_sendRequests.back());
          }
        }
      }
    }

  protected:
    // Deterministic shutdown: leave no announcement crossing MPI_Finalize. Every rank reaches the
    // same barrier, pumping the dangling-message channel so all announcements are delivered, then
    // each rank completes its own sends and drains whatever is still queued on its tag.
    void shutdown() override {
      auto& rank = tarch::mpi::Rank::getInstance();
      if (_tag < 0 or rank.getNumberOfRanks() <= 1) {
        return;
      }

      // Make sure our own announcement, if any, is in flight before the barrier.
      receiveDanglingMessages();
      tarch::mpi::barrier(rank.getCommunicator(), []() {
        tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
      });

      for (MPI_Request& request : _sendRequests) {
        if (request != MPI_REQUEST_NULL) {
          tarch::mpi::wait(&request);
        }
      }
      _sendRequests.clear();

      while (rank.isMessageInQueue(_tag)) {
        int remoteRank = -1;
        tarch::mpi::receive(&remoteRank, 1, MPI_INT, MPI_ANY_SOURCE, _tag, rank.getCommunicator());
      }
    }
  };
#endif
} // namespace

void tarch::initNonCriticalAssertionEnvironment() {
#ifdef PEANO_WITH_MPI
  NonCriticalAssertionService::getInstance().init();
#endif
}

void tarch::enableNonCriticalAssertions(bool value) { useNonCriticalAssertions = value; }

void tarch::triggerNonCriticalAssertion(
  const std::string& file,
  int                line,
  const std::string& expression,
  const std::string& parameterValuePairs
) {
  tarch::multicore::Lock lock(assertionSemaphore);

  const int assertionRank = rankWhichHasSetNonCriticalAssertion.load();
  if (assertionRank >= 0 and useNonCriticalAssertions) {
    logDebug("Noncritical assertion " << expression << " failed in (" << file << ":" << line << ")");
    logDebug(
      "There has been a noncritical assertion before, so node should already be in "
      "shutdown mode"
    );
  }
  if (assertionRank < 0 and useNonCriticalAssertions) {
    logError("Noncritical assertion " << expression << " failed in (" << file << ":" << line << ")");
    logError(parameterValuePairs);
    logError("Mark application to dump solution and shut down");

    rankWhichHasSetNonCriticalAssertion.store(tarch::mpi::Rank::getInstance().getRank());
  }
}

bool tarch::hasNonCriticalAssertionBeenViolated() { return rankWhichHasSetNonCriticalAssertion.load() >= 0; }
