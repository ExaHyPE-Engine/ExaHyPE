// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PollPacingTest.h"

#include "tarch/mpi/PollPacing.h"

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

tarch::mpi::tests::PollPacingTest::PollPacingTest() :
  tarch::tests::TestCase("tarch::mpi::tests::PollPacingTest") {}

void
tarch::mpi::tests::PollPacingTest::run() {
  testMethod(testFirstPollReportsProgress);
  testMethod(testTurnsWidenWhileTheWaitLasts);
  testMethod(testStrideSaturatesSoReportingContinues);
  testMethod(testEveryWaitStartsTight);
}

void
tarch::mpi::tests::PollPacingTest::testFirstPollReportsProgress() {
  tarch::mpi::PollPacing pacing;
  validate(pacing.reportProgressNow());
}

/**
 * The number of polls between two turns doubles.
 */
void
tarch::mpi::tests::PollPacingTest::testTurnsWidenWhileTheWaitLasts() {
  const auto             turns = getTurns(100);
  const std::vector<int> expectedTurns{1, 4, 9, 18, 35, 68};
  validateEqualsWithParams1(turns.size(), expectedTurns.size(), turns.size());
  for (std::size_t turn = 0; turn < expectedTurns.size(); turn++) {
    validateEqualsWithParams2(turns[turn], expectedTurns[turn], turn, turns[turn]);
  }
}

/**
 * The gap between two turns stops growing at one poll more than the largest stride.
 */
void
tarch::mpi::tests::PollPacingTest::testStrideSaturatesSoReportingContinues() {
  constexpr int NumberOfPolls = 100000;
  const auto    turns         = getTurns(NumberOfPolls);

  validateWithParams1(turns.size() > 20, turns.size());
  validateWithParams1(turns.size() < 200, turns.size());

  int largestGap = 0;
  for (std::size_t turn = 1; turn < turns.size(); turn++) {
    largestGap = std::max(largestGap, turns[turn] - turns[turn - 1]);
    validateWithParams1(turns[turn] > turns[turn - 1], turn);
  }
  validateEqualsWithParams1(largestGap, 1025, largestGap);
}

/**
 * Every new instance starts with the shortest gap.
 */
void
tarch::mpi::tests::PollPacingTest::testEveryWaitStartsTight() {
  const auto firstWait  = getTurns(20);
  const auto secondWait = getTurns(20);
  validateEquals(firstWait.size(), secondWait.size());
  for (std::size_t turn = 0; turn < firstWait.size(); turn++) {
    validateEqualsWithParams1(firstWait[turn], secondWait[turn], turn);
  }
}
