#include "smartScheduling.h"

#if defined(UseSmartMPI)

#include "smartmpi.h"

int tarch::multicore::registerSmartMPITask(int taskTypeNumber, smartmpi::Receiver functor) {
  smartmpi::registerReceiver(
    taskTypeNumber,
    functor
  );
  return taskTypeNumber;
}

#endif
