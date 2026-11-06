// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/mpi/AbstractCommunicationManager.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/SerialCommunicationManager.h"
#include "tarch/mpi/ThreadedCommunicationManager.h"
#include "tarch/testing/UnitTest.h"

UNIT_TEST("One progress call of the serial manager runs every poller once") {
  GIVEN("A serial manager with two pollers that count their invocations") {
    tarch::mpi::SerialCommunicationManager manager;
    int                                    firstPollerInvocations{0};
    int                                    secondPollerInvocations{0};
    manager.registerPoller([&firstPollerInvocations]() -> bool {
      firstPollerInvocations++;
      return true;
    });
    manager.registerPoller([&secondPollerInvocations]() -> bool {
      secondPollerInvocations++;
      return true;
    });

    WHEN("The manager progresses twice") {
      manager.progress();
      manager.progress();

      THEN("Each poller counts two invocations") {
        VALIDATE_EQUALS(firstPollerInvocations, 2);
        VALIDATE_EQUALS(secondPollerInvocations, 2);
      }
    }
  }
}

UNIT_TEST("The serial manager finishes the pending work by polling until every poller reports idle") {
  GIVEN("A serial manager with a poller that reports idle from its fourth invocation on") {
    constexpr int                          PassesUntilIdle = 4;
    tarch::mpi::SerialCommunicationManager manager;
    int                                    pollerInvocations{0};
    manager.registerPoller([&pollerInvocations]() -> bool {
      pollerInvocations++;
      return pollerInvocations >= PassesUntilIdle;
    });

    WHEN("The manager finishes the pending work") {
      manager.finishPendingWork();

      THEN("The poller counts four invocations") { VALIDATE_EQUALS(pollerInvocations, PassesUntilIdle); }
    }
  }
}

UNIT_TEST("The thread of the threaded manager runs the pollers without a progress call") {
  GIVEN("A threaded manager with a poller that counts its invocations") {
    tarch::mpi::ThreadedCommunicationManager manager;
    std::atomic<int>                         pollerInvocations{0};
    manager.registerPoller([&pollerInvocations]() -> bool {
      pollerInvocations.fetch_add(1, std::memory_order_relaxed);
      return true;
    });

    WHEN("The manager starts, the caller waits for three invocations and the manager stops") {
      manager.start();
      while (pollerInvocations.load(std::memory_order_relaxed) < 3) {
      }
      manager.stop();

      THEN("The poller counts at least three invocations") {
        VALIDATE(pollerInvocations.load() >= 3, pollerInvocations.load());
      }
    }
  }
}

UNIT_TEST("The wait of the threaded manager returns only after the poller reports idle") {
  GIVEN("A threaded manager with a poller that reports idle from its third pass on") {
    constexpr int                            PassesUntilIdle = 3;
    tarch::mpi::ThreadedCommunicationManager manager;
    std::atomic<int>                         passes{0};
    manager.registerPoller([&passes]() -> bool {
      return passes.fetch_add(1, std::memory_order_relaxed) + 1 >= PassesUntilIdle;
    });

    WHEN("The manager starts, finishes the pending work and stops") {
      manager.start();
      manager.finishPendingWork();
      const int passesAfterWait = passes.load(std::memory_order_relaxed);
      manager.stop();

      THEN("The poller ran at least three passes when the wait returned") {
        VALIDATE(passesAfterWait >= PassesUntilIdle, passesAfterWait);
      }
    }
  }
}

UNIT_TEST("Without a running thread, the wait of the threaded manager runs the pollers on the calling thread") {
  GIVEN("A threaded manager that never starts and a poller that reports idle from its second invocation on") {
    constexpr int                            PassesUntilIdle = 2;
    tarch::mpi::ThreadedCommunicationManager manager;
    std::atomic<int>                         pollerInvocations{0};
    manager.registerPoller([&pollerInvocations]() -> bool {
      return pollerInvocations.fetch_add(1, std::memory_order_relaxed) + 1 >= PassesUntilIdle;
    });

    WHEN("The manager finishes the pending work") {
      manager.finishPendingWork();

      THEN("The poller counts two invocations") { VALIDATE_EQUALS(pollerInvocations.load(), PassesUntilIdle); }
    }
  }
}

UNIT_TEST("The dedicated communication thread runs if a caller enables it and the process has a peer") {
  GIVEN("The configuration on entry and whether the process has a peer") {
    const bool usedOnEntry = tarch::mpi::AbstractCommunicationManager::usesDedicatedThread();
    const bool hasPeers    = tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1;

    WHEN("A caller disables the thread, enables it and configures the value on entry again") {
      tarch::mpi::AbstractCommunicationManager::configure(false);
      const bool usedWhenDisabled = tarch::mpi::AbstractCommunicationManager::usesDedicatedThread();
      tarch::mpi::AbstractCommunicationManager::configure(true);
      const bool usedWhenEnabled = tarch::mpi::AbstractCommunicationManager::usesDedicatedThread();
      tarch::mpi::AbstractCommunicationManager::configure(usedOnEntry);
      const bool usedWhenConfiguredAgain = tarch::mpi::AbstractCommunicationManager::usesDedicatedThread();

      THEN("The thread runs only when enabled with a peer and the value on entry restores the configuration") {
        VALIDATE(not usedWhenDisabled);
        VALIDATE_EQUALS(usedWhenEnabled, hasPeers);
        VALIDATE_EQUALS(usedWhenConfiguredAgain, usedOnEntry);
      }
    }
  }
}
