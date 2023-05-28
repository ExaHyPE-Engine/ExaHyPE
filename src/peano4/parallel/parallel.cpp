#include "parallel.h"

int peano4::parallel::getTaskType() {
  static int taskTypeCounter = -1;
  taskTypeCounter++;
  return taskTypeCounter;
}
