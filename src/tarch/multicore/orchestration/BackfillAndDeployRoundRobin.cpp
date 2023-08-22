#include "BackfillAndDeployRoundRobin.h"

#include <limits>

#include "tarch/accelerator/Device.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/Tasks.h"

tarch::multicore::orchestration::BackfillAndDeployRoundRobin::BackfillAndDeployRoundRobin(int minTasksToFuse):
  _minTasksToFuse(minTasksToFuse),
  _nextDeviceToUse(Task::Host),
  _inBSPSection(false) {}

void tarch::multicore::orchestration::BackfillAndDeployRoundRobin::toggleDevice() {
  if (tarch::accelerator::Device::getInstance().getNumberOfDevices() == 0) {
    _nextDeviceToUse = Task::Host;
  } else {
    _nextDeviceToUse++;
    _nextDeviceToUse = _nextDeviceToUse % tarch::accelerator::Device::getInstance().getNumberOfDevices();
  }
}

void tarch::multicore::orchestration::BackfillAndDeployRoundRobin::startBSPSection(int nestedParallelismLevel) {
  if (nestedParallelismLevel <= 1) {
    _inBSPSection = true;
  }
}

void tarch::multicore::orchestration::BackfillAndDeployRoundRobin::endBSPSection(int nestedParallelismLevel) {
  if (nestedParallelismLevel <= 1) {
    _inBSPSection = false;
  }
}

int tarch::multicore::orchestration::BackfillAndDeployRoundRobin::getNumberOfTasksToHoldBack(int taskType) {
  return _inBSPSection ? std::numeric_limits<int>::max() : 0;
}

tarch::multicore::orchestration::BackfillAndDeployRoundRobin::FuseInstruction tarch::multicore::orchestration::
  BackfillAndDeployRoundRobin::getNumberOfTasksToFuseAndTargetDevice(int taskType) {
  int deviceForFusedTasks = _nextDeviceToUse;
  toggleDevice();
  return tarch::multicore::orchestration::BackfillAndDeployRoundRobin::FuseInstruction(
    deviceForFusedTasks, _minTasksToFuse, std::numeric_limits<int>::max()
  );
}

bool tarch::multicore::orchestration::BackfillAndDeployRoundRobin::fuseTasksImmediatelyWhenSpawned(int taskType) { return true; }

tarch::multicore::orchestration::Strategy::ExecutionPolicy tarch::multicore::orchestration::
  BackfillAndDeployRoundRobin::paralleliseForkJoinSection(
    int nestedParallelismLevel, int numberOfTasks, int codeLocationIdentifier
  ) {
  if (nestedParallelismLevel <= 1) {
    return tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunParallel;
  } else {
    return tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunParallelAndIgnoreWithholdSubtasks;
  }
}
