#include "Hardcoded.h"
#include "tarch/multicore/Tasks.h"
#include <limits>


tarch::multicore::orchestration::Hardcoded* tarch::multicore::orchestration::Hardcoded::createBSP() {
  return new Hardcoded(
    std::numeric_limits<int>::max(), // numberOfTasksToHoldBack
    0,                               // tasksToFuse
    tarch::multicore::Task::Host,    // deviceForFusedTasks
    false,                           // fuseTasksImmediatelyWhenSpawned
    false                            // processPendingTasksWhileWaiting
  );
}


tarch::multicore::orchestration::Hardcoded* tarch::multicore::orchestration::Hardcoded::createNative() {
  return new Hardcoded(
    0,                               // numberOfTasksToHoldBack
    0,                               // tasksToFuse
    tarch::multicore::Task::Host,    // deviceForFusedTasks
    false,                           // fuseTasksImmediatelyWhenSpawned
    false                            // processPendingTasksWhileWaiting
  );
}


tarch::multicore::orchestration::Hardcoded* tarch::multicore::orchestration::Hardcoded::createBackfill() {
  return new Hardcoded(
    std::numeric_limits<int>::max(), // numberOfTasksToHoldBack
    0,                               // tasksToFuse
    tarch::multicore::Task::Host,    // deviceForFusedTasks
    false,                           // fuseTasksImmediatelyWhenSpawned
    true                             // processPendingTasksWhileWaiting
  );
}


tarch::multicore::orchestration::Hardcoded* tarch::multicore::orchestration::Hardcoded::createFuseAll(int numberOfTasksToFuse, bool fuseImmediately, int targetDevice) {
  return new Hardcoded(
    std::numeric_limits<int>::max(), // numberOfTasksToHoldBack
    numberOfTasksToFuse,             // tasksToFuse
    targetDevice,                    // deviceForFusedTasks
    fuseImmediately,                 // fuseTasksImmediatelyWhenSpawned
    not fuseImmediately              // processPendingTasksWhileWaiting
  );
}


tarch::multicore::orchestration::Hardcoded::Hardcoded(
  int  numberOfTasksToHoldBack,
  int  tasksToFuse,
  int  deviceForFusedTasks,
  bool fuseTasksImmediatelyWhenSpawned,
  bool processPendingTasksWhileWaiting
):
  _numberOfTasksToHoldBack(numberOfTasksToHoldBack),
  _tasksToFuse(tasksToFuse),
  _deviceForFusedTasks(deviceForFusedTasks),
  _fuseTasksImmediatelyWhenSpawned(fuseTasksImmediatelyWhenSpawned),
  _processPendingTasksWhileWaiting(processPendingTasksWhileWaiting) {}


void tarch::multicore::orchestration::Hardcoded::startBSPSection() {}


void tarch::multicore::orchestration::Hardcoded::endBSPSection() {}


int tarch::multicore::orchestration::Hardcoded::getNumberOfTasksToHoldBack() {
  return _numberOfTasksToHoldBack;
}


std::pair<int,int> tarch::multicore::orchestration::Hardcoded::getNumberOfTasksToFuseAndTargetDevice() {
  return std::pair<int,int>(_tasksToFuse,_deviceForFusedTasks);
}


bool tarch::multicore::orchestration::Hardcoded::fuseTasksImmediatelyWhenSpawned() {
  return _fuseTasksImmediatelyWhenSpawned;
}


bool tarch::multicore::orchestration::Hardcoded::processPendingTasksWhileWaitingInBSPSection() {
  return _processPendingTasksWhileWaiting;
}
