// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::database::tests {
  class GlobalTimeSeriesDatabaseTest;
} // namespace tarch::database::tests

class tarch::database::tests::GlobalTimeSeriesDatabaseTest final : public tarch::tests::TestCase {
private:
  void testAppendsChronologicallyWithoutDuplication();
  void testClearRewritesFileFromScratch();
  void testTimeDeltaThrottlesSnapshots();

public:
  GlobalTimeSeriesDatabaseTest();
  ~GlobalTimeSeriesDatabaseTest() override = default;

  void run() override;
};
