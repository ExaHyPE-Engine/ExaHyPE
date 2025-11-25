#include "parallel.h"

#include "tarch/Assertions.h"
#include "tarch/logging/Log.h"
#include "tarch/mpi/Rank.h"


namespace {
  std::vector< std::string > classNamesForTaskType;
}


int peano4::parallel::getTaskType(const std::string&  className) {
  static int taskTypeCounter = -1;
  taskTypeCounter++;

  classNamesForTaskType.push_back( className );

  #if PeanoDebug>0
  std::cout << "assign task " << className << " id " << taskTypeCounter  << std::endl;
  #endif

  return taskTypeCounter;
}


std::string peano4::parallel::getClassNameOfType(int taskType) {
  assertion( taskType>=0 );
  assertion( taskType<classNamesForTaskType.size() );
  return classNamesForTaskType.at(taskType);
}

