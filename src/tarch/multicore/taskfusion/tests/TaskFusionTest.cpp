// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TaskFusionTest.h"

#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Task.h"
#include "tarch/multicore/taskfusion/taskfusion.h"

constexpr int NumberOfTasksToFuse = 1000;

int numberOfTasksThatHaveBeenFused = 0;

class FusableTask: public tarch::multicore::Task {
public:
  FusableTask(int priority = tarch::multicore::Task::DefaultPriority):
    Task(0, priority) {}

  ~FusableTask() override = default;

  bool fuse(const std::list<Task*>& otherTasks) override {
    const int numberOfTasksToBeFused = otherTasks.size() + 1;
    numberOfTasksThatHaveBeenFused   = numberOfTasksToBeFused;
    for (auto& p : otherTasks) {
      delete p;
    }
    return false;
  }

  bool canFuse() const override { return true; }

  bool run() override {
    // Do nothing
    return false;
  }
};

tarch::multicore::taskfusion::tests::TaskFusionTest::TaskFusionTest():
  TestCase("tarch::multicore::taskfusion::tests::TaskFusionTest") {}

void tarch::multicore::taskfusion::tests::TaskFusionTest::run() {
#ifdef SharedOMP
#pragma omp parallel
    {
#pragma omp master
#endif
      {
        testMethod([this]() { testTranslateFusableTaskIntoTaskSequence(); });
      }
#ifdef SharedOMP
    }
#endif
}

void tarch::multicore::taskfusion::tests::TaskFusionTest::testTranslateFusableTaskIntoTaskSequence() {
  numberOfTasksThatHaveBeenFused = 0;
  for (int i = 0; i < NumberOfTasksToFuse; i++) {
    spawnTask(new FusableTask);
  }
  processPendingTasksThatCanBeFused();
  validateEquals(numberOfTasksThatHaveBeenFused, NumberOfTasksToFuse);
}
