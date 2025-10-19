#include "Strategy.h"
#include "AllOnGPU.h"
#include "Hardcoded.h"
#include "tarch/multicore/Task.h"
#include "tarch/accelerator/accelerator.h"

tarch::multicore::orchestration::Strategy* tarch::multicore::orchestration::createDefaultStrategy() {
  #if defined(GPUOffloadingOff)
  return Hardcoded::createFuseAll(
    16, //  tasks to fuse
    tarch::multicore::Task::Host
  );
  #else
  return new AllOnGPU(0);
  #endif
}


