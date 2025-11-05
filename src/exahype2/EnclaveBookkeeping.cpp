#include "EnclaveBookkeeping.h"
#include "EnclaveTask.h"

#include "tarch/Assertions.h"
#include "tarch/NonCriticalAssertions.h"
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
  tarch::multicore::Lock finishedTasksLock(_finishedTasksSemaphore);
  logInfo("dumpStatistics()", "active tasks=" << exahype2::EnclaveTask::getNumberOfActiveTasks());

  std::ostringstream finishedTasksMsg;
  finishedTasksMsg << "(#" << _finishedTasks.size();
  for (auto& p : _finishedTasks) {
    finishedTasksMsg << "," << p.first << "->" << p.second.toString();
  }
  finishedTasksMsg << ")";
  logInfo("dumpStatistics()", "finished tasks=" << finishedTasksMsg.str());
}


exahype2::EnclaveBookkeeping::Entry exahype2::EnclaveBookkeeping::waitForTaskToTerminateAndReturnResult(
  int number,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h
) {
  logDebug("waitForTaskToTerminateAndReturnResult(int)", "wait for task " << number << " at " << x << "x" << h );
  tarch::multicore::waitForTask(number);

  tarch::multicore::Lock finishedTasksLock(_finishedTasksSemaphore);

  assertionEquals1( _finishedTasks.count(number), 1, number );

  #if PeanoDebug>0
  assertion3( _taskToMarkerMapping.count(number)>0, number, x, h );
  assertionEquals3( _taskToMarkerMapping.at(number).x(), x, number, x, h );
  assertionEquals3( _taskToMarkerMapping.at(number).h(), h, number, x, h );

  if (
    _taskToMarkerMapping.count(number)==0
    or
    _taskToMarkerMapping.at(number).x()!=x
    or
    _taskToMarkerMapping.at(number).h()!=h
  ) {
    tarch::triggerNonCriticalAssertion( __FILE__, __LINE__, "_taskToMarkerMapping.count(number)>0", "(" + std::to_string(number) + "," + toString(x) + "," + toString(h) + ")" );
  }
  _taskToMarkerMapping.erase(number);
  #endif

  Entry storedData = _finishedTasks.at(number);
  _finishedTasks.erase(number);
  finishedTasksLock.free();
  logDebug("waitForTaskToTerminateAndReturnResult(int)", "deleted data of task " << number );

  exahype2::EnclaveTask::releaseTaskNumber(number);

  return storedData;
}


void exahype2::EnclaveBookkeeping::waitForTaskAndDiscardResult(
  int number,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h
) {
  logTraceInWith1Argument("waitForTaskAndDiscardResult(int)", number);

  Entry storedData = waitForTaskToTerminateAndReturnResult(number,x,h);
  tarch::freeMemory(storedData.data, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);

  logTraceOut("waitForTaskAndDiscardResult(int)");
}


void exahype2::EnclaveBookkeeping::waitForTaskToTerminateAndCopyResultOver(
  int number,
  double* destination,
  double& maxEigenvalue,
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h
) {
  logTraceInWith1Argument("waitForTaskToTerminateAndCopyResultOver(int,double*,double&)", number);

  Entry storedData = waitForTaskToTerminateAndReturnResult(number,x,h);

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
  int taskNumber,
  int numberOfResultValues,
  double* data,
  double maxEigenvalue,
  const tarch::la::Vector<Dimensions, double>& markerX,
  const tarch::la::Vector<Dimensions, double>& markerH
) {
  tarch::multicore::Lock lockFinishedTasks(_finishedTasksSemaphore);
  assertionEquals1(_finishedTasks.count(taskNumber), 0, taskNumber);

  #if PeanoDebug>0
  assertion3( _taskToMarkerMapping.count(taskNumber)>0, taskNumber, markerX, markerH );
  assertionEquals3( _taskToMarkerMapping.at(taskNumber).x(), markerX, taskNumber, markerX, markerH );
  assertionEquals3( _taskToMarkerMapping.at(taskNumber).h(), markerH, taskNumber, markerX, markerH );

  if (
    _taskToMarkerMapping.count(taskNumber)==0
    or
    _taskToMarkerMapping.at(taskNumber).x()!=markerX
    or
    _taskToMarkerMapping.at(taskNumber).h()!=markerH
  ) {
    tarch::triggerNonCriticalAssertion( __FILE__, __LINE__, "_taskToMarkerMapping.count(number)>0", "(" + std::to_string(taskNumber) + "," + toString(markerX) + "," + toString(markerH) + ")" );
  }
  #endif

  if (_tasksThatHaveToBeCancelled.count(taskNumber)>0) {
    _tasksThatHaveToBeCancelled.erase(taskNumber);
    delete[] data;
    lockFinishedTasks.free();
    logDebug( "finishedTask(...)", "task " << taskNumber << " is marked as to-be-deleted" );
    exahype2::EnclaveTask::releaseTaskNumber(taskNumber);
  }
  else {
    logDebug( "finishedTask(...)", "bookmarked outcome for task " << taskNumber << " at memory location " << data << " (" + std::to_string(taskNumber) + "," + toString(markerX) + "," + toString(markerH) + ")");
    auto  oldBucketCount = _finishedTasks.bucket_count();
    Entry newEntry(numberOfResultValues, data, maxEigenvalue);
    _finishedTasks.insert(std::pair<int, Entry>(taskNumber, newEntry));
    if (_finishedTasks.bucket_count() > oldBucketCount) {
      ::tarch::logging::Statistics::getInstance().inc(MemoryAllocationsInLookupTableIdentifier);
    }
  }
}



void exahype2::EnclaveBookkeeping::registerTask( int taskNumber, const ::peano4::datamanagement::CellMarker& marker ) {
  tarch::multicore::Lock finishedTasksLock(_finishedTasksSemaphore);

  #if PeanoDebug>0
  assertion2(_taskToMarkerMapping.count(taskNumber)==0, taskNumber, marker.toString() );

  if (_taskToMarkerMapping.count(taskNumber)!=0) {
    tarch::triggerNonCriticalAssertion( "__FILE__", __LINE__, "_taskToMarkerMapping.count(taskNumber)==0", marker.toString() );
  }

  _taskToMarkerMapping.insert( std::pair<int, ::peano4::datamanagement::CellMarker >(taskNumber, marker) );
  #endif
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

