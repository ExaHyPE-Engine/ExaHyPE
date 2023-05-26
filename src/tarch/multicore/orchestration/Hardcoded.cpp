#include "Hardcoded.h"
#include "tarch/multicore/Tasks.h"
#include <limits>


tarch::multicore::orchestration::Hardcoded* tarch::multicore::orchestration::Hardcoded::createBSP() {
  return new Hardcoded(
    std::numeric_limits<int>::max(), // numberOfTasksToHoldBack
    std::numeric_limits<int>::max(), // min tasks to fuse
    0,                               // max tasks to fuse
    tarch::multicore::Task::Host,    // deviceForFusedTasks
    false,                           // fuseTasksImmediatelyWhenSpawned
    false,                           // processPendingTasksWhileWaiting
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
    false,                           // processPendingTasksWhileWaiting
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
    true,                            // processPendingTasksWhileWaiting
    1
  );
}


tarch::multicore::orchestration::Hardcoded* tarch::multicore::orchestration::Hardcoded::createFuseAll(int numberOfTasksToFuse, bool fuseImmediately, bool processTasksWhileWaitingInBSPArea, int targetDevice) {
  return new Hardcoded(
    std::numeric_limits<int>::max(),   // numberOfTasksToHoldBack
    numberOfTasksToFuse,               // tasksToFuse
    numberOfTasksToFuse,               // tasksToFuse
    targetDevice,                      // deviceForFusedTasks
    fuseImmediately,                   // fuseTasksImmediatelyWhenSpawned
    processTasksWhileWaitingInBSPArea, // processPendingTasksWhileWaiting
    1
  );
}


tarch::multicore::orchestration::Hardcoded::Hardcoded(
  int  numberOfTasksToHoldBack,
  int  minTasksToFuse,
  int  maxTasksToFuse,
  int  deviceForFusedTasks,
  bool fuseTasksImmediatelyWhenSpawned,
  bool processPendingTasksWhileWaiting,
  int  maxNestedConcurrency
):
  _numberOfTasksToHoldBack(numberOfTasksToHoldBack),
  _minTasksToFuse(minTasksToFuse),
  _maxTasksToFuse(maxTasksToFuse),
  _deviceForFusedTasks(deviceForFusedTasks),
  _fuseTasksImmediatelyWhenSpawned(fuseTasksImmediatelyWhenSpawned),
  _processPendingTasksWhileWaiting(processPendingTasksWhileWaiting),
  _maxNestedConcurrency(maxNestedConcurrency) {}


void tarch::multicore::orchestration::Hardcoded::startBSPSection() {}


void tarch::multicore::orchestration::Hardcoded::endBSPSection() {}


int tarch::multicore::orchestration::Hardcoded::getNumberOfTasksToHoldBack() {
  return _numberOfTasksToHoldBack;
}


tarch::multicore::orchestration::Hardcoded::FuseInstruction tarch::multicore::orchestration::Hardcoded::getNumberOfTasksToFuseAndTargetDevice() {
  return tarch::multicore::orchestration::Hardcoded::FuseInstruction( _deviceForFusedTasks, _minTasksToFuse, _maxTasksToFuse );
}


bool tarch::multicore::orchestration::Hardcoded::fuseTasksImmediatelyWhenSpawned() {
  return _fuseTasksImmediatelyWhenSpawned;
}


tarch::multicore::orchestration::Strategy::ExecutionPolicy tarch::multicore::orchestration::Hardcoded::getExecutionPolicyForForkJoinSection(int nestedParallelismLevel, int numberOfTasks, int codeLocationIdentifier) {
  if (nestedParallelismLevel>_maxNestedConcurrency) {
    return tarch::multicore::orchestration::Strategy::ExecutionPolicy::EmbedWithoutSubTasks;
  }
  else if (_processPendingTasksWhileWaiting) {
    return tarch::multicore::orchestration::Strategy::ExecutionPolicy::MaximiseThroughput;
  }
  else {
    return tarch::multicore::orchestration::Strategy::ExecutionPolicy::MinimiseLatency;
  }
}
