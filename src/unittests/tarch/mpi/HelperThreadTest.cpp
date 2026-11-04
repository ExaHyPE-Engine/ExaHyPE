// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/mpi/HelperThread.h"

#include "tarch/testing/UnitTest.h"

UNIT_TEST("A polling function that returns true ends the loop of the helper thread") {
  GIVEN("A helper thread whose polling function reports completion at its fifth poll") {
    constexpr int    PollsUntilComplete = 5;
    std::atomic<int> numberOfPolls{0};
    tarch::mpi::HelperThread helperThread([&numberOfPolls]() -> bool {
      return numberOfPolls.fetch_add(1, std::memory_order_relaxed) + 1 >= PollsUntilComplete;
    });

    WHEN("The thread starts, the caller waits until the thread finishes and stops it") {
      helperThread.start();
      while (not helperThread.hasFinished()) {
      }
      helperThread.stop();

      THEN("The thread finished after five polls") {
        VALIDATE(helperThread.hasFinished());
        VALIDATE_EQUALS(numberOfPolls.load(), PollsUntilComplete);
      }
    }
  }
}

UNIT_TEST("A stop ends the loop of a polling function that never returns true") {
  GIVEN("A helper thread whose polling function counts its polls and never reports completion") {
    std::atomic<int> numberOfPolls{0};
    tarch::mpi::HelperThread helperThread([&numberOfPolls]() -> bool {
      numberOfPolls.fetch_add(1, std::memory_order_relaxed);
      return false;
    });

    WHEN("The thread starts, the caller waits for the first poll and stops the thread") {
      helperThread.start();
      while (numberOfPolls.load(std::memory_order_relaxed) < 1) {
      }
      helperThread.stop();

      THEN("The polling function ran at least once and never reported completion") {
        VALIDATE(not helperThread.hasFinished());
        VALIDATE(numberOfPolls.load() >= 1, numberOfPolls.load());
      }
    }
  }
}

UNIT_TEST("A stop without a start leaves the thread finished") {
  GIVEN("A helper thread whose polling function reports completion") {
    tarch::mpi::HelperThread helperThread([]() -> bool { return true; });

    WHEN("The thread stops twice without a start") {
      helperThread.stop();
      helperThread.stop();

      THEN("The thread still counts as finished") { VALIDATE(helperThread.hasFinished()); }
    }
  }
}
