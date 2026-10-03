// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::particles::tests {
  class TrajectoryRecordingTest;
} // namespace exahype2::particles::tests

class exahype2::particles::tests::TrajectoryRecordingTest final : public tarch::tests::TestCase {
public:
  TrajectoryRecordingTest();
  TrajectoryRecordingTest(const TrajectoryRecordingTest&)            = delete;
  TrajectoryRecordingTest(TrajectoryRecordingTest&&)                 = delete;
  TrajectoryRecordingTest& operator=(const TrajectoryRecordingTest&) = delete;
  TrajectoryRecordingTest& operator=(TrajectoryRecordingTest&&)      = delete;
  ~TrajectoryRecordingTest() override                                = default;

  void run() override;

private:
  void testEveryParticleOfTheRankBecomesOneRow();
  void testASnapshotDueOnTheRankOfTheParticleIsDueOnEveryRank();
};
