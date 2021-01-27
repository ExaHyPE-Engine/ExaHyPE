// This file is part of the SmartMPI project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org. This file will
// later go into a separate project.
#ifndef _SMART_MPI_H_
#define _SMART_MPI_H_


#include <mpi.h>


namespace smartmpi {
  /**
   * Smart MPI Task
   *
   * Smart MPI Tasks are tasks that can be migrated onto another
   * node. For this, it is important that each task type is a unique
   * task number.
   */
  class SmartMPITask {
    public:
      const int TypeNumber;
      SmartMPITask( int typeNumber ):
        TypeNumber(typeNumber) {}
  };

  class Scheduler {
    void scheduleLocally( SmartMPITask* task );
  };

  class Receiver {
      SmartMPITask* receive( int typeNumber );
  };

  void spawn(SmartMPITask* task);
  void registerSchedulerAndReceiver( Scheduler* scheduler, Receiver* receiver );
  
  /**
   * Init SmartMPI
   * 
   * Should be called early throughout the computation. In the ideal case,
   * it is called immediately after an MPI_Init. SmartMPI creates its own
   * subcommunicator. Therefore, it needs a communicator.
   */
  void init(MPI_Comm communicator);
}



#endif
