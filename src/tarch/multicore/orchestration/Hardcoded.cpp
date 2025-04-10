#include "Hardcoded.h"

#include "tarch/multicore/Task.h"

#include <limits>

tarch::multicore::orchestration::Hardcoded* tarch::multicore::orchestration::Hardcoded::createBSP() {
  return new Hardcoded(
    std::numeric_limits<int>::max(), // numberOfTasksToHoldBack
    std::numeric_limits<int>::max(), // min tasks to fuse
    0,                               // max tasks to fuse
    tarch::multicore::Task::Host,    // deviceForFusedTasks
    false,                           // fuseTasksImmediatelyWhenSpawned
    1
  );
}

tarch::multicore::orchestration::Hardcoded* tarch::multicore::orchestration::Hardcoded::createNative() {
  return new Hardcoded(
    0,                               // numberOfTasksToHoldBack
    std::numeric_limits<int>::max(), // min tasks to fuse
    0,                               // max tasks to fuse
    tarch::multicore::Task::Host,    // deviceForFusedTasks
    false,                           // fuseTasksImmediatelyWhenSpawned
    1
  );
}

tarch::multicore::orchestration::Hardcoded* tarch::multicore::orchestration::Hardcoded::createBackfill() {
  return new Hardcoded(
    std::numeric_limits<int>::max(), // numberOfTasksToHoldBack
    std::numeric_limits<int>::max(), // min tasks to fuse
    0,                               // max tasks to fuse
    tarch::multicore::Task::Host,    // deviceForFusedTasks
    false,                           // fuseTasksImmediatelyWhenSpawned
    1
  );
}

tarch::multicore::orchestration::Hardcoded* tarch::multicore::orchestration::Hardcoded::createFuseAll(
  [[maybe_unused]] int   numberOfTasksToFuse,
  [[maybe_unused]] bool  fuseImmediately,
  [[maybe_unused]] bool  processTasksWhileWaitingInBSPArea,
  [[maybe_unused]] int   targetDevice
) {
  return new Hardcoded(
    std::numeric_limits<int>::max(),   // numberOfTasksToHoldBack
    numberOfTasksToFuse,               // tasksToFuse
    numberOfTasksToFuse,               // tasksToFuse
    targetDevice,                      // deviceForFusedTasks
    fuseImmediately,                   // fuseTasksImmediatelyWhenSpawned
    1
  );
}

tarch::multicore::orchestration::Hardcoded::Hardcoded(
  int  numberOfTasksToHoldBack,
  int  minTasksToFuse,
  int  maxTasksToFuse,
  int  deviceForFusedTasks,
  bool fuseTasksImmediatelyWhenSpawned,
  int  maxNestedConcurrency
):
  _numberOfTasksToHoldBack(numberOfTasksToHoldBack),
  _minTasksToFuse(minTasksToFuse),
  _maxTasksToFuse(maxTasksToFuse),
  _deviceForFusedTasks(deviceForFusedTasks),
  _fuseTasksImmediatelyWhenSpawned(fuseTasksImmediatelyWhenSpawned),
  _maxNestedConcurrency(maxNestedConcurrency) {}

void tarch::multicore::orchestration::Hardcoded::startBSPSection(int /*nestedParallelismLevel*/) {}

void tarch::multicore::orchestration::Hardcoded::endBSPSection(int /*nestedParallelismLevel*/) {}

tarch::multicore::orchestration::Hardcoded::FuseInstruction tarch::multicore::orchestration::Hardcoded::fuse(int /*taskType*/) {
  return tarch::multicore::orchestration::Hardcoded::FuseInstruction(
    _deviceForFusedTasks, _minTasksToFuse, _maxTasksToFuse
  );
}

tarch::multicore::orchestration::Strategy::ExecutionPolicy tarch::multicore::orchestration::Hardcoded::paralleliseForkJoinSection(
  [[maybe_unused]] int nestedParallelismLevel,
  [[maybe_unused]] int numberOfTasks,
  [[maybe_unused]] int codeLocationIdentifier
) {
  if (nestedParallelismLevel > _maxNestedConcurrency) {
    return tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunSerially;
  } else {
    return tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunParallel;
  }
}



void tarch::multicore::orchestration::Hardcoded::updateFuseStatistics(
  int    taskType,
  int    queueSize,
  double averageNumberOfPendingReadyTasksPerProcessTaskExecution
) {
  // empty on purpose
}
