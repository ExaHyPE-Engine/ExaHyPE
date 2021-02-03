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
    MPI_Comm                                         _communicator;
    
    double*                                          _localWaitTimes;
    double*                                          _waitTimes;
    MPI_Request*                                     _request;
    
    int                                              _numberOfRanks;
    int                                              _rank;
  public:  
    GlobalStatistics();
    ~GlobalStatistics();

    /**
     * This is a delegate from the global signature. See smartmpi.h.
     */
    void reportMPIWaitTime(double time, int rank);
    
    void setCommunicator( MPI_Comm communicator );
    
    void gatherWaitTimes();
};

#endif
