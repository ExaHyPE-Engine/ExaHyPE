#include "SmartScheduler.h"

#if defined(UseSmartMPI)

#include "smartmpi.h"
#include "tarch/Assertions.h"
#include "tarch/multicore/Tasks.h"


tarch::multicore::SmartScheduler::SmartScheduler():
  smartmpi::scheduler::Scheduler( "tarch::multicore::SmartScheduler" ) {
}


bool tarch::multicore::SmartScheduler::schedule(smartmpi::Task* task) {
  tarch::multicore::native::spawnTask( dynamic_cast<tarch::multicore::Task*>(task) );
  return true;
}


void tarch::multicore::SmartScheduler::configure( const smartmpi::topology::Topology& topology ) {
}


void tarch::multicore::SmartScheduler::update( const smartmpi::GlobalStatistics& statistics ) {
}


#endif
