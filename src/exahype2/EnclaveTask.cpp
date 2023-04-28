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

#if defined(UseOtter)
  auto &otter_root = otter::Otter::get_otter().get_root_task();
  auto enclave_task = otter_root.make_child(1);
#endif

  computeTask();

  EnclaveBookkeeping::getInstance().finishedTask(getTaskId(),_numberOfResultValues,_outputValues,_maxEigenvalue);

#if defined(UseOtter)
  enclave_task.end_task();
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
