#include "multicore.h"
#include "tarch/mpi/Rank.h"

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


void tarch::multicore::initSmartMPI() {
  #ifdef UseSmartMPI
  smartmpi::init( tarch::mpi::Rank::getInstance().getCommunicator() );
  #endif
}


void tarch::multicore::shutdownSmartMPI() {
  #ifdef UseSmartMPI
  smartmpi::shutdown();
  #endif
}
