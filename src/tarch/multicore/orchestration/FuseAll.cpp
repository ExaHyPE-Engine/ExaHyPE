#include "FuseAll.h"

#include <limits>


tarch::multicore::orchestration::FuseAll::FuseAll(int numberOfTaskToFuse):
  _numberOfTaskToFuse(numberOfTaskToFuse),
  _isInBSPSection(false) {
}


void tarch::multicore::orchestration::FuseAll::startBSPSection() {
  _isInBSPSection = true;
}


void tarch::multicore::orchestration::FuseAll::endBSPSection() {
  _isInBSPSection = false;
}


int tarch::multicore::orchestration::FuseAll::getNumberOfTasksToHoldBack() {
  return _isInBSPSection ? std::numeric_limits<int>::max() : 0;
}


int tarch::multicore::orchestration::FuseAll::getNumberOfTasksToFuse() {
  return _isInBSPSection ? _numberOfTaskToFuse : 0;
}

