// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::database::tests {
  class GlobalTimeSeriesDatabaseTest;
} // namespace tarch::database::tests

class tarch::database::tests::GlobalTimeSeriesDatabaseTest final : public tarch::tests::TestCase {
public:
  GlobalTimeSeriesDatabaseTest();
  GlobalTimeSeriesDatabaseTest(const GlobalTimeSeriesDatabaseTest& other)            = delete;
  GlobalTimeSeriesDatabaseTest(GlobalTimeSeriesDatabaseTest&& other)                 = delete;
  GlobalTimeSeriesDatabaseTest& operator=(const GlobalTimeSeriesDatabaseTest& other) = delete;
  GlobalTimeSeriesDatabaseTest& operator=(GlobalTimeSeriesDatabaseTest&& other)      = delete;
  ~GlobalTimeSeriesDatabaseTest() override                                           = default;

  void run() override;

private:
  void testAppendsChronologicallyWithoutDuplication();
  void testClearRewritesFileFromScratch();
  void testARepeatedTimeStampOverwritesTheRowInTheFile();
  void testTimeDeltaThrottlesSnapshots();
  void testASnapshotIsDueForTheNewestTimeStampAndAfterTheTimeDelta();
  void testEveryLaterTimestampIsDueBeforeTheTimeDelta();
  void testADatabaseWithoutFileNameWritesNoFile();
};
