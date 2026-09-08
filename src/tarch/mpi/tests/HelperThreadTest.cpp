// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HelperThreadTest.h"

#include "tarch/mpi/HelperThread.h"

tarch::mpi::tests::HelperThreadTest::HelperThreadTest() :
  tarch::tests::TestCase("tarch::mpi::tests::HelperThreadTest") {}

void tarch::mpi::tests::HelperThreadTest::run() {
  testMethod(testPollsUntilTheFunctionReportsCompletion);
  testMethod(testStopEndsAPollThatNeverCompletes);
  testMethod(testRestartPollsAgain);
  testMethod(testStopWithoutStartIsHarmless);
}

/** A function that reports completion ends the loop, and the thread stops on its own. */
void tarch::mpi::tests::HelperThreadTest::testPollsUntilTheFunctionReportsCompletion() {
  constexpr int            PollsUntilComplete = 5;
  std::atomic<int>         numberOfPolls{0};
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

/** A function that never completes keeps polling until stop() takes the thread down. */
void tarch::mpi::tests::HelperThreadTest::testStopEndsAPollThatNeverCompletes() {
  std::atomic<int>         numberOfPolls{0};
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

/** A stopped thread polls again after a restart. */
void tarch::mpi::tests::HelperThreadTest::testRestartPollsAgain() {
  std::atomic<int>         numberOfPolls{0};
  tarch::mpi::HelperThread helperThread([&numberOfPolls]() -> bool {
    numberOfPolls.fetch_add(1, std::memory_order_relaxed);
    return true;
  });

  helperThread.start();
  while (not helperThread.hasFinished()) {
  }
  helperThread.stop();
  const int pollsAfterFirstRun = numberOfPolls.load();

  helperThread.restart();
  while (not helperThread.hasFinished()) {
  }
  helperThread.stop();

  validate(helperThread.hasFinished());
  validateEqualsWithParams1(pollsAfterFirstRun, 1, pollsAfterFirstRun);
  validateEqualsWithParams1(numberOfPolls.load(), 2, numberOfPolls.load());
}

/** Stopping a thread that never ran leaves it alone, so the destructor can stop unconditionally. */
void tarch::mpi::tests::HelperThreadTest::testStopWithoutStartIsHarmless() {
  tarch::mpi::HelperThread helperThread([]() -> bool { return true; });

  helperThread.stop();
  helperThread.stop();

  validate(helperThread.hasFinished());
}
