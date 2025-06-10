// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "parallel.h"

#include "tarch/logging/Log.h"
#include "tarch/mpi/Rank.h"

int peano4::parallel::getTaskType(const std::string& className) {
  static int taskTypeCounter = -1;
  taskTypeCounter++;

#if PeanoDebug>0
  if (tarch::mpi::Rank::getInstance().isGlobalMaster() ) {
    std::cout << "Assign task " << className << " id " << taskTypeCounter  << std::endl;
  }
#endif

  return taskTypeCounter;
}
