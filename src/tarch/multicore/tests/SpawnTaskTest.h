// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::multicore::tests {
  class SpawnTaskTest;
}

class tarch::multicore::tests::SpawnTaskTest final: public tarch::tests::TestCase {
private:
  void testSpawnNonFusableTasksWithoutDependencies(
    int taskLoopIterations,
    int numberOfTasksToSpawn
  );
  void testSpawnFusableTasksWithoutDependencies(
    int taskLoopIterations,
    int numberOfTasksToSpawn
  );

public:
  SpawnTaskTest();
  ~SpawnTaskTest() override final = default;

  /**
   * This routine is triggered by the TestCaseCollection
   */
  void run() override final;
};
