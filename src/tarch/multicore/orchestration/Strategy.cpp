#include "Strategy.h"
#include "AllOnGPU.h"


tarch::multicore::orchestration::Strategy* tarch::multicore::orchestration::createDefaultStrategy() {
  // @todo Round Robin instaed
  return new AllOnGPU(0);
}
