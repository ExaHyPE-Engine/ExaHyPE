#include "multicore.h"
#include "tarch/mpi/Rank.h"
#include "tarch/compiler/CompilerSpecificSettings.h"

#ifdef UseSmartMPI
#include "smartmpi.h"
#include "topology/topologies.h"
#endif

#ifndef SharedMemoryParallelisation

double* tarch::allocateMemory(int size, MemoryLocation location) {
 return new (std::align_val_t(AlignmentOnHeap)) double[size];
}


void tarch::freeMemory(double* data, MemoryLocation location) {
  delete[] data;
}
#endif


void tarch::multicore::initSmartMPI() {
  #ifdef UseSmartMPI
  using namespace smartmpi::topology;
  typedef UseSmartMPI MyTopology;
  smartmpi::topology::Topology* smartMPITopology = new MyTopology(
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
  smartmpi::init( smartMPITopology );

  smartmpi::appendToSchedulerChain( "HardcodedMigrationScheduler" );

  tarch::mpi::Rank::getInstance().setCommunicator( smartMPITopology->computeNodeOrSmartServerCommunicator );
  #endif
}


void tarch::multicore::shutdownSmartMPI() {
  #ifdef UseSmartMPI
  smartmpi::shutdown();
  #endif
}


#if defined(GPUOffloadingOMP)

#pragma omp declare target
void tarch::gpuAbort() {
}
#pragma omp end declare target

#elif defined(GPUOffloadingSYCL)

GPUCallableMethod void tarch::gpuAbort() {
}

#elif defined(GPUOffloadingCPP)

__host__ __device__ void tarch::gpuAbort(){
  std::abort();
};

#else
void tarch::gpuAbort() {
  std::abort();
}
#endif
