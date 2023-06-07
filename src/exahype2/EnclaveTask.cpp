#include "config.h"

#if defined(UseOtter)
#define OTTER_TASK_GRAPH_ENABLE_USER
#define INIT_ENCLAVE_TASK_IN_CONSTRUCTOR 1
#define INIT_ENCLAVE_TASK_ON_RUN 2
#define INIT_ENCLAVE INIT_ENCLAVE_TASK_IN_CONSTRUCTOR
#endif

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
#if INIT_ENCLAVE == INIT_ENCLAVE_TASK_IN_CONSTRUCTOR
  OTTER_DECLARE_HANDLE(root_task);
  OTTER_DECLARE_HANDLE(enclave_task);
#if OTTER_ROOT_TASK_EXPLICIT
  OTTER_BORROW_FROM_POOL(root_task, otter::label::root);
#endif
  OTTER_INIT_TASK(enclave_task, root_task, 1, otter_add_to_pool, otter::label::enclave);
#endif
}

bool exahype2::EnclaveTask::run() {
  logTraceInWith1Argument( "run()", getTaskId() );

#if INIT_ENCLAVE == INIT_ENCLAVE_TASK_ON_RUN
  OTTER_DECLARE_HANDLE(root_task);
  OTTER_DECLARE_HANDLE(enclave_task);
#if OTTER_ROOT_TASK_EXPLICIT
  OTTER_BORROW_FROM_POOL(root_task, otter::label::root);
#endif
  OTTER_INIT_TASK(enclave_task, root_task, 1, otter_no_add_to_pool, otter::label::enclave);
#elif INIT_ENCLAVE == INIT_ENCLAVE_TASK_IN_CONSTRUCTOR
  OTTER_DECLARE_HANDLE(enclave_task);
  OTTER_REMOVE_FROM_POOL(enclave_task, otter::label::enclave);
#endif

  OTTER_TASK_START(enclave_task);
  computeTask();
  EnclaveBookkeeping::getInstance().finishedTask(getTaskId(),_numberOfResultValues,_outputValues,_maxEigenvalue);
  OTTER_TASK_END(enclave_task);

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
