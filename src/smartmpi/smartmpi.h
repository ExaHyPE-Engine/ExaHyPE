// This file is part of the SmartMPI project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org. This file will
// later go into a separate project.
#ifndef _SMART_MPI_H_
#define _SMART_MPI_H_


#include <functional>
#include <string>
#include <mpi.h>


#ifndef SmartMPIPrefix
#define SmartMPIPrefix "SmartMPI:\t"
#endif

namespace smartmpi {
  /**
   * Smart MPI Task
   *
   * Smart MPI Tasks are tasks that can be migrated onto another
   * node. For this, it is important that each task type is a unique
   * task number.
   *
   * Tasks in SmartMPI are always pointers to  objects. If 
   */
  class SmartMPITask {
    public:
      const int TypeNumber;
      SmartMPITask( int typeNumber );
      virtual ~SmartMPITask() = default;
      
      virtual std::string toString() const;
      
      /**
       * Execute the task locally. Tasks in SmartMPI are always pointers to 
       * objects. Therefore, you have to tell the SmartMPI runtime whether 
       * it shall delete these objects or whether someone else does it. You
       * might for example forward the 
       * 
       *
       * @return SmartMPI should delete task object eventually
       */
      virtual void runLocally() = 0;
  };

  void spawn(SmartMPITask* task);
  void registerReceiver( std::function<void(int typeNumber)> );
  
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
