#include "BackfillAndDeployRoundRobin.h"
#include "tarch/multicore/Tasks.h"
#include "tarch/multicore/Core.h"
#include <limits>


tarch::multicore::orchestration::BackfillAndDeployRoundRobin::BackfillAndDeployRoundRobin(
  int  minTasksToFuse
):
  _minTasksToFuse(minTasksToFuse),
  _nextDeviceToUse(-1),
  _inBSPSection(false) {
  toggleDevice();
}


void tarch::multicore::orchestration::BackfillAndDeployRoundRobin::toggleDevice() {
  _nextDeviceToUse++;
  if ( tarch::multicore::Core::getInstance().getNumberOfGPUs()==0 ) {
    _nextDeviceToUse = Task::Host;
  }
  else {
    _nextDeviceToUse = _nextDeviceToUse % tarch::multicore::Core::getInstance().getNumberOfGPUs();
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


bool tarch::multicore::orchestration::BackfillAndDeployRoundRobin::processPendingTasksWhileWaitingInBSPSection() {
  return true;
}
