// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "EnclaveBookkeepingTest.h"

#include "exahype2/EnclaveBookkeeping.h"

#include "tarch/multicore/Task.h"

constexpr int NumberOfTasksToExecute = 1000;

int numberOfTasksThatHaveBeenExecuted = 0;

class FusableTask: public tarch::multicore::Task {
public:
  FusableTask(int priority = tarch::multicore::Task::DefaultPriority):
    Task(0, priority) {}

  ~FusableTask() override = default;

  void fuse(const std::list<Task*>& otherTasks) override {
    numberOfTasksThatHaveBeenExecuted = otherTasks.size() + 1;
    for (auto& otherTask: otherTasks) {
      delete otherTask;
    }
  }

  bool run() override {
    // Do nothing
    return false;
  }
};

exahype2::tests::EnclaveBookkeepingTest::EnclaveBookkeepingTest():
  TestCase("exahype2::tests::EnclaveBookkeepingTest") {}

void exahype2::tests::EnclaveBookkeepingTest::run() {
#ifdef SharedOMP
#pragma omp parallel
    {
#pragma omp master
#endif
      {
        testMethod([this]() { testFusePendingTasks(); });
      }
#ifdef SharedOMP
    }
#endif
}

void exahype2::tests::EnclaveBookkeepingTest::testFusePendingTasks() {
  numberOfTasksThatHaveBeenExecuted = 0;
  for (int i = 0; i < NumberOfTasksToExecute; i++) {
    EnclaveBookkeeping::getInstance().bookmarkTask(new FusableTask);
  }
  validateEquals(EnclaveBookkeeping::getInstance().getTotalNumberOfTasks(), NumberOfTasksToExecute);
  EnclaveBookkeeping::getInstance().fusePendingTasks();
  validateEquals(numberOfTasksThatHaveBeenExecuted, NumberOfTasksToExecute);
}
