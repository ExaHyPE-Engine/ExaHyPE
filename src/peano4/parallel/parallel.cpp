#include "parallel.h"

#include "tarch/logging/Log.h"
#include "tarch/mpi/Rank.h"


int peano4::parallel::getTaskType(const std::string&  className) {
  static int taskTypeCounter = -1;
  taskTypeCounter++;

  if (tarch::mpi::Rank::getInstance().isGlobalMaster() ) {
    std::cout << "assign task " << className << " id " << taskTypeCounter  << std::endl;
  }

  return taskTypeCounter;
}
