#include "config.h"
#if defined(UseOtter)
#define OTTER_TASK_GRAPH_ENABLE_USER
#define ROOT_TASK_LABEL "[ROOT TASK]"
#endif

#include "EnclaveBookkeeping.h"
#include "EnclaveTask.h"

#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/Tasks.h"
#include "tarch/multicore/otter.h"
#include "tarch/accelerator/Device.h"

#include "tarch/logging/Statistics.h"

#include <algorithm>



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
    finishedTasksMsg << "," << p.first << "->" << p.second.toString();
  }
  finishedTasksMsg << ")";
  logInfo( "dumpStatistics()", "finished tasks=" << finishedTasksMsg.str() );
}


void exahype2::EnclaveBookkeeping::cancelTask(int number) {
  tarch::multicore::Lock finishedTasksLock( _finishedTasksSemaphore );

  if (_finishedTasks.count( number )>0) {
    Entry storedData = _finishedTasks.at(number);
    _finishedTasks.erase( number );
    tarch::freeMemory(storedData.data,tarch::MemoryLocation::Heap );
    tarch::multicore::releaseTaskNumber(number);
  }
  else {
    _tasksThatHaveToBeCancelled.insert(number);
  }
}


exahype2::EnclaveBookkeeping::Entry exahype2::EnclaveBookkeeping::waitForTaskToTerminateAndReturnResult(int number) {
  tarch::multicore::Lock finishedTasksLock( _finishedTasksSemaphore );
  bool isContained = _finishedTasks.count( number )>0;
  finishedTasksLock.free();

  while (not isContained) {
    ::tarch::logging::Statistics::getInstance().inc( LookupMissesIdentifier );

    bool foundTask = ::tarch::multicore::processPendingTasks(1,false);
    if (not foundTask) {
      ::tarch::multicore::Core::getInstance().yield();
    }

    finishedTasksLock.lock();
    isContained = _finishedTasks.count( number )>0;
    finishedTasksLock.free();
  }

  finishedTasksLock.lock();
  assertionEquals( _finishedTasks.count(number),1 );
  Entry storedData = _finishedTasks.at(number);
  _finishedTasks.erase( number );
  finishedTasksLock.free();

  tarch::multicore::releaseTaskNumber(number);

  // TODO (AT): not sure this synchronisation point makes sense - try removing it
#if 0
  OTTER_DECLARE(root_task);
  OTTER_BORROW(root_task, ROOT_TASK_LABEL);
  OTTER_SYNCHRONISE(root_task, descendants);
#endif

  return storedData;
}


void exahype2::EnclaveBookkeeping::waitForTaskToTerminateAndCopyResultOver(int number, double* destination, double& maxEigenvalue) {
  logTraceInWith1Argument( "waitForTaskToTerminateAndCopyResultOver(int,double,double&)", number );

  Entry storedData = waitForTaskToTerminateAndReturnResult(number);

  std::copy_n( storedData.data, storedData.numberOfResultValues, destination );
  maxEigenvalue = storedData.maxEigenvalue;

  tarch::freeMemory( storedData.data, tarch::MemoryLocation::Heap );
  logTraceOutWith2Arguments( "waitForTaskToTerminateAndCopyResultOver(int,double,double&)", maxEigenvalue, storedData.toString() );
}


void exahype2::EnclaveBookkeeping::finishedTask(int taskNumber, int numberOfResultValues, double* data, double maxEigenvalue) {
  logDebug( "finishedTask()", "task " << taskNumber << " has terminated. Bookkeep results" );

  tarch::multicore::Lock lockFinishedTasks( _finishedTasksSemaphore );
  assertionEquals1( _finishedTasks.count(taskNumber),0,taskNumber );
  if (_tasksThatHaveToBeCancelled.count(taskNumber)) {
    _tasksThatHaveToBeCancelled.erase(taskNumber);
    delete[] data;
    lockFinishedTasks.free();
    tarch::multicore::releaseTaskNumber(taskNumber);
  }
  else {
    auto oldBucketCount = _finishedTasks.bucket_count();
    Entry newEntry(numberOfResultValues,data,maxEigenvalue);
    _finishedTasks.insert( std::pair<int, Entry >(taskNumber,newEntry) );
    if (_finishedTasks.bucket_count()>oldBucketCount) {
      ::tarch::logging::Statistics::getInstance().inc( MemoryAllocationsInLookupTableIdentifier );
    }
    lockFinishedTasks.free();
  }
}


exahype2::EnclaveBookkeeping::Entry::Entry( int  numberOfResultValues_, double*  data_, double   maxEigenvalue_ ):
  numberOfResultValues(numberOfResultValues_),
  data(data_),
  maxEigenvalue(maxEigenvalue_) {}


std::string exahype2::EnclaveBookkeeping::Entry::toString() const {
  std::ostringstream msg;
  msg << "(#" << numberOfResultValues << ",\\lambda_max=" << maxEigenvalue << ")";
  return msg.str();
}
