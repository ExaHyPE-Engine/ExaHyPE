#include "EnclaveTask.h"

#include "config.h"
#include "EnclaveBookkeeping.h"
#include "tarch/accelerator/Device.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/Lock.h"


tarch::logging::Log exahype2::EnclaveTask::_log("exahype2::EnclaveTask");


tarch::Enumerator   exahype2::EnclaveTask::_enumerator;


int exahype2::EnclaveTask::getNumberOfActiveTasks() { return _enumerator.size(); }


void exahype2::EnclaveTask::releaseTaskNumber(int number) {
  _enumerator.releaseNumber(number);
}


int exahype2::EnclaveTask::reserveTaskNumber() {
  return _enumerator.getNumber();
}


exahype2::EnclaveTask::EnclaveTask(
  int                                         enclaveTaskTypeId,
  const ::peano4::datamanagement::CellMarker& marker,
  double                                      t,
  double                                      dt,
  double* __restrict__                        inputValues,
  double* __restrict__                        outputValues,
  int     numberOfInputValues,
  int     numberOfResultValues,
  Functor functor
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
  logTraceOut("EnclaveTask(...)");
}


bool exahype2::EnclaveTask::run() {
  logTraceInWith1Argument("run()", getTaskId());
  computeTask();
  EnclaveBookkeeping::getInstance().finishedTask(getTaskId(), _numberOfResultValues, _outputValues, _maxEigenvalue);
  logTraceOut("run()");
  return false;
}


void exahype2::EnclaveTask::computeTask() {
  assertion(_numberOfResultValues > 0);
  assertion(_inputValues != nullptr);

  if (_outputValues==nullptr) {
    _outputValues = tarch::allocateMemory<double>(_numberOfResultValues, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
  }
  else {
    logDebug( "computeTask()", "enclave task " << _taskNumber << " writes directly into mesh data structure at " << _outputValues << " (uses data from " << _inputValues << ")" );
  }

  logDebug("computeTask()", "lambda=" << _maxEigenvalue);
  _functor();
}


int exahype2::EnclaveTask::getTaskId() const { return _taskNumber; }
