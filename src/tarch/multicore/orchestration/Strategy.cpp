#include "Strategy.h"
#include "AllOnGPU.h"

tarch::multicore::orchestration::Strategy* tarch::multicore::orchestration::createDefaultStrategy() {
  return new AllOnGPU(0);
}
