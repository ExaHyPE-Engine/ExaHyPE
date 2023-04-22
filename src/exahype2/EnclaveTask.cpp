#include "EnclaveTask.h"
#include "EnclaveBookkeeping.h"

#include "tarch/accelerator/Device.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/otter.h"


tarch::logging::Log  exahype2::EnclaveTask::_log( "exahype2::EnclaveTask" );


exahype2::EnclaveTask::EnclaveTask(
  int                                            enclaveTaskTypeId,
  const ::peano4::datamanagement::CellMarker&    marker,
  double                                         t,
  double                                         dt,
  double* __restrict__                           inputValues,
  int                                            numberOfInputValues,
  int                                            numberOfResultValues,
  Functor                                        functor
):
  tarch::multicore::Task(tarch::multicore::reserveTaskNumber(),enclaveTaskTypeId,tarch::multicore::Task::DefaultPriority),
  _marker(marker),
  _t(t),
  _dt(dt),
  _inputValues(inputValues),
  _outputValues(nullptr),
  _numberOfInputValues(numberOfInputValues),
  _numberOfResultValues(numberOfResultValues),
  _functor(functor),
  _maxEigenvalue( -1.0 ) {
  logTraceIn( "EnclaveTask(...)" );
  logTraceOut( "EnclaveTask(...)" );
}

bool exahype2::EnclaveTask::run() {
  logTraceInWith1Argument( "run()", getTaskId() );

#if defined(UseOtterCPP)
  auto &otter_root = otter::Otter::get_otter().get_root_task();
  auto task = otter_root.make_child(1); // TODO (AT): should enclave tasks be considered children of the traversal task which spawns them, but with no synchronisation constraint?
#else
  otter_task_context *task = otterTaskBegin_flavour(OTTER_SRC_ARGS(), NULL, 1); // TODO (AT): replace with C++ api. These are children of the traversal tasks
#endif

  computeTask();

  EnclaveBookkeeping::getInstance().finishedTask(getTaskId(),_numberOfResultValues,_outputValues,_maxEigenvalue);

#if defined(UseOtterCPP)
  task.end_task();
#else
  otterTaskEnd(task); // TODO (AT): replace with C++ api
#endif

  logTraceOut( "run()" );
  return false;
}

void exahype2::EnclaveTask::computeTask() {
  assertion( _outputValues==nullptr );
  assertion( _numberOfResultValues>0 );
  assertion( _inputValues!=nullptr );

  _outputValues = tarch::allocateMemory<double>( _numberOfResultValues, tarch::MemoryLocation::Heap );

  logDebug( "computeTask()", "lambda=" << _maxEigenvalue );
  _functor();
}
