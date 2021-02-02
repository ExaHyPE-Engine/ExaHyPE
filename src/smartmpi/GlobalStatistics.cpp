#include "GlobalStatistics.h"
#include "smartmpi.h"

#include <cassert>
#include <iostream>


smartmpi::GlobalStatistics::GlobalStatistics():
  _communicator( MPI_COMM_WORLD ),
  _localWaitTimes( nullptr ),
  _waitTimes( nullptr ),
  _request(nullptr),
  _numberOfRanks(-1),
  _rank(-1) {
}


smartmpi::GlobalStatistics::~GlobalStatistics() {
  if (_localWaitTimes!=nullptr) {
    delete[] _localWaitTimes;
    delete[] _waitTimes;
    _localWaitTimes = nullptr;
    _waitTimes      = nullptr;
  }
}


void smartmpi::GlobalStatistics::reportMPIWaitTime(double time, int rank) {
  assert( _localWaitTimes != nullptr );
  _localWaitTimes[ _rank ] += time;
  _localWaitTimes[ _rank ] /= 2.0;
}


void smartmpi::GlobalStatistics::setCommunicator( MPI_Comm communicator ) {
  MPI_Comm_size(communicator, &_numberOfRanks);
  MPI_Comm_rank(_communicator, &_rank);

  _localWaitTimes = new double[_numberOfRanks];
  _waitTimes      = new double[_numberOfRanks*_numberOfRanks];
}


void smartmpi::GlobalStatistics::gatherWaitTimes() {
  assert( _waitTimes!=nullptr );
  assert( _localWaitTimes!=nullptr );

  if (_request!=nullptr) {
    MPI_Wait( _request, MPI_STATUS_IGNORE );
    delete _request;
  }

  _request = new MPI_Request();

  MPI_Iallgather(
    &_localWaitTimes,
    _numberOfRanks,
    MPI_DOUBLE,
    _waitTimes,
    _numberOfRanks,
    MPI_DOUBLE,
    _communicator,
    _request
  );

  #if SmartMPIDebug>=1
  for (int row=0; row<_numberOfRanks; row++) {
    std::cout << SmartMPIPrefix << "rank " << _rank << ":";
    for (int col=0; col<_numberOfRanks; col++) {
      std::cout << " " << _waitTimes[row*_numberOfRanks + col];
    }
    std::cout << std::endl;
  }
  #endif
}
