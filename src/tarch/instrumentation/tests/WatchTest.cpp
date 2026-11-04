// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "WatchTest.h"

#include "tarch/instrumentation/Watch.h"

tarch::instrumentation::tests::WatchTest::WatchTest() :
  tarch::tests::TestCase("tarch::instrumentation::tests::WatchTest") {}

void
tarch::instrumentation::tests::WatchTest::run() {
  testMethod(testAStoppedWatchReportsTheTimeSinceItsStart);
  testMethod(testAWatchWithoutStartReportsZero);
}

/**
 * The calendar time of a stopped watch covers a sleep between start and stop.
 */
void
tarch::instrumentation::tests::WatchTest::testAStoppedWatchReportsTheTimeSinceItsStart() {
  constexpr double SleepTime = 0.02;

  tarch::instrumentation::Watch watch("tarch::instrumentation::tests::WatchTest", "sleep", false);
  std::this_thread::sleep_for(std::chrono::duration<double>(SleepTime));
  watch.stop();

  validate(watch.getCalendarTime() >= SleepTime);
  validate(watch.getCPUTime() >= 0.0);

  const double firstCalendarTime = watch.getCalendarTime();
  std::this_thread::sleep_for(std::chrono::duration<double>(SleepTime));
  watch.stop();
  validate(watch.getCalendarTime() >= firstCalendarTime + SleepTime);
}

/**
 * A watch that does not start on construction reports zero times until start() and stop().
 */
void
tarch::instrumentation::tests::WatchTest::testAWatchWithoutStartReportsZero() {
  tarch::instrumentation::Watch watch("tarch::instrumentation::tests::WatchTest", "idle", false, false);
  validateEquals(watch.getCalendarTime(), 0.0);
  validateEquals(watch.getCPUTime(), 0.0);

  watch.start();
  watch.stop();
  validate(watch.getCalendarTime() >= 0.0);
}
