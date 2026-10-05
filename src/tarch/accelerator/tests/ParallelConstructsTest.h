// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::accelerator::tests {
  class ParallelConstructsTest;
} // namespace tarch::accelerator::tests

class tarch::accelerator::tests::ParallelConstructsTest final : public tarch::tests::TestCase {
public:
  ParallelConstructsTest();
  ParallelConstructsTest(const ParallelConstructsTest&)            = delete;
  ParallelConstructsTest(ParallelConstructsTest&&)                 = delete;
  ParallelConstructsTest& operator=(const ParallelConstructsTest&) = delete;
  ParallelConstructsTest& operator=(ParallelConstructsTest&&)      = delete;
  ~ParallelConstructsTest() override                               = default;

  void run() override;

private:
  void testParallelForOnHost();
  void testParallelFor();
  void testParallelForRangeMD2();
  void testParallelForRangeMD3();
  void testParallelForTeam();
  void testParallelForTeamRangeMD2();
  void testParallelForTeamRangeMD3();
  void testTeamBarrier();
  void testParallelReduceSum();
  void testParallelReduceMin();
  void testParallelReduceMax();
  void testParallelReduceRangeMD2();
  void testParallelReduceRangeMD3();
  void testParallelReduceTeam();
  void testParallelReduceTeamSum();
  void testParallelReduceTeamRangeMD2();
  void testParallelReduceTeamRangeMD3();
  void testParallelReduceCustom();
  void testParallelReduceCustomRange();
  void testReductionScratchKeepsAnAddressThatFits();
  void testWaitForDeviceCompletesTheLaunchedLoop();
  void testAReductionOverManyTeamsSumsEveryTeam();
  void testReduceOverTeamCombinesEveryThread();
  void testEveryTeamRunsTheDefaultNumberOfThreads();
  void testATeamLaunchRunsTheRequestedThreads();
  void testTeamScratchMirrorsWithinTheTeam();
  void testTheDeviceRunsADefaultTeamAtOnce();
};
