// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SpawnTaskTest.h"

#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/Task.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/taskfusion/taskfusion.h"

int numberOfTasksThatHaveBeenExecuted = 0;
tarch::multicore::BooleanSemaphore numberOfTasksThatHaveBeenExecutedSemaphore;

class NonFusableTask : public tarch::multicore::Task {
public:
  NonFusableTask(int priority = tarch::multicore::Task::DefaultPriority)
    : Task(Task::DontFuse, priority) {}

  ~NonFusableTask() override = default;

  bool run() override {
    tarch::multicore::Lock lock(numberOfTasksThatHaveBeenExecutedSemaphore);
    ++numberOfTasksThatHaveBeenExecuted;
    return false;
  }
};

class FusableTask : public tarch::multicore::Task {
public:
  FusableTask(int priority = tarch::multicore::Task::DefaultPriority)
    : Task(0, priority) {}

  ~FusableTask() override = default;

  bool fuse(const std::list<Task*>& otherTasks, int) override {
    const int numberOfTasksToBeFused = otherTasks.size() + 1;
    numberOfTasksThatHaveBeenExecuted = numberOfTasksToBeFused;
    return false;
  }

  bool run() override {
    // Do nothing
    return false;
  }
};

tarch::multicore::tests::SpawnTaskTest::SpawnTaskTest():
  TestCase("tarch::multicore::tests::SpawnTaskTest") {}

void tarch::multicore::tests::SpawnTaskTest::run() {
  testMethod([this]() { testSpawnNonFusableTasksWithoutDependencies(4, 1000); });
  testMethod([this]() { testSpawnFusableTasksWithoutDependencies(4, 1000); });
}

void tarch::multicore::tests::SpawnTaskTest::testSpawnNonFusableTasksWithoutDependencies(
  int taskLoopIterations,
  int numberOfTasksToSpawn
) {
  #if defined(SharedOMP)
  #pragma omp parallel
  #pragma omp master
  #pragma omp taskloop
  #endif
  for (int t = 0; t < taskLoopIterations; t++) {
    for (int i = 0; i < numberOfTasksToSpawn; i++) {
      NonFusableTask* nonFusableTask = new NonFusableTask();
      spawnTask(nonFusableTask);
    }
  }

  waitForAllTasks();
  validateEquals(numberOfTasksThatHaveBeenExecuted, taskLoopIterations * numberOfTasksToSpawn);

  numberOfTasksThatHaveBeenExecuted = 0;
  validateEquals(numberOfTasksThatHaveBeenExecuted, 0);
}

void tarch::multicore::tests::SpawnTaskTest::testSpawnFusableTasksWithoutDependencies(
  int taskLoopIterations,
  int numberOfTasksToSpawn
) {
  #if defined(SharedOMP)
  #pragma omp parallel
  #pragma omp master
  #pragma omp taskloop
  #endif
  for (int t = 0; t < taskLoopIterations; t++) {
    for (int i = 0; i < numberOfTasksToSpawn; i++) {
      FusableTask* fusableTask = new FusableTask();
      spawnTask(fusableTask);
    }
  }

  taskfusion::processPendingTasksThatCanBeFused();
  validateEquals(numberOfTasksThatHaveBeenExecuted, taskLoopIterations * numberOfTasksToSpawn);

  numberOfTasksThatHaveBeenExecuted = 0;
  validateEquals(numberOfTasksThatHaveBeenExecuted, 0);
}
