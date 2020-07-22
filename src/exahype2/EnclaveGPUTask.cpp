#include "EnclaveGPUTask.h"
#include "EnclaveBookkeeping.h"



tarch::logging::Log  exahype2::EnclaveGPUTask::_log( "exahype2::EnclaveGPUTask" );


exahype2::EnclaveGPUTask::EnclaveGPUTask(
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
  logTraceIn( "EnclaveGPUTask(...)" );
  logTraceOut( "EnclaveGPUTask(...)" );
}


int exahype2::EnclaveGPUTask::getTaskNumber() const {
  return _taskNumber;
}


bool exahype2::EnclaveGPUTask::run() {
  logTraceIn( "run()" );
  _outputValues = new double[_numberOfResultValues];
  _functor(_inputValues,_outputValues,_marker);
  delete[] _inputValues;

  EnclaveBookkeeping::getInstance().finishedTask(_taskNumber,_numberOfResultValues,_outputValues);
  logTraceOut( "run()" );
  return false;
}


void exahype2::EnclaveGPUTask::prefetch() {
  logTraceIn( "prefetch()" );
  logTraceOut( "prefetch()" );
}


