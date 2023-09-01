#include "parallel.h"

#include "tarch/logging/Log.h"


int peano4::parallel::getTaskType(const std::string&  className) {
  static tarch::logging::Log _log( "peano4::parallel" );

  static int taskTypeCounter = -1;
  taskTypeCounter++;

  logInfo( "getTaskType(string)", "assign task " << className << " id " << taskTypeCounter );
  return taskTypeCounter;
}
