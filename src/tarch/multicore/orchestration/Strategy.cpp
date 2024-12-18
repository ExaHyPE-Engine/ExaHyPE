#include "Strategy.h"
#include "AllOnGPU.h"

tarch::multicore::orchestration::Strategy* tarch::multicore::orchestration::createDefaultStrategy() {
  // @todo Round Robin instead
  return new AllOnGPU(0);
}
