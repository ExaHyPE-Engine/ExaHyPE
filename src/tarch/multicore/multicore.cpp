#include "multicore.h"
#include "tarch/mpi/Rank.h"

#ifdef UseSmartMPI
#include "smartmpi.h"
#include "topology/topologies.h"
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
  using namespace smartmpi::topology;
  typedef UseSmartMPI MyTopology;
  smartmpi::topology::Topology* smartMPITopology = new MyTopology(
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
  smartmpi::init( smartMPITopology );
  tarch::mpi::Rank::getInstance().setCommunicator( smartMPITopology->computeNodeOrSmartServerCommunicator );
  #endif
}


void tarch::multicore::shutdownSmartMPI() {
  #ifdef UseSmartMPI
  smartmpi::shutdown();
  #endif
}
