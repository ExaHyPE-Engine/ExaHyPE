#include "tarch/multicore/multicore.h"

#ifdef SharedTBB
#include "TBB.h"
#include "tarch/logging/Log.h"


int tarch::multicore::MaxNumberOfConcurrentHighBandwidthTasks = std::numeric_limits<int>::max();
int tarch::multicore::MaxNumberOfConcurrentBackgroundTasks    = std::numeric_limits<int>::max();

#endif
