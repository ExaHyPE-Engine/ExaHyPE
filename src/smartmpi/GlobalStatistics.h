// This file is part of the SmartMPI project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org. This file will
// later go into a separate project.
#ifndef _SMARTMPI_GLOBAL_STATISTICS_H_
#define _SMARTMPI_GLOBAL_STATISTICS_H_


#include <chrono>
#include <mpi.h>


namespace smartmpi {
  class GlobalStatistics;
}



class smartmpi::GlobalStatistics {
  private:
    double _accumulatedWaitTimePerNanoSecond;
    int    _numberOfMeasurementsContributingTowardsAccumulation;
    double _waitTimePerNanoSecond;
    
    std::chrono::high_resolution_clock::time_point   _startTime;
    
    MPI_Comm                                         _communicator;
    double*                                          _waitTimesPerNanoSecond;
    MPI_Request*                                     _request;
    
    int                                              _numberOfRanks;
  public:  
    GlobalStatistics();
    ~GlobalStatistics();

    /**
     * This is a delegate from the global signature. See smartmpi.h.
     */
    void reportMPIWaitTime(double time);
    
    void setCommunicator( MPI_Comm communicator );
    
    void gatherWaitTimes();
};

#endif
