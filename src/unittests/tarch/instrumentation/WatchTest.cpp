// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/instrumentation/Watch.h"

#include "tarch/testing/UnitTest.h"

UNIT_TEST("The calendar time of a stopped watch covers every sleep between its start and its stop") {
  GIVEN("A watch that starts on construction and a sleep time") {
    constexpr double              SleepTime = 0.02;
    tarch::instrumentation::Watch watch("tarch/instrumentation/WatchTest", "sleep", false);

    WHEN("The watch stops after one sleep and stops again after a second sleep") {
      std::this_thread::sleep_for(std::chrono::duration<double>(SleepTime));
      watch.stop();
      const double firstCalendarTime = watch.getCalendarTime();
      const double firstCPUTime      = watch.getCPUTime();
      std::this_thread::sleep_for(std::chrono::duration<double>(SleepTime));
      watch.stop();
      const double secondCalendarTime = watch.getCalendarTime();

      THEN("The first calendar time covers one sleep, the second covers both and the CPU time is not negative") {
        VALIDATE(firstCalendarTime >= SleepTime, firstCalendarTime);
        VALIDATE(secondCalendarTime >= firstCalendarTime + SleepTime, firstCalendarTime, secondCalendarTime);
        VALIDATE(firstCPUTime >= 0.0, firstCPUTime);
      }
    }
  }
}

UNIT_TEST("A watch that does not start on construction reports zero times until start() and stop()") {
  GIVEN("A watch that does not start on construction") {
    tarch::instrumentation::Watch watch("tarch/instrumentation/WatchTest", "idle", false, false);

    THEN("The calendar time and the CPU time are zero") {
      VALIDATE_EQUALS(watch.getCalendarTime(), 0.0);
      VALIDATE_EQUALS(watch.getCPUTime(), 0.0);
    }

    WHEN("The watch starts and stops") {
      watch.start();
      watch.stop();

      THEN("The calendar time is not negative") { VALIDATE(watch.getCalendarTime() >= 0.0); }
    }
  }
}
