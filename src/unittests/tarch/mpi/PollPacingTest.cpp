// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/mpi/PollPacing.h"

#include "tarch/testing/UnitTest.h"

namespace {
  /**
   * Polls the pacing and collects the one-based indices at which it grants a turn.
   */
  std::vector<int> getTurns(const int numberOfPolls) {
    tarch::mpi::PollPacing pacing;
    std::vector<int>       turns;
    for (int poll = 1; poll <= numberOfPolls; poll++) {
      if (pacing.reportProgressNow()) {
        turns.push_back(poll);
      }
    }
    return turns;
  }
} // namespace

UNIT_TEST("The first poll of a pacing grants a turn") {
  GIVEN("A new pacing") {
    tarch::mpi::PollPacing pacing;

    WHEN("The test polls the pacing once") {
      const bool grantsATurn = pacing.reportProgressNow();

      THEN("The poll grants a turn") { VALIDATE(grantsATurn); }
    }
  }
}

UNIT_TEST("The gap between two turns doubles while the wait lasts") {
  GIVEN("One hundred polls and the turns 1, 4, 9, 18, 35 and 68") {
    constexpr int          NumberOfPolls = 100;
    const std::vector<int> expectedTurns{1, 4, 9, 18, 35, 68};

    WHEN("The test polls a new pacing one hundred times") {
      const auto turns = getTurns(NumberOfPolls);

      THEN("The pacing grants exactly these turns") {
        VALIDATE_EQUALS(turns.size(), expectedTurns.size());
        for (std::size_t turn = 0; turn < expectedTurns.size(); turn++) {
          VALIDATE_EQUALS(turns[turn], expectedTurns[turn], turn);
        }
      }
    }
  }
}

UNIT_TEST("The gap between two turns stops growing at one poll more than the largest stride and the turns continue") {
  GIVEN("One hundred thousand polls") {
    constexpr int NumberOfPolls = 100000;

    WHEN("The test polls a new pacing one hundred thousand times and takes the largest gap between two turns") {
      const auto turns      = getTurns(NumberOfPolls);
      int        largestGap = 0;
      for (std::size_t turn = 1; turn < turns.size(); turn++) {
        largestGap = std::max(largestGap, turns[turn] - turns[turn - 1]);
      }

      THEN("More than 20 and fewer than 200 turns ascend and the largest gap is 1025 polls") {
        VALIDATE(turns.size() > 20, turns.size());
        VALIDATE(turns.size() < 200, turns.size());
        for (std::size_t turn = 1; turn < turns.size(); turn++) {
          VALIDATE(turns[turn] > turns[turn - 1], turn);
        }
        VALIDATE_EQUALS(largestGap, 1025);
      }
    }
  }
}

UNIT_TEST("Every new pacing starts with the shortest gap") {
  GIVEN("Twenty polls") {
    constexpr int NumberOfPolls = 20;

    WHEN("The test polls two new pacings twenty times each") {
      const auto firstWait  = getTurns(NumberOfPolls);
      const auto secondWait = getTurns(NumberOfPolls);

      THEN("Both pacings grant the same turns") {
        VALIDATE_EQUALS(firstWait.size(), secondWait.size());
        for (std::size_t turn = 0; turn < firstWait.size(); turn++) {
          VALIDATE_EQUALS(firstWait[turn], secondWait[turn], turn);
        }
      }
    }
  }
}
