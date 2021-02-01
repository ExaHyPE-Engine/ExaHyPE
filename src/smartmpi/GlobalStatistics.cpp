#include "GlobalStatistics.h"
#include "smartmpi.h"

#include <cassert>
#include <iostream>


smartmpi::GlobalStatistics::GlobalStatistics():
  _accumulatedWaitTimePerNanoSecond(0.0),
  _numberOfMeasurementsContributingTowardsAccumulation(0),
  _startTime( std::chrono::high_resolution_clock::now() ),
  _waitTimesPerNanoSecond(nullptr),
  _request(nullptr) {
}


smartmpi::GlobalStatistics::~GlobalStatistics() {
  if (_waitTimesPerNanoSecond!=nullptr) {
    delete[] _waitTimesPerNanoSecond;
    _waitTimesPerNanoSecond = nullptr;
  }
}


void smartmpi::GlobalStatistics::reportMPIWaitTime(double time) {
  std::chrono::high_resolution_clock::time_point timeStamp = std::chrono::high_resolution_clock::now();
  double elapsedTime = std::chrono::duration_cast<std::chrono::nanoseconds>(timeStamp - _startTime).count();
  _startTime = timeStamp;

  _numberOfMeasurementsContributingTowardsAccumulation++;
  _accumulatedWaitTimePerNanoSecond += time / elapsedTime;

  _waitTimePerNanoSecond = _accumulatedWaitTimePerNanoSecond / static_cast<double>(_numberOfMeasurementsContributingTowardsAccumulation);
}


void smartmpi::GlobalStatistics::setCommunicator( MPI_Comm communicator ) {
  MPI_Comm_size(communicator, &_numberOfRanks);
  _waitTimesPerNanoSecond = new double[_numberOfRanks];
}


void smartmpi::GlobalStatistics::gatherWaitTimes() {
  assert( _waitTimesPerNanoSecond!=nullptr );

  if (_request!=nullptr) {
    MPI_Wait( _request, MPI_STATUS_IGNORE );
    delete _request;
  }

  _request = new MPI_Request();

  MPI_Iallgather(
    &_waitTimePerNanoSecond,
    1,
    MPI_DOUBLE,
    _waitTimesPerNanoSecond,
    1,
    MPI_DOUBLE,
    _communicator,
    _request
  );

  // #if SmartMPIDebug>=4
  int rank;
  MPI_Comm_rank(_communicator, &rank);

  std::cout << SmartMPIPrefix << "rank " << rank << ":";
  for (int i=0; i<_numberOfRanks; i++)
    std::cout << " " << _waitTimesPerNanoSecond[i];
  std::cout << std::endl;
  // #endif
}
