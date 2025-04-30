// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SpawnTaskTest.h"

#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Task.h"

#include <chrono>
#include <thread>

constexpr int NumberOfTasksToExecute = 100;

int numberOfProducerTasksThatHaveBeenExecuted = 0;
int numberOfConsumerTasksThatHaveBeenExecuted = 0;

tarch::multicore::BooleanSemaphore producerTaskSemaphore;
tarch::multicore::BooleanSemaphore consumerTaskSemaphore;

class ProducerTask: public tarch::multicore::Task {
public:
  ProducerTask():
    tarch::multicore::Task(Task::DontFuse, Task::DefaultPriority) {}

  ~ProducerTask() override = default;

  bool run() override {
    tarch::multicore::Lock lock(producerTaskSemaphore);
    numberOfProducerTasksThatHaveBeenExecuted++;
    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // simulate work
    return false;
  }

  bool canFuse() const override { return false; }
};

class ConsumerTask: public tarch::multicore::Task {
public:
  ConsumerTask():
    tarch::multicore::Task(Task::DontFuse, Task::DefaultPriority) {}

  ~ConsumerTask() override = default;

  bool run() override {
    tarch::multicore::Lock lock(consumerTaskSemaphore);
    numberOfConsumerTasksThatHaveBeenExecuted++;
    return false;
  }

  bool canFuse() const override { return false; }
};

tarch::multicore::tests::SpawnTaskTest::SpawnTaskTest():
  TestCase("tarch::multicore::tests::SpawnTaskTest") {}

void tarch::multicore::tests::SpawnTaskTest::run() {
#ifdef SharedOMP
#pragma omp parallel
  {
#pragma omp master
#endif
    {
      testMethod([this]() { testSpawnIndependentTask(); });
      testMethod([this]() { testSpawnTaskWithOutDependency(); });
      testMethod([this]() { testSpawnTaskWithInAndOutDependency(); });
      testMethod([this]() { testSpawnTasks(); });
    }
#ifdef SharedOMP
  }
#endif
}

void tarch::multicore::tests::SpawnTaskTest::testSpawnIndependentTask() {
  numberOfProducerTasksThatHaveBeenExecuted = 0;
  for (int i = 0; i < NumberOfTasksToExecute; i++) {
    spawnTask(new ProducerTask);
  }
  waitForAllTasks();
  validateEquals(numberOfProducerTasksThatHaveBeenExecuted, NumberOfTasksToExecute);
}

void tarch::multicore::tests::SpawnTaskTest::testSpawnTaskWithOutDependency() {
  numberOfProducerTasksThatHaveBeenExecuted = 0;
  std::vector<char> outDependencies(NumberOfTasksToExecute);
  for (int i = 0; i < NumberOfTasksToExecute; i++) {
    spawnTask(new ProducerTask, nullptr, &outDependencies[i]);
  }
  for (int i = 0; i < NumberOfTasksToExecute; i++) {
    waitForTask(&outDependencies[i]);
  }
  validateEquals(numberOfProducerTasksThatHaveBeenExecuted, NumberOfTasksToExecute);
}

void tarch::multicore::tests::SpawnTaskTest::testSpawnTaskWithInAndOutDependency() {
  numberOfProducerTasksThatHaveBeenExecuted = 0;
  numberOfConsumerTasksThatHaveBeenExecuted = 0;
  std::vector<char> producerDependencies(NumberOfTasksToExecute);
  std::vector<char> consumerDependencies(NumberOfTasksToExecute);
  for (int i = 0; i < NumberOfTasksToExecute; i++) {
    spawnTask(new ProducerTask, nullptr, &producerDependencies[i]);
    spawnTask(new ConsumerTask, &producerDependencies[i], &consumerDependencies[i]);
  }
  for (int i = 0; i < NumberOfTasksToExecute; i++) {
    waitForTask(&consumerDependencies[i]);
  }
  validateEquals(numberOfProducerTasksThatHaveBeenExecuted, NumberOfTasksToExecute);
  validateEquals(numberOfConsumerTasksThatHaveBeenExecuted, NumberOfTasksToExecute);
}

void tarch::multicore::tests::SpawnTaskTest::testSpawnTasks() {
  numberOfProducerTasksThatHaveBeenExecuted = 0;
  std::vector<Task*> tasks;
  for (int i = 0; i < NumberOfTasksToExecute; i++) {
    tasks.push_back(new ProducerTask);
  }
  spawnTasks(tasks);
  validateEquals(numberOfProducerTasksThatHaveBeenExecuted, NumberOfTasksToExecute);
}
