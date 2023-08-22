#include "Hardcoded.h"

#include <limits>

#include "tarch/multicore/Tasks.h"

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
  int numberOfTasksToFuse, bool fuseImmediately, bool processTasksWhileWaitingInBSPArea, int targetDevice
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

void tarch::multicore::orchestration::Hardcoded::startBSPSection(int nestedParallelismLevel) {}

void tarch::multicore::orchestration::Hardcoded::endBSPSection(int nestedParallelismLevel) {}

int tarch::multicore::orchestration::Hardcoded::getNumberOfTasksToHoldBack(int taskType) { return _numberOfTasksToHoldBack; }

tarch::multicore::orchestration::Hardcoded::FuseInstruction tarch::multicore::orchestration::Hardcoded::
  getNumberOfTasksToFuseAndTargetDevice(int taskType) {
  return tarch::multicore::orchestration::Hardcoded::FuseInstruction(
    _deviceForFusedTasks, _minTasksToFuse, _maxTasksToFuse
  );
}

bool tarch::multicore::orchestration::Hardcoded::fuseTasksImmediatelyWhenSpawned(int taskType) {
  return _fuseTasksImmediatelyWhenSpawned;
}

tarch::multicore::orchestration::Strategy::ExecutionPolicy tarch::multicore::orchestration::Hardcoded::
  paralleliseForkJoinSection(int nestedParallelismLevel, int numberOfTasks, int codeLocationIdentifier) {
  if (nestedParallelismLevel > _maxNestedConcurrency) {
    return tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunSerially;
  } else if (nestedParallelismLevel <= 1) {
    return tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunParallel;
  } else {
    return tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunParallelAndIgnoreWithholdSubtasks;
  }
}
