#include "EnclaveBookkeeping.h"
#include "EnclaveTask.h"

#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/Tasks.h"

#include <algorithm>


tarch::logging::Log  exahype2::EnclaveBookkeeping::_log( "exahype2::EnclaveBookkeeping" );



exahype2::EnclaveBookkeeping& exahype2::EnclaveBookkeeping::getInstance() {
  static EnclaveBookkeeping singleton;
  return singleton;
}


void exahype2::EnclaveBookkeeping::dumpStatistics() {
  std::ostringstream activeTasksMsg;
  activeTasksMsg << "(#" << _activeTaskNumbers.size();
  for (auto& p: _activeTaskNumbers) {
    activeTasksMsg << "," << p;
  }
  activeTasksMsg << ")";
  logInfo( "dumpStatistics()", "active tasks=" << activeTasksMsg.str() );

  std::ostringstream finishedTasksMsg;
  finishedTasksMsg << "(#" << _finishedTasks.size();
  for (auto& p: _finishedTasks) {
    finishedTasksMsg << "," << p.first << "x" << p.second.first;
  }
  finishedTasksMsg << ")";
  logInfo( "dumpStatistics()", "finished tasks=" << finishedTasksMsg.str() );
}


void exahype2::EnclaveBookkeeping::waitForTaskToTerminateAndCopyResultOver(int number, double* destination) {
  logDebug( "waitForTaskToTerminateAndCopyResultOver(int,double)", "fetch results of task " << number );
  tarch::multicore::Lock finishedTasksLock( _finishedTasksSemaphore );
  bool isContained = _finishedTasks.count( number );
  finishedTasksLock.free();

  while (not isContained) {
    bool processedTask = ::tarch::multicore::processPendingTasks(1);

    if (not processedTask) {
      tarch::multicore::yield();
    }

    finishedTasksLock.lock();
    isContained = _finishedTasks.count( number );
    finishedTasksLock.free();
  }

  finishedTasksLock.lock();
  assertionEquals( _finishedTasks.count(number),1 );
  std::pair<int, double*> storedData = _finishedTasks[number];
  _finishedTasks.erase( number );
  finishedTasksLock.free();

  tarch::multicore::Lock activeTasksLock( _activeTasksSemaphore );
  assertionEquals( _activeTaskNumbers.count(number),1 );
  _activeTaskNumbers.erase( number );
  activeTasksLock.free();

  std::copy_n( storedData.second, storedData.first, destination );
  delete[] storedData.second;
  logDebug( "waitForTaskToTerminateAndCopyResultOver()", "delivered outcome of task " << number << " (" << storedData.first << " entries copied over)");
}


int  exahype2::EnclaveBookkeeping::reserveTaskNumber() {
  tarch::multicore::Lock lock( _activeTasksSemaphore );
  int result = _activeTaskNumbers.size();
  while (_activeTaskNumbers.count( result )>0) {
    result+=23;
  }
  _activeTaskNumbers.insert( result );
  logDebug( "reserveTaskNumber()", "return " << result << " at a total task number count of " << _activeTaskNumbers.size() );
  return result;
}


void exahype2::EnclaveBookkeeping::finishedTask(int taskNumber, int numberOfResultValues, double* data) {
  logDebug( "finishedTask()", "task " << taskNumber << " has terminated. Bookkeep results" );

  tarch::multicore::Lock lockFinishedTasks( _finishedTasksSemaphore );
  assertionEquals( _finishedTasks.count(taskNumber),0 );
  std::pair<int,double*> newEntry(numberOfResultValues,data);
  _finishedTasks.insert( std::pair<int, std::pair<int,double*> >(taskNumber,newEntry) );

  lockFinishedTasks.free();
}


