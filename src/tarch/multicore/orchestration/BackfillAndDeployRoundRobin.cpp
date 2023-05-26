#include "BackfillAndDeployRoundRobin.h"
#include "tarch/multicore/Tasks.h"
#include "tarch/multicore/Core.h"
#include "tarch/accelerator/Device.h"
#include <limits>


tarch::multicore::orchestration::BackfillAndDeployRoundRobin::BackfillAndDeployRoundRobin(
  int  minTasksToFuse
):
  _minTasksToFuse(minTasksToFuse),
  _nextDeviceToUse(Task::Host),
  _inBSPSection(false) {
}


void tarch::multicore::orchestration::BackfillAndDeployRoundRobin::toggleDevice() {
  if ( tarch::accelerator::Device::getInstance().getNumberOfDevices()==0 ) {
    _nextDeviceToUse = Task::Host;
  }
  else {
    _nextDeviceToUse++;
    _nextDeviceToUse = _nextDeviceToUse % tarch::accelerator::Device::getInstance().getNumberOfDevices();
  }
}


void tarch::multicore::orchestration::BackfillAndDeployRoundRobin::startBSPSection() {
  _inBSPSection = true;
}


void tarch::multicore::orchestration::BackfillAndDeployRoundRobin::endBSPSection() {
  _inBSPSection = false;
}


int tarch::multicore::orchestration::BackfillAndDeployRoundRobin::getNumberOfTasksToHoldBack() {
  return _inBSPSection ? std::numeric_limits<int>::max() : 0;
}


tarch::multicore::orchestration::BackfillAndDeployRoundRobin::FuseInstruction tarch::multicore::orchestration::BackfillAndDeployRoundRobin::getNumberOfTasksToFuseAndTargetDevice() {
  int deviceForFusedTasks = _nextDeviceToUse;
  toggleDevice();
  return tarch::multicore::orchestration::BackfillAndDeployRoundRobin::FuseInstruction(
    deviceForFusedTasks,
    _minTasksToFuse,
    std::numeric_limits<int>::max()
  );
}


bool tarch::multicore::orchestration::BackfillAndDeployRoundRobin::fuseTasksImmediatelyWhenSpawned() {
  return true;
}


tarch::multicore::orchestration::Strategy::ExecutionPolicy tarch::multicore::orchestration::BackfillAndDeployRoundRobin::getExecutionPolicyForForkJoinSection(int nestedParallelismLevel, int numberOfTasks, int codeLocationIdentifier) {
  return nestedParallelismLevel>1 ? tarch::multicore::orchestration::Strategy::ExecutionPolicy::EmbedWithoutSubTasks : tarch::multicore::orchestration::Strategy::ExecutionPolicy::MaximiseThroughput;
}
