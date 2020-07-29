#include "EnclaveGPUTask.h"
#include "EnclaveBookkeeping.h"


#include "tarch/multicore/Core.h"

#include <algorithm>


tarch::logging::Log  exahype2::EnclaveGPUTask::_log( "exahype2::EnclaveGPUTask" );


exahype2::EnclaveGPUTask::EnclaveGPUTask(
  const ::peano4::datamanagement::CellMarker&    marker,
  double*                                        inputValues,
  int                                            numberOfResultValues,
  std::function< void(double* input, double* output, const ::peano4::datamanagement::CellMarker& marker) >                        functor,
  bool                                           inputDataCreatedOnDevice
):
  tarch::multicore::Task(0),
  _marker(marker),
  _inputValues(inputValues),
  _outputValues(nullptr),
  _numberOfResultValues(numberOfResultValues),
  _functor(functor),
  _taskNumber(EnclaveBookkeeping::getInstance().reserveTaskNumber()),
  _inputDataCreatedOnDevice(inputDataCreatedOnDevice) {
  logTraceIn( "EnclaveGPUTask(...)" );

  _outputValues = tarch::multicore::allocateMemoryOnAccelerator(_numberOfResultValues);

  logTraceOut( "EnclaveGPUTask(...)" );
}


int exahype2::EnclaveGPUTask::getTaskNumber() const {
  return _taskNumber;
}


bool exahype2::EnclaveGPUTask::run() {
  logTraceIn( "run()" );

  _functor(_inputValues,_outputValues,_marker);

  if (_inputDataCreatedOnDevice) {
    tarch::multicore::freeMemoryOnAccelerator(_inputValues);
  }
  else {
    delete[] _inputValues;
  }

  double* outputValuesOnHost = new double[_numberOfResultValues];
  std::copy_n( _outputValues, _numberOfResultValues, outputValuesOnHost );
  tarch::multicore::freeMemoryOnAccelerator(_outputValues);

  EnclaveBookkeeping::getInstance().finishedTask(_taskNumber,_numberOfResultValues,outputValuesOnHost);
  logTraceOut( "run()" );
  return false;
}


void exahype2::EnclaveGPUTask::prefetch() {
  logTraceIn( "prefetch()" );
  logTraceOut( "prefetch()" );
}


