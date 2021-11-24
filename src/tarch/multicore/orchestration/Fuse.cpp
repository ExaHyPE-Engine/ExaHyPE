#include "Fuse.h"

#include <limits>


tarch::multicore::orchestration::Fuse::Fuse(int numberOfTaskToFuse, int targetDevice):
  _numberOfTaskToFuse(numberOfTaskToFuse),
  _targetDevice(targetDevice),
  _isInBSPSection(false) {
}


void tarch::multicore::orchestration::Fuse::startBSPSection() {
  _isInBSPSection = true;
}


void tarch::multicore::orchestration::Fuse::endBSPSection() {
  _isInBSPSection = false;
}


int tarch::multicore::orchestration::Fuse::getNumberOfTasksToHoldBack() {
  return _isInBSPSection ? std::numeric_limits<int>::max() : 0;
}


std::pair<int,int> tarch::multicore::orchestration::Fuse::getNumberOfTasksToFuseAndTargetDevice() {
  return _isInBSPSection ? std::pair<int,int>(_numberOfTaskToFuse,_targetDevice) : std::pair<int,int>(0,0);
}

