#include "EnclaveParticleBookkeeping.h"
#include "EnclaveParticleTask.h"

#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/Tasks.h"
#include "peano4/parallel/Tasks.h"

#include "tarch/logging/Statistics.h"

#include <algorithm>


tarch::logging::Log
peano4::EnclaveParticleBookkeeping::_log( "peano4::EnclaveParticleBookkeeping" );

const std::string peano4::EnclaveParticleBookkeeping::MemoryAllocationsInLookupTableIdentifier( "peano4::EnclaveParticleBookkeeping::memory-allocations" );
const std::string peano4::EnclaveParticleBookkeeping::LookupMissesIdentifier( "peano4::EnclaveParticleBookkeeping::lookup-misses" );



peano4::EnclaveParticleBookkeeping& peano4::EnclaveParticleBookkeeping::getInstance() {
  static EnclaveParticleBookkeeping singleton;
  return singleton;
}


void peano4::EnclaveParticleBookkeeping::dumpStatistics() {
  logInfo( "dumpStatistics()", "active tasks=" << tarch::multicore::getNumberOfReservedTaskNumbers() );

  std::ostringstream finishedTasksMsg;
  finishedTasksMsg << "(#" << _finishedTasks.size();
  for (auto& p: _finishedTasks) {
    finishedTasksMsg << "," << p;
  }
  finishedTasksMsg << ")";
  logInfo( "dumpStatistics()", "finished tasks=" << finishedTasksMsg.str() );
}


void peano4::EnclaveParticleBookkeeping::waitForTasksToTerminate(const peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexTaskCounter>& taskCounters) {
  logDebug( "waitForTaskToTerminate(int)", "check for completion of task " << number );
  tarch::multicore::Lock finishedTasksLock( _tasksSemaphore );
  bool adjacentTasksCompleted = true;
  for (int i=0; i<TwoPowerD; i++) {
    adjacentTasksCompleted &= taskCounters(i).getNumTasksRemaining() == 0;
  }
  finishedTasksLock.free();

  while (not adjacentTasksCompleted) {
    ::tarch::logging::Statistics::getInstance().inc( LookupMissesIdentifier );

    bool processedTasks = ::tarch::multicore::processPendingTasks(ThreePowerD);

    if (not processedTasks) {
      tarch::multicore::yield();
    }

    finishedTasksLock.lock();
    adjacentTasksCompleted = true;
    for (int i=0; i<TwoPowerD; i++) {
      adjacentTasksCompleted &= taskCounters(i).getNumTasksRemaining() == 0;
    }
    finishedTasksLock.free();
  }

  logDebug( "waitForTaskToTerminate()", "returned successfully" );
}


void peano4::EnclaveParticleBookkeeping::spawnTask(EnclaveParticleTask* task, peano4::parallel::Tasks::TaskType priority, const std::string& location) {
  tarch::multicore::Lock lock( _tasksSemaphore );
  for (int i=0; i<TwoPowerD; i++) {
    task->_taskCounters(i).addTask();
  }
  lock.free();

  peano4::parallel::Tasks spawn(
    task,
    priority,
    peano4::parallel::Tasks::getLocationIdentifier( location )
  );
}


void peano4::EnclaveParticleBookkeeping::finishedTask(int taskNumber, const peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexTaskCounter>& taskCounters) {
  logDebug( "finishedTask()", "task " << taskNumber << " has terminated. Bookkeep results" );
  tarch::multicore::Lock lockFinishedTasks( _tasksSemaphore );
  assertionEquals( _finishedTasks.count(taskNumber),0 );
  auto oldBucketCount = _finishedTasks.bucket_count();
  _finishedTasks.insert( taskNumber );
  if (_finishedTasks.bucket_count()>oldBucketCount) {
    ::tarch::logging::Statistics::getInstance().inc( MemoryAllocationsInLookupTableIdentifier );
  }
  for (int i=0; i<TwoPowerD; i++) {
    taskCounters(i).removeTask();
  }
  lockFinishedTasks.free();
}
