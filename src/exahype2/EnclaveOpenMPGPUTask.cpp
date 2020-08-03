#include "EnclaveBookkeeping.h"


#include "tarch/multicore/Core.h"

#include <algorithm>
#include "EnclaveOpenMPGPUTask.h"


tarch::logging::Log  exahype2::EnclaveOpenMPGPUTask::_log( "exahype2::EnclaveOpenMPGPUTask" );


exahype2::EnclaveOpenMPGPUTask::EnclaveOpenMPGPUTask(
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
  logTraceIn( "EnclaveOpenMPGPUTask(...)" );

  _outputValues = tarch::multicore::allocateMemory(_numberOfResultValues,tarch::multicore::MemoryLocation::Accelerator);

  logTraceOut( "EnclaveOpenMPGPUTask(...)" );
}


int exahype2::EnclaveOpenMPGPUTask::getTaskNumber() const {
  return _taskNumber;
}


bool exahype2::EnclaveOpenMPGPUTask::run() {
  logTraceIn( "run()" );

  int*  dependencyMarker = new int;

  _functor(_inputValues,_outputValues,_marker);

  #if defined(GPUOffloading)
  #pragma omp task depend(in:dependencyMarker)
  #endif
  {
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
  }
  #if defined(GPUOffloading)
  #pragma omp taskwait
  #endif

  delete dependencyMarker;

  logTraceOut( "run()" );
  return false;
}


void exahype2::EnclaveOpenMPGPUTask::prefetch() {
  logTraceIn( "prefetch()" );
  logTraceOut( "prefetch()" );
}


