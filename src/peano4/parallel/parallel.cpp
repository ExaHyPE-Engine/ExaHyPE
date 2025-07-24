// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "parallel.h"

int peano4::parallel::getTaskType(const std::string&) {
  static int taskTypeCounter = -1;
  taskTypeCounter++;
  return taskTypeCounter;
}
