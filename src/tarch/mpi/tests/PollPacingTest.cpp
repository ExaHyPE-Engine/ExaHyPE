// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PollPacingTest.h"

#include "tarch/mpi/PollPacing.h"

namespace {
  /** Polls the pacing and collects the one-based indices at which it grants a turn. */
  std::vector<int> collectTurns(const int numberOfPolls) {
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
  TestCase("tarch::mpi::tests::PollPacingTest") {}

void tarch::mpi::tests::PollPacingTest::run() {
  testMethod(testFirstPollReportsProgress);
  testMethod(testTurnsWidenWhileTheWaitLasts);
  testMethod(testStrideSaturatesSoReportingContinues);
  testMethod(testEveryWaitStartsTight);
}

/** A wait that ends on its first poll still has to report once, so the first poll grants a turn. */
void tarch::mpi::tests::PollPacingTest::testFirstPollReportsProgress() {
  tarch::mpi::PollPacing pacing;
  validate(pacing.reportProgressNow());
}

/** The gap between two turns doubles, so a long wait spends its polls on the awaited request. */
void tarch::mpi::tests::PollPacingTest::testTurnsWidenWhileTheWaitLasts() {
  const auto             turns = collectTurns(100);
  const std::vector<int> expectedTurns{1, 4, 9, 18, 35, 68};
  validateEqualsWithParams1(turns.size(), expectedTurns.size(), turns.size());
  for (std::size_t turn = 0; turn < expectedTurns.size(); turn++) {
    validateEqualsWithParams2(turns[turn], expectedTurns[turn], turn, turns[turn]);
  }
}

/**
 * The widening is bounded, so a wait of any length keeps checking its timeouts. Over a hundred thousand
 * polls the turns stay far below the poll count while never thinning out to nothing.
 */
void tarch::mpi::tests::PollPacingTest::testStrideSaturatesSoReportingContinues() {
  constexpr int NumberOfPolls = 100000;
  const auto    turns         = collectTurns(NumberOfPolls);

  validateWithParams1(turns.size() > 20, turns.size());
  validateWithParams1(turns.size() < 200, turns.size());

  int largestGap = 0;
  for (std::size_t turn = 1; turn < turns.size(); turn++) {
    largestGap = std::max(largestGap, turns[turn] - turns[turn - 1]);
    validateWithParams1(turns[turn] > turns[turn - 1], turn);
  }
  validateEqualsWithParams1(largestGap, 1025, largestGap);
}

/** A fresh instance per wait keeps a short wait's latency, so the pacing cannot leak between waits. */
void tarch::mpi::tests::PollPacingTest::testEveryWaitStartsTight() {
  const auto firstWait  = collectTurns(20);
  const auto secondWait = collectTurns(20);
  validateEquals(firstWait.size(), secondWait.size());
  for (std::size_t turn = 0; turn < firstWait.size(); turn++) {
    validateEqualsWithParams1(firstWait[turn], secondWait[turn], turn);
  }
}
