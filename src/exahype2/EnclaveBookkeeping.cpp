#include "EnclaveBookkeeping.h"

#include <algorithm>

#include "EnclaveTask.h"
#include "tarch/Assertions.h"
#include "tarch/accelerator/Device.h"
#include "tarch/logging/Statistics.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/otter.h"
#include "tarch/multicore/Tasks.h"


tarch::logging::Log exahype2::EnclaveBookkeeping::_log("exahype2::EnclaveBookkeeping");

const std::string exahype2::EnclaveBookkeeping::MemoryAllocationsInLookupTableIdentifier(
  "exahype2::EnclaveBookkeeping::memory-allocations"
);


exahype2::EnclaveBookkeeping& exahype2::EnclaveBookkeeping::getInstance() {
  static EnclaveBookkeeping singleton;
  return singleton;
}


void exahype2::EnclaveBookkeeping::dumpStatistics() {
  logInfo("dumpStatistics()", "active tasks=" << exahype2::EnclaveTask::getNumberOfActiveTasks());

  std::ostringstream finishedTasksMsg;
  finishedTasksMsg << "(#" << _finishedTasks.size();
  for (auto& p : _finishedTasks) {
    finishedTasksMsg << "," << p.first << "->" << p.second.toString();
  }
  finishedTasksMsg << ")";
  logInfo("dumpStatistics()", "finished tasks=" << finishedTasksMsg.str());
}


void exahype2::EnclaveBookkeeping::cancelTask(int number) {
  tarch::multicore::Lock finishedTasksLock(_finishedTasksSemaphore);

  if (_finishedTasks.count(number) > 0) {
    Entry storedData = _finishedTasks.at(number);
    _finishedTasks.erase(number);
    tarch::freeMemory(storedData.data, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
    exahype2::EnclaveTask::releaseTaskNumber(number);
  } else {
    _tasksThatHaveToBeCancelled.insert(number);
  }
}


exahype2::EnclaveBookkeeping::Entry exahype2::EnclaveBookkeeping::waitForTaskToTerminateAndReturnResult(int number) {
  tarch::multicore::waitForTask(number);

  tarch::multicore::Lock finishedTasksLock(_finishedTasksSemaphore);
  assertionEquals(_finishedTasks.count(number), 1);
  Entry storedData = _finishedTasks.at(number);
  _finishedTasks.erase(number);
  finishedTasksLock.free();

  exahype2::EnclaveTask::releaseTaskNumber(number);

  return storedData;
}


void exahype2::EnclaveBookkeeping::waitForTaskAndDiscardResult(int number) {
  logTraceInWith1Argument("waitForTaskAndDiscardResult(int)", number);

  Entry storedData = waitForTaskToTerminateAndReturnResult(number);
  tarch::freeMemory(storedData.data, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);

  logTraceOut("waitForTaskAndDiscardResult(int)");
}


void exahype2::EnclaveBookkeeping::waitForTaskToTerminateAndCopyResultOver(
  int number, double* destination, double& maxEigenvalue
) {
  logTraceInWith1Argument("waitForTaskToTerminateAndCopyResultOver(int,double*,double&)", number);

  Entry storedData = waitForTaskToTerminateAndReturnResult(number);

  if ( destination!=storedData.data ) {
    std::copy_n(storedData.data, storedData.numberOfResultValues, destination);
    tarch::freeMemory(storedData.data, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
  }
  else {
    logDebug( "waitForTaskToTerminateAndCopyResultOver(int,double*,double&)", "target memory and enclave memory region seem to be the same, so no need to free memory on bookkeeping's side")
  }

  maxEigenvalue = storedData.maxEigenvalue;

  logTraceOutWith2Arguments(
    "waitForTaskToTerminateAndCopyResultOver(int,double*,double&)", maxEigenvalue, storedData.toString()
  );
}


void exahype2::EnclaveBookkeeping::finishedTask(
  int taskNumber, int numberOfResultValues, double* data, double maxEigenvalue
) {
  logDebug("finishedTask()", "task " << taskNumber << " has terminated. Bookkeep results");

  tarch::multicore::Lock lockFinishedTasks(_finishedTasksSemaphore);
  assertionEquals1(_finishedTasks.count(taskNumber), 0, taskNumber);
  if (_tasksThatHaveToBeCancelled.count(taskNumber)) {
    logWarning( "finishedTask(...)", "task entry for task " << taskNumber << " does exist already" );
    _tasksThatHaveToBeCancelled.erase(taskNumber);
    delete[] data;
    lockFinishedTasks.free();
    exahype2::EnclaveTask::releaseTaskNumber(taskNumber);
  } else {
    logDebug( "finishedTask(...)", "bookmarked new outcome for task " << taskNumber << " at memory location " << data );
    auto  oldBucketCount = _finishedTasks.bucket_count();
    Entry newEntry(numberOfResultValues, data, maxEigenvalue);
    _finishedTasks.insert(std::pair<int, Entry>(taskNumber, newEntry));
    if (_finishedTasks.bucket_count() > oldBucketCount) {
      ::tarch::logging::Statistics::getInstance().inc(MemoryAllocationsInLookupTableIdentifier);
    }
    lockFinishedTasks.free();
  }
}


exahype2::EnclaveBookkeeping::Entry::Entry(int numberOfResultValues_, double* data_, double maxEigenvalue_):
  numberOfResultValues(numberOfResultValues_),
  data(data_),
  maxEigenvalue(maxEigenvalue_) {}


std::string exahype2::EnclaveBookkeeping::Entry::toString() const {
  std::ostringstream msg;
  msg << "(#" << numberOfResultValues << ",\\lambda_max=" << maxEigenvalue << ")";
  return msg.str();
}
