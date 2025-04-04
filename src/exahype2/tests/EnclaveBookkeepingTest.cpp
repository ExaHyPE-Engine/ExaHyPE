// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "EnclaveBookkeepingTest.h"

#include "exahype2/EnclaveTask.h"
#include "exahype2/EnclaveBookkeeping.h"

#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/BooleanSemaphore.h"

#include <thread>
#include <chrono>

class NonFusableEnclaveTask : public exahype2::EnclaveTask<double> {
public:
  static void copyInputToOutput(double* input, double* output) {
    assertion(input);
    assertion(output);
    //std::this_thread::sleep_for(std::chrono::nanoseconds(100000)); // Emulate some work
    output[0] = input[0];
    delete[] input;
  }

  NonFusableEnclaveTask(double* input, int priority = tarch::multicore::Task::DefaultPriority - 1)
    : EnclaveTask(
      Task::DontFuse,
      peano4::datamanagement::CellMarker(peano4::grid::GridTraversalEvent{}),
      0.0,
      0.0,
      input,
      nullptr,
      1,
      1,
      [&]() -> void {
        copyInputToOutput(_inputValues, _outputValues);
      }
    ) {
      setPriority(priority);
    }

  ~NonFusableEnclaveTask() override = default;
};

exahype2::tests::EnclaveBookkeepingTest::EnclaveBookkeepingTest():
  TestCase("exahype2::tests::EnclaveBookkeepingTest") {}

void exahype2::tests::EnclaveBookkeepingTest::run() {
  testMethod([this]() { testSpawnAndMergeNonFusableEnclaveTasksWithoutDependencies(5, 1, 1000); });
  testMethod([this]() { testSpawnAndMergeNonFusableEnclaveTasksWithoutDependencies(5, 16, 1000); });
}

void exahype2::tests::EnclaveBookkeepingTest::testSpawnAndMergeNonFusableEnclaveTasksWithoutDependencies(
  int timeSteps,
  int taskLoopIterations,
  int numberOfNonFusableEnclaveTasksToSpawn
) {
  for (int timeStep = 0; timeStep < timeSteps; timeStep++) {
    std::vector<int> taskNumbers(taskLoopIterations * numberOfNonFusableEnclaveTasksToSpawn, -1);
    std::vector<double> results(taskLoopIterations * numberOfNonFusableEnclaveTasksToSpawn, -1.0);

    // Ensure bookkeeping assertions are valid at the start
    validateEquals(EnclaveTask<double>::getNumberOfActiveTasks(), 0);
    validateEquals(EnclaveBookkeeping<double>::getInstance().getNumberOfFinishedTasks(), 0);

    #if defined(SharedOMP)
    #pragma omp parallel
    #pragma omp master
    #pragma omp taskloop
    #endif
    for (int t = 0; t < taskLoopIterations; t++) {
      for (int i = 0; i < numberOfNonFusableEnclaveTasksToSpawn; i++) {
        // Allocate task input
        const int globalIndex = t * numberOfNonFusableEnclaveTasksToSpawn + i;
        double* input = new double[1]{ static_cast<double>(globalIndex) };

        // Create and spawn the task
        NonFusableEnclaveTask* nonFusableEnclaveTask = new NonFusableEnclaveTask(input);
        const int taskNumber = nonFusableEnclaveTask->getTaskId();
        tarch::multicore::spawnTask(nonFusableEnclaveTask, tarch::multicore::NoInDependencies, taskNumber);

        // Store task ID
        taskNumbers.at(globalIndex) = taskNumber;
      }

      for (int i = 0; i < numberOfNonFusableEnclaveTasksToSpawn; i++) {
        const int globalIndex = t * numberOfNonFusableEnclaveTasksToSpawn + i;

        // Merge results
        double result = -1.0;
        double maxEigenvalue = -1.0;
        EnclaveBookkeeping<double>::getInstance().waitForTaskToTerminateAndCopyResultOver(taskNumbers.at(globalIndex), &result, maxEigenvalue);
        results.at(globalIndex) = result;
      }
    }

    // Ensure bookkeeping assertions are valid at the end, i.e. all tasks have been merged
    validateEquals(EnclaveTask<double>::getNumberOfActiveTasks(), 0);
    validateEquals(EnclaveBookkeeping<double>::getInstance().getNumberOfFinishedTasks(), 0);

    for (int i = 0; i < results.size(); i++) {
      validateEqualsWithParams2(results.at(i), static_cast<double>(i), i, taskNumbers.at(i));
    }
  }
}
