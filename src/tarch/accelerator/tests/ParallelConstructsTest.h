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
  ParallelConstructsTest(const ParallelConstructsTest& other)            = delete;
  ParallelConstructsTest(ParallelConstructsTest&& other)                 = delete;
  ParallelConstructsTest& operator=(const ParallelConstructsTest& other) = delete;
  ParallelConstructsTest& operator=(ParallelConstructsTest&& other)      = delete;
  ~ParallelConstructsTest() override                                     = default;

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
  void testEveryThreadOfATeamReceivesTheVotesOfTheTeam();
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
  void testAReductionOverTeamsOfThreeWarpsSumsEveryTeam();
  void testReduceOverTeamCombinesEveryThread();
  void testEveryTeamRunsTheDefaultNumberOfThreads();
  void testATeamLaunchRunsTheRequestedThreads();
  void testATeamLaunchStopsAtTheDefaultNumberOfThreads();
  void testTeamScratchMirrorsWithinTheTeam();
  void testTheDeviceRunsADefaultTeamAtOnce();
  void testTheNumberOfHostTasksFollowsTheTimePerItem();
#if not defined(PEANO_WITH_GPU_BACKEND)
  void testAHostReductionMergesItsBlocksInOrder();
#endif
};
