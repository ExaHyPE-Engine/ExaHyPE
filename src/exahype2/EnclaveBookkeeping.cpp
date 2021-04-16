#include "EnclaveBookkeeping.h"
#include "EnclaveTask.h"

#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/Tasks.h"

#include "tarch/logging/Statistics.h"

#include <algorithm>


#include "exahype2/fv/Generic.h"


tarch::logging::Log
exahype2::EnclaveBookkeeping::_log( "exahype2::EnclaveBookkeeping" );

const std::string exahype2::EnclaveBookkeeping::MemoryAllocationsInLookupTableIdentifier( "exahype2::EnclaveBookkeeping::memory-allocations" );
const std::string exahype2::EnclaveBookkeeping::LookupMissesIdentifier( "exahype2::EnclaveBookkeeping::lookup-misses" );



exahype2::EnclaveBookkeeping& exahype2::EnclaveBookkeeping::getInstance() {
  static EnclaveBookkeeping singleton;
  return singleton;
}


void exahype2::EnclaveBookkeeping::dumpStatistics() {
  logInfo( "dumpStatistics()", "active tasks=" << tarch::multicore::getNumberOfReservedTaskNumbers() );

  std::ostringstream finishedTasksMsg;
  finishedTasksMsg << "(#" << _finishedTasks.size();
  for (auto& p: _finishedTasks) {
    finishedTasksMsg << "," << p.first << "x" << p.second.first;
  }
  finishedTasksMsg << ")";
  logInfo( "dumpStatistics()", "finished tasks=" << finishedTasksMsg.str() );
}


void exahype2::EnclaveBookkeeping::cancelTask(int number) {
  tarch::multicore::Lock finishedTasksLock( _finishedTasksSemaphore );

  if (_finishedTasks.count( number )>0) {
    std::pair<int, double*> storedData = _finishedTasks[number];
    _finishedTasks.erase( number );
    tarch::freeMemory(storedData.second,tarch::MemoryLocation::Heap );
    tarch::multicore::releaseTaskNumber(number);
  }
  else {
    _tasksThatHaveToBeCancelled.insert(number);
  }
}


std::pair<int, double*>  exahype2::EnclaveBookkeeping::waitForTaskToTerminateAndReturnResult(int number) {
  tarch::multicore::Lock finishedTasksLock( _finishedTasksSemaphore );
  bool isContained = _finishedTasks.count( number )>0;
  finishedTasksLock.free();

  while (not isContained) {
    ::tarch::logging::Statistics::getInstance().inc( LookupMissesIdentifier );

    bool processedTask = ::tarch::multicore::processTask(number);

    if (not processedTask) {
      tarch::multicore::yield();
    }

    finishedTasksLock.lock();
    isContained = _finishedTasks.count( number )>0;
    finishedTasksLock.free();
  }

  finishedTasksLock.lock();
  assertionEquals( _finishedTasks.count(number),1 );
  std::pair<int, double*> storedData = _finishedTasks[number];
  _finishedTasks.erase( number );
  finishedTasksLock.free();

  tarch::multicore::releaseTaskNumber(number);

  return storedData;
}


void exahype2::EnclaveBookkeeping::waitForTaskToTerminateAndCopyResultOver(int number, double* destination) {
  logDebug( "waitForTaskToTerminateAndCopyResultOver(int,double)", "fetch results of task " << number );

  std::pair<int, double*> storedData = waitForTaskToTerminateAndReturnResult(number);

  std::copy_n( storedData.second, storedData.first, destination );

  tarch::freeMemory( storedData.second, tarch::MemoryLocation::Heap );
  logDebug( "waitForTaskToTerminateAndCopyResultOver()", "delivered outcome of task " << number << " (" << storedData.first << " entries copied over)");
}


void exahype2::EnclaveBookkeeping::finishedTask(int taskNumber, int numberOfResultValues, double* data) {
  logDebug( "finishedTask()", "task " << taskNumber << " has terminated. Bookkeep results" );

  tarch::multicore::Lock lockFinishedTasks( _finishedTasksSemaphore );
  assertionEquals( _finishedTasks.count(taskNumber),0 );
  if (_tasksThatHaveToBeCancelled.count(taskNumber)) {
    _tasksThatHaveToBeCancelled.erase(taskNumber);
    delete[] data;
    lockFinishedTasks.free();
    tarch::multicore::releaseTaskNumber(taskNumber);
  }
  else {
    auto oldBucketCount = _finishedTasks.bucket_count();
    std::pair<int,double*> newEntry(numberOfResultValues,data);
    _finishedTasks.insert( std::pair<int, std::pair<int,double*> >(taskNumber,newEntry) );
    if (_finishedTasks.bucket_count()>oldBucketCount) {
      ::tarch::logging::Statistics::getInstance().inc( MemoryAllocationsInLookupTableIdentifier );
    }
    lockFinishedTasks.free();
  }
}


