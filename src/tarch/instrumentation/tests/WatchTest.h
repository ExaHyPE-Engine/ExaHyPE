// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::instrumentation::tests {
  class WatchTest;
} // namespace tarch::instrumentation::tests

class tarch::instrumentation::tests::WatchTest final : public tarch::tests::TestCase {
public:
  WatchTest();
  WatchTest(const WatchTest&)            = delete;
  WatchTest(WatchTest&&)                 = delete;
  WatchTest& operator=(const WatchTest&) = delete;
  WatchTest& operator=(WatchTest&&)      = delete;
  ~WatchTest() override                  = default;

  void run() override;

private:
  void testAStoppedWatchReportsTheTimeSinceItsStart();
  void testAWatchWithoutStartReportsZero();
};
