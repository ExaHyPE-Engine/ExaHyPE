#include "parallel.h"


int peano4::parallel::getTaskType(const std::string&  className) {
  static int taskTypeCounter = -1;
  taskTypeCounter++;
  return taskTypeCounter;
}

