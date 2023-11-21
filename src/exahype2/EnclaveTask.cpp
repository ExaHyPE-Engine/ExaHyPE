#include "EnclaveTask.h"

#include "config.h"
#include "EnclaveBookkeeping.h"
#include "tarch/accelerator/Device.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/otter.h"


tarch::logging::Log exahype2::EnclaveTask::_log("exahype2::EnclaveTask");


tarch::multicore::BooleanSemaphore exahype2::EnclaveTask::_taskNumberSemaphore;
std::set<int>                      exahype2::EnclaveTask::_activeTaskNumbers;


int exahype2::EnclaveTask::getNumberOfActiveTasks() {
  return _activeTaskNumbers.size();
}


void exahype2::EnclaveTask::releaseTaskNumber(int number) {
  assertion( number>=0 );
  tarch::multicore::Lock taskNumberSemaphore(_taskNumberSemaphore);
  assertionEquals(_activeTaskNumbers.count(number), 1);
  _activeTaskNumbers.erase(number);
}


int exahype2::EnclaveTask::reserveTaskNumber() {
  tarch::multicore::Lock taskNumberSemaphore(_taskNumberSemaphore);
  int                    result = _activeTaskNumbers.size();
  constexpr int Prime = 23;
  constexpr int Max   = std::numeric_limits<int>::max()-2*Prime;

  assertion2( Prime % Max!=0, Prime, Max );

  while (_activeTaskNumbers.count(result) > 0) {
    result = result % Max;
    result += Prime;
  }
  _activeTaskNumbers.insert(result);
  return result;
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
