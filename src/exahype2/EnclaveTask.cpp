#include "EnclaveTask.h"

#include "config.h"
#include "EnclaveBookkeeping.h"
#include "tarch/accelerator/Device.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/otter.h"


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
  double* __restrict__ inputValues,
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
  _outputValues(nullptr),
  _numberOfInputValues(numberOfInputValues),
  _numberOfResultValues(numberOfResultValues),
  _functor(functor),
  _maxEigenvalue(-1.0) {
  logTraceIn("EnclaveTask(...)");
  logTraceOut("EnclaveTask(...)");
  OTTER_DEFINE_TASK(
    enclave_task, OTTER_NULL_TASK, otter_add_to_pool, otter::label::enclave_with_typeid, enclaveTaskTypeId
  );
}


bool exahype2::EnclaveTask::run() {
  logTraceInWith1Argument("run()", getTaskId());

  OTTER_POOL_DECL_POP(enclave_task, otter::label::enclave_with_typeid, getTaskId());

  OTTER_TASK_START(enclave_task);
  computeTask();
  EnclaveBookkeeping::getInstance().finishedTask(getTaskId(), _numberOfResultValues, _outputValues, _maxEigenvalue);
  OTTER_TASK_END(enclave_task);

  logTraceOut("run()");
  return false;
}


void exahype2::EnclaveTask::computeTask() {
  assertion(_outputValues == nullptr);
  assertion(_numberOfResultValues > 0);
  assertion(_inputValues != nullptr);

  _outputValues = tarch::allocateMemory<double>(_numberOfResultValues, tarch::MemoryLocation::Heap);

  logDebug("computeTask()", "lambda=" << _maxEigenvalue);
  _functor();
}


int exahype2::EnclaveTask::getTaskId() const { return _taskNumber; }
