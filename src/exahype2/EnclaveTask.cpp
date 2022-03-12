#include "EnclaveTask.h"
#include "EnclaveBookkeeping.h"

#include "tarch/multicore/Core.h"


tarch::logging::Log  exahype2::EnclaveTask::_log( "exahype2::EnclaveTask" );


exahype2::EnclaveTask::EnclaveTask(
  int                                            enclaveTaskTypeId,
  const ::peano4::datamanagement::CellMarker&    marker,
  double                                         t,
  double                                         dt,
  double*                                        inputValues,
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

  assertion( _outputValues==nullptr );
  assertion( _numberOfResultValues>0 );
  assertion( _inputValues!=nullptr );

  _outputValues = tarch::allocateMemory( _numberOfResultValues, tarch::MemoryLocation::Heap );
  _functor();
  //tarch::freeMemory(_inputValues,tarch::MemoryLocation::Heap );

#ifndef UseSmartMPI // smarTeaMPI must bookmark this differently since getTaskId() does not return the right ID for a moved task. It does so in runLocally()
  EnclaveBookkeeping::getInstance().finishedTask(getTaskId(),_numberOfResultValues,_outputValues,_maxEigenvalue);
#endif
  logTraceOut( "run()" );
  return false;
}

