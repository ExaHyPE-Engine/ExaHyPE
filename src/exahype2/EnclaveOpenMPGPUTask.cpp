#include "EnclaveBookkeeping.h"


#include "tarch/multicore/Core.h"

#include <algorithm>
#include "EnclaveOpenMPGPUTask.h"

#include "exahype2/fv/BoundaryConditions.h"
#include "exahype2/fv/Generic.h"
#include "exahype2/fv/Rusanov.h"


tarch::logging::Log  exahype2::EnclaveOpenMPGPUTask::_log( "exahype2::EnclaveOpenMPGPUTask" );


// @todo Holger:
//
// Grosser Hack (klingt a bisserl wie ein Trump-Tweet. Am Ende muss eine solche
// Klasse pro Applikation generiert werden
//
// Ich schreibe eigentlich immer alle Doku in den Header in JavaDoc Syntax, so
// dass doxygen es von da aufgabeln kann, aber hier hau ich es mal direkt rein.
// Im Gegensatz zum anderen Enclave Task (den wir evtl. spaeter auch noch
// brauchen) muss ein GPU Task sein Zeugs so frueh wie moeglich issuen, damit
// dann das run eigentlich nur noch das Ergebnis abholt.
//
// Per Konvention liegen input data bereits auf accelerator-passendem Memory (die
// Namensgebung Accelerator heisst ja net, dass es tatsaechlich da schon liegt,
// sondern sagt nur, dass ich CUDA Managed Memory schon anlege, so dass der
// Transfer schneller geht).
//
//
//
//
//
exahype2::EnclaveOpenMPGPUTask::EnclaveOpenMPGPUTask(
  const ::peano4::datamanagement::CellMarker&    marker,
  double*                                        inputValues,
  int                                            numberOfResultValues
):
  tarch::multicore::Task(tarch::multicore::reserveTaskNumber(),tarch::multicore::Task::DefaultPriority),
  _marker(marker),
  _inputValues(inputValues),
  _outputValuesOnDevice(nullptr),
  _outputValuesOnHost(nullptr),
  _numberOfResultValues(numberOfResultValues)
{
  logTraceIn( "EnclaveOpenMPGPUTask(...)" );

  _outputValuesOnDevice = tarch::multicore::allocateMemory(_numberOfResultValues,tarch::multicore::MemoryLocation::Accelerator);

  // Gleich jetzt aufrufen -> Sollte ein unabhaengiger Task sein, der auf der GPU anlaeuft, sobald inputValues drueben ist.
  // Ist aber Zukunftsmusik. Erst mal muss die GPU ueberhaupt verwendet werden
  manual(_inputValues,_outputValuesOnDevice,_marker);

  logTraceOut( "EnclaveOpenMPGPUTask(...)" );
}


bool exahype2::EnclaveOpenMPGPUTask::run() {
  logTraceIn( "run()" );

  // Eigentliche Berechnung sollte/muss jetzt fertig sein (siehe Konstruktur). Jetzt
  // schaufeln wir die Daten zurueck und sind fertig.

  tarch::multicore::freeMemory(_inputValues,tarch::multicore::MemoryLocation::Accelerator );
  _outputValuesOnHost = tarch::multicore::allocateMemory( _numberOfResultValues, tarch::multicore::MemoryLocation::Heap );
  std::copy_n( _outputValuesOnDevice, _numberOfResultValues, _outputValuesOnHost);
  tarch::multicore::freeMemory(_outputValuesOnDevice,tarch::multicore::MemoryLocation::Accelerator );

  EnclaveBookkeeping::getInstance().finishedTask(getTaskId(),_numberOfResultValues,_outputValuesOnHost);


  //_functor(_inputValues,_outputValues,_marker);
  // sollte Accelerator sein
  //int*  dependencyMarker = new int;
  //#if defined(GPUOffloading)
  //#pragma omp task depend(out:dependencyMarker)
  //{
//    #if defined(GPUOffloading)
 //   #pragma omp target
 //   #endif
 
   
   
   
    //#if defined(GPUOffloading)
  //} }

  //#pragma omp task depend(in:dependencyMarker)
  //#endif
  //
  //
  //
  //
  //
/*  {
    tarch::multicore::freeMemory(_inputValues,tarch::multicore::MemoryLocation::Accelerator);

    logWarning( "run()", "size of array=" << _numberOfResultValues );

    double* outputValuesOnHost = tarch::multicore::allocateMemory( _numberOfResultValues, tarch::multicore::MemoryLocation::Heap);
   // std::copy_n( _outputValues, _numberOfResultValues, outputValuesOnHost );
    tarch::multicore::freeMemory(_outputValues,tarch::multicore::MemoryLocation::Accelerator);

    EnclaveBookkeeping::getInstance().finishedTask(getTaskId(),_numberOfResultValues,outputValuesOnHost);
  }
*/



  //#if defined(GPUOffloading)
  //#pragma omp taskwait
  //#endif

  //delete dependencyMarker;

  logTraceOut( "run()" );
  return false;
}

//#if defined(GPUOffloading)
//#pragma omp declare target
//#endif
void exahype2::EnclaveOpenMPGPUTask::manual(double* reconstructedPatch, double* originalPatch, const ::peano4::datamanagement::CellMarker& marker)
{
/*
  ::exahype2::fv::copyPatch(
    reconstructedPatch,
    originalPatch,
    5,
    0,
    101,
    1
  );
*/
}
//#pragma omp end declare target



void exahype2::EnclaveOpenMPGPUTask::prefetch() {
  logTraceIn( "prefetch()" );
  logTraceOut( "prefetch()" );
}


