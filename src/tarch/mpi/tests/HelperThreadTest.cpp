// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HelperThreadTest.h"

#include "tarch/mpi/HelperThread.h"

tarch::mpi::tests::HelperThreadTest::HelperThreadTest() :
  tarch::tests::TestCase("tarch::mpi::tests::HelperThreadTest") {}

void
tarch::mpi::tests::HelperThreadTest::run() {
  testMethod(testPollsUntilTheFunctionReportsCompletion);
  testMethod(testStopEndsAPollThatNeverCompletes);
  testMethod(testStopWithoutStartIsHarmless);
}

/** A polling function that returns true ends the loop of the thread. */
void
tarch::mpi::tests::HelperThreadTest::testPollsUntilTheFunctionReportsCompletion() {
  constexpr int    PollsUntilComplete = 5;
  std::atomic<int> numberOfPolls{0};
  tarch::mpi::HelperThread helperThread([&numberOfPolls]() -> bool {
    return numberOfPolls.fetch_add(1, std::memory_order_relaxed) + 1 >= PollsUntilComplete;
  });

  helperThread.start();
  while (not helperThread.hasFinished()) {
  }
  helperThread.stop();

  validate(helperThread.hasFinished());
  validateEqualsWithParams1(numberOfPolls.load(), PollsUntilComplete, numberOfPolls.load());
}

/** stop() ends the loop of a polling function that never returns true. */
void
tarch::mpi::tests::HelperThreadTest::testStopEndsAPollThatNeverCompletes() {
  std::atomic<int> numberOfPolls{0};
  tarch::mpi::HelperThread helperThread([&numberOfPolls]() -> bool {
    numberOfPolls.fetch_add(1, std::memory_order_relaxed);
    return false;
  });

  helperThread.start();
  while (numberOfPolls.load(std::memory_order_relaxed) < 1) {
  }
  helperThread.stop();

  validate(not helperThread.hasFinished());
  validateWithParams1(numberOfPolls.load() >= 1, numberOfPolls.load());
}

/** stop() without start() has no effect. */
void
tarch::mpi::tests::HelperThreadTest::testStopWithoutStartIsHarmless() {
  tarch::mpi::HelperThread helperThread([]() -> bool { return true; });

  helperThread.stop();
  helperThread.stop();

  validate(helperThread.hasFinished());
}
