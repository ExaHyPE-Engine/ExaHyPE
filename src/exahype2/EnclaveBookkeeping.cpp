// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "EnclaveBookkeeping.h"
#include "EnclaveTask.h"

#include "tarch/Assertions.h"
#include "tarch/logging/Statistics.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/Task.h"
#include "tarch/multicore/Lock.h"

#include <algorithm>


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


std::size_t exahype2::EnclaveBookkeeping::getNumberOfFinishedTasks() const {
  return _finishedTasks.size();
}


void exahype2::EnclaveBookkeeping::cancelTask(int taskNumber) {
  tarch::multicore::Lock finishedTasksLock(_finishedTasksSemaphore);

  if (_finishedTasks.count(taskNumber) > 0) {
    Entry storedData = _finishedTasks.at(taskNumber);
    _finishedTasks.erase(taskNumber);
    tarch::freeMemory(storedData.data, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
    exahype2::EnclaveTask::releaseTaskNumber(taskNumber);
  } else {
    _tasksThatHaveToBeCancelled.insert(taskNumber);
  }
}


exahype2::EnclaveBookkeeping::Entry exahype2::EnclaveBookkeeping::waitForTaskToTerminateAndReturnResult(int taskNumber) {
  tarch::multicore::waitForTask(taskNumber);

  tarch::multicore::Lock finishedTasksLock(_finishedTasksSemaphore);
  logDebug(
    "waitForTaskToTerminateAndReturnResult(int)",
    "release task " << taskNumber <<
    ", number of finished tasks: " << _finishedTasks.size() <<
    ", number of active tasks: " << exahype2::EnclaveTask::getNumberOfActiveTasks()
  );

  if (_finishedTasks.count(taskNumber) != 1) {
    logWarning(
      "waitForTaskToTerminateAndReturnResult(int)",
      "trying to fetch result that has already been fetched. Returning without solution."
    );
    return Entry(0, nullptr, -1);
  }

  assertionEquals3(_finishedTasks.count(taskNumber), 1, taskNumber, _finishedTasks.count(taskNumber), exahype2::EnclaveTask::getNumberOfActiveTasks());
  Entry storedData = _finishedTasks.at(taskNumber);
  _finishedTasks.erase(taskNumber);
  finishedTasksLock.free();

  exahype2::EnclaveTask::releaseTaskNumber(taskNumber);

  return storedData;
}


void exahype2::EnclaveBookkeeping::waitForTaskAndDiscardResult(int taskNumber) {
  logTraceInWith1Argument("waitForTaskAndDiscardResult(int)", taskNumber);

  Entry storedData = waitForTaskToTerminateAndReturnResult(taskNumber);
  // TODO: Is the freeMemory below dangerous if we handle target memory and enclave memory regions as the same?
  tarch::freeMemory(storedData.data, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);

  logTraceOut("waitForTaskAndDiscardResult(int)");
}


void exahype2::EnclaveBookkeeping::waitForTaskToTerminateAndCopyResultOver(
  int taskNumber, double* destination, double& maxEigenvalue
) {
  logTraceInWith1Argument("waitForTaskToTerminateAndCopyResultOver(int,double*,double&)", taskNumber);

  Entry storedData = waitForTaskToTerminateAndReturnResult(taskNumber);
  if (not storedData.data) return;

  if (destination != storedData.data) {
    //#pragma omp taskwait depend(in: storedData.data)
    std::copy_n(storedData.data, storedData.numberOfResultValues, destination);
    //tarch::accelerator::MemoryManager(storedData.data, 0);
    tarch::freeMemory(storedData.data, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
  } else {
    logDebug(
      "waitForTaskToTerminateAndCopyResultOver(int,double*,double&)",
      "target memory and enclave memory region seem to be the same, so no need to free memory on bookkeeping's side"
    )
  }

  maxEigenvalue = storedData.maxEigenvalue;

  logTraceOutWith2Arguments(
    "waitForTaskToTerminateAndCopyResultOver(int,double*,double&)", maxEigenvalue, storedData.toString()
  );
}


void exahype2::EnclaveBookkeeping::bookmarkFinishedTask(
  int taskNumber, int numberOfResultValues, double* data, double maxEigenvalue
) {
  logDebug("bookmarkFinishedTask(...)", "task " << taskNumber << " has terminated or will be terminated soon. Bookkeep results");

  tarch::multicore::Lock finishedTasksLock(_finishedTasksSemaphore);
  assertionEquals1(_finishedTasks.count(taskNumber), 0, taskNumber);
  if (_tasksThatHaveToBeCancelled.count(taskNumber)) {
    logWarning("bookmarkFinishedTask(...)", "task entry for task " << taskNumber << " does exist already");
    _tasksThatHaveToBeCancelled.erase(taskNumber);
    tarch::freeMemory(data, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
    //delete[] data;
    finishedTasksLock.free();
    exahype2::EnclaveTask::releaseTaskNumber(taskNumber);
  } else {
    logDebug(
      "bookmarkFinishedTask(...)",
      "bookmarked new outcome for task " << taskNumber << " at memory location " << data <<
      ", number of finished tasks: " << _finishedTasks.size() <<
      ", number of active tasks: " << exahype2::EnclaveTask::getNumberOfActiveTasks()
    );
    auto  oldBucketCount = _finishedTasks.bucket_count();
    Entry newEntry(numberOfResultValues, data, maxEigenvalue);
    _finishedTasks.insert(std::pair<int, Entry>(taskNumber, newEntry));
    if (_finishedTasks.bucket_count() > oldBucketCount) {
      tarch::logging::Statistics::getInstance().inc(MemoryAllocationsInLookupTableIdentifier);
    }
    finishedTasksLock.free();
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
