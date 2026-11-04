// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::particles::tests {
  class TrajectoryDatabaseTest;
} // namespace exahype2::particles::tests

class exahype2::particles::tests::TrajectoryDatabaseTest final : public tarch::tests::TestCase {
public:
  TrajectoryDatabaseTest();
  TrajectoryDatabaseTest(const TrajectoryDatabaseTest& other)            = delete;
  TrajectoryDatabaseTest(TrajectoryDatabaseTest&& other)                 = delete;
  TrajectoryDatabaseTest& operator=(const TrajectoryDatabaseTest& other) = delete;
  TrajectoryDatabaseTest& operator=(TrajectoryDatabaseTest&& other)      = delete;
  ~TrajectoryDatabaseTest() override                                     = default;

  void run() override;

private:
  void testAppendsChronologicallyWithoutDuplication();
  void testClearRewritesFileFromScratch();
  void testASnapshotIsDueForANewParticleAfterTheDeltasAndOnAMove();
  void testEveryLaterTimestampIsDueForAParticleAtRest();
};
