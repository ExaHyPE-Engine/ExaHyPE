#include "EnclaveBookkeeping.h"


#include "tarch/multicore/Core.h"

#include <algorithm>
#include "EnclaveOpenMPGPUTask.h"

#include "exahype2/fv/BoundaryConditions.h"
#include "exahype2/fv/Generic.h"
#include "exahype2/fv/Rusanov.h"


tarch::logging::Log  exahype2::EnclaveOpenMPGPUTask::_log( "exahype2::EnclaveOpenMPGPUTask" );


exahype2::EnclaveOpenMPGPUTask::EnclaveOpenMPGPUTask(
  const ::peano4::datamanagement::CellMarker&    marker,
  double*                                        inputValues,
  int                                            numberOfResultValues//,
//  Functor                                        functor
):
  tarch::multicore::Task(tarch::multicore::reserveTaskNumber(),tarch::multicore::Task::DefaultPriority),
  _marker(marker),
  _inputValues(inputValues),
  _outputValues(nullptr),
  _numberOfResultValues(numberOfResultValues)//,
//  _functor(functor) 
{
  logTraceIn( "EnclaveOpenMPGPUTask(...)" );

  _outputValues = tarch::multicore::allocateMemory(_numberOfResultValues,tarch::multicore::MemoryLocation::Accelerator);

  logTraceOut( "EnclaveOpenMPGPUTask(...)" );
}


bool exahype2::EnclaveOpenMPGPUTask::run() {
  logTraceIn( "run()" );

  int*  dependencyMarker = new int;

  //#if defined(GPUOffloading)
  //#pragma omp task depend(out:dependencyMarker)
  //{
//    #if defined(GPUOffloading)
 //   #pragma omp target
 //   #endif
    manual(_inputValues,_outputValues,_marker);
  //#if defined(GPUOffloading)
  //} }

  //#pragma omp task depend(in:dependencyMarker)
  //#endif
  {
    tarch::multicore::freeMemory(_inputValues,tarch::multicore::MemoryLocation::Accelerator);

    double* outputValuesOnHost = tarch::multicore::allocateMemory( _numberOfResultValues, tarch::multicore::MemoryLocation::Heap);
    std::copy_n( _outputValues, _numberOfResultValues, outputValuesOnHost );
    tarch::multicore::freeMemory(_outputValues,tarch::multicore::MemoryLocation::Accelerator);

    EnclaveBookkeeping::getInstance().finishedTask(getTaskId(),_numberOfResultValues,outputValuesOnHost);
  }
  //#if defined(GPUOffloading)
  //#pragma omp taskwait
  //#endif

  delete dependencyMarker;

  logTraceOut( "run()" );
  return false;
}

//#if defined(GPUOffloading)
//#pragma omp declare target
//#endif
void exahype2::EnclaveOpenMPGPUTask::manual(double* reconstructedPatch, double* originalPatch, const ::peano4::datamanagement::CellMarker& marker)
{
     //::exahype2::fv::copyPatch(
       //reconstructedPatch,
       //originalPatch,
       //5,
       //0,
       //101,
       //1
     //);
}
//#pragma omp end declare target



void exahype2::EnclaveOpenMPGPUTask::prefetch() {
  logTraceIn( "prefetch()" );
  logTraceOut( "prefetch()" );
}


