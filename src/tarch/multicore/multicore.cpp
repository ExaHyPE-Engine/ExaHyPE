#include "multicore.h"


#ifdef UseSmartMPI
#include "smartmpi.h"
#endif


#ifndef SharedMemoryParallelisation

double* tarch::allocateMemory(int size, MemoryLocation location) {
 return new double[size];
}


void tarch::freeMemory(double* data, MemoryLocation location) {
  delete[] data;
}
#endif


void exahype2::initSmartMPI() {
  #ifdef UseSmartMPI
  smartmpi::init( tarch::mpi::Rank::getInstance().getCommunicator() );
  #endif
}


void exahype2::shutdownSmartMPI() {
  #ifdef UseSmartMPI
  smartmpi::shutdown();
  #endif
}
