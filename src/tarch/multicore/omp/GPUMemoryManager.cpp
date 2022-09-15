#include "GPUMemoryManager.h"

#if defined(GPUOffloadingOMP)

tarch::logging::Log  tarch::multicore::omp::GPUMemoryManager::_log( "tarch::multicore::omp::GPUMemoryManager" );


tarch::multicore::omp::GPUMemoryManager& tarch::multicore::omp::GPUMemoryManager::getInstance() {
  static tarch::multicore::omp::GPUMemoryManager singleton;
  return singleton;
}


void*  tarch::multicore::omp::GPUMemoryManager::allocateRawData( int size ) {
// lock!
}


void tarch::multicore::omp::GPUMemoryManager::free(void* devicePointer) {

}

#endif

