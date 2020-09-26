#include "EnclaveTask.h"
#include "EnclaveBookkeeping.h"



tarch::logging::Log  exahype2::EnclaveTask::_log( "exahype2::EnclaveTask" );


exahype2::EnclaveTask::EnclaveTask(
  const ::peano4::datamanagement::CellMarker&    marker,
  double*                                        inputValues,
  int                                            numberOfResultValues,
  std::function< void(double* input, double* output, const ::peano4::datamanagement::CellMarker& marker) >                        functor
):
  tarch::multicore::Task(0),
  _marker(marker),
  _inputValues(inputValues),
  _outputValues(nullptr),
  _numberOfResultValues(numberOfResultValues),
  _functor(functor),
  _taskNumber(EnclaveBookkeeping::getInstance().reserveTaskNumber()) {
  logTraceIn( "EnclaveTask(...)" );
  logTraceOut( "EnclaveTask(...)" );
}


int exahype2::EnclaveTask::getTaskNumber() const {
  return _taskNumber;
}


bool exahype2::EnclaveTask::run() {
  logTraceIn( "run()" );
  _outputValues = tarch::multicore::allocateMemory( _numberOfResultValues, tarch::multicore::MemoryLocation::Heap );

  _functor(_inputValues,_outputValues,_marker);
  tarch::multicore::freeMemory(_inputValues,tarch::multicore::MemoryLocation::Heap );

  EnclaveBookkeeping::getInstance().finishedTask(_taskNumber,_numberOfResultValues,_outputValues);
  logTraceOut( "run()" );
  return false;
}


void exahype2::EnclaveTask::prefetch() {
  logTraceIn( "prefetch()" );
  logTraceOut( "prefetch()" );
}


