#include "BSP.h"

#include <limits>


void tarch::multicore::orchestration::BSP::startBSPSection() {}


void tarch::multicore::orchestration::BSP::endBSPSection() {}


int tarch::multicore::orchestration::BSP::getNumberOfTasksToHoldBack() {
  return std::numeric_limits<int>::max();
}


int tarch::multicore::orchestration::BSP::getNumberOfTasksToFuse() {
  return 0;
}

