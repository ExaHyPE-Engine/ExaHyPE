// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "EnclaveTask.h"
#include "EnclaveBookkeeping.h"

tarch::logging::Log exahype2::EnclaveTask::_log("exahype2::EnclaveTask");


exahype2::EnclaveTaskNumberPool exahype2::EnclaveTask::_taskNumberPool;


int exahype2::EnclaveTask::getNumberOfActiveTasks() {
  return _taskNumberPool.sizeInUse();
}


void exahype2::EnclaveTask::releaseTaskNumber(int number) {
  _taskNumberPool.release(number);
}


int exahype2::EnclaveTask::reserveTaskNumber() {
  return _taskNumberPool.getNext();
}


exahype2::EnclaveTask::EnclaveTask(
  int                                         enclaveTaskTypeId,
  const peano4::datamanagement::CellMarker&   marker,
  double                                      t,
  double                                      dt,
  double* __restrict__                        inputValues,
  double* __restrict__                        outputValues,
  int                                         numberOfInputValues,
  int                                         numberOfResultValues,
  Functor                                     functor
):
  tarch::multicore::Task(enclaveTaskTypeId, tarch::multicore::Task::DefaultPriority),
  _taskNumber(reserveTaskNumber()),
  _marker(marker),
  _t(t),
  _dt(dt),
  _inputValues(inputValues),
  _outputValues(outputValues),
  _numberOfInputValues(numberOfInputValues),
  _numberOfResultValues(numberOfResultValues),
  _functor(functor),
  _maxEigenvalue(-1.0) {
  logTraceIn("EnclaveTask(...)");

  assertion(_numberOfResultValues > 0);
  assertion(_inputValues != nullptr);

  logTraceOut("EnclaveTask(...)");
}


bool exahype2::EnclaveTask::run() {
  logTraceInWith1Argument("run()", _taskNumber);
  computeTask();
  EnclaveBookkeeping::getInstance().bookmarkFinishedTask(_taskNumber, _numberOfResultValues, _outputValues, _maxEigenvalue);
  logTraceOut("run()");
  return false;
}


void exahype2::EnclaveTask::computeTask() {
  if (!_outputValues) {
    _outputValues = tarch::allocateMemory<double>(_numberOfResultValues, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
  } else {
    logDebug(
      "computeTask(...)",
      "enclave task " << _taskNumber << " writes directly into mesh data structure at " << _outputValues << " (uses data from " << _inputValues << ")"
    );
  }

  _functor();
  logDebug("computeTask()", "task " << _taskNumber << " finished its computation: lambda=" << _maxEigenvalue);
}


int exahype2::EnclaveTask::getTaskId() const {
  return _taskNumber;
}
