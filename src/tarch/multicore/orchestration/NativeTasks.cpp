#include "NativeTasks.h"


void tarch::multicore::orchestration::NativeTasks::startBSPSection() {}


void tarch::multicore::orchestration::NativeTasks::endBSPSection() {}


int tarch::multicore::orchestration::NativeTasks::getNumberOfTasksToHoldBack() {
  return 0;
}


std::pair<int,int> tarch::multicore::orchestration::NativeTasks::getNumberOfTasksToFuseAndTargetDevice() {
  return std::pair<int,int>(0,0);
}

