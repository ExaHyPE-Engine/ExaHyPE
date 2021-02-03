// This file is part of the SmartMPI project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org. This file will
// later go into a separate project.
#ifndef _SMARTMPI_H_
#define _SMARTMPI_H_


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
   * task number. It is up to the user to implement the SmartMPI tasks,
   * but most systems realise it as a builder mechanism:
   *
   * The tasks is a representative for a real task. Upon the launch
   * of some of its routines, it either migrates data, or it spwans
   * a new task directly into the runtime of choice.
   *
   * Tasks in SmartMPI are always pointers to  objects. SmartMPI will
   * delete them autonomously.
   */
  class SmartMPITask {
    public:
      const int TypeNumber;
      SmartMPITask( int typeNumber );
      virtual ~SmartMPITask() = default;
      
      virtual std::string toString() const;
      
      /**
       * Execute the task locally. 
       *
       * Tasks in SmartMPI are always pointers, and SmartMPI will delete
       * the pointer directly after this routine is called. Therefore, you
       * 
       * - either execute the task straightaway and everything is fine;
       * - or you spawn a real task in the tasking backend of your choice
       *   and hand all required data over to this one.
       *
       * In the latter case, the SmartMPI task becomes an empty hull after
       * the launch (all compute responsibility has been passed on incl all
       * required information) and we can safely destroy it.
       */
      virtual void runLocally() = 0;
      virtual void sendTaskToRank(int rank, int tag, MPI_Comm communicator) = 0;
      virtual void runLocallyAndSendResultToRank(int rank, int tag, MPI_Comm communicator) = 0;
  };

  void spawn(SmartMPITask* task);
  
  /**
   * Creation callback
   * 
   * Has to be provided by user, i.e. via this callback the user code enables
   * SmartMPI to create tasks (knowing the unique SmartMPI task number) upon
   * demand on any rank.
   */
  void registerReceiver( std::function<void(int typeNumber)> );
  
  /**
   * Init SmartMPI
   * 
   * Should be called early throughout the computation. In the ideal case,
   * it is called immediately after an MPI_Init. SmartMPI creates its own
   * subcommunicator. Therefore, it needs a communicator.
   *
   * @param createSubcommunicator Create a new subcommunicator for SmartMPI
   */
  void init(MPI_Comm communicator, bool createSubcommunicator=true);
  void shutdown();
  
  /**
   * Report that some MPI time has been observed
   *
   * I don't really care what time unit you use, but please keep it consistent.
   * That is: if you use seconds at one point, don't use milliseconds in another
   * place.
   */
  void reportMPIWaitTime(double time, int rank);
  
  /**
   * Should be called in (quasi-regular) intervals; like a clock tick. It has to
   * be called on all ranks at the same time. The invocation updates the task 
   * distribution pattern.
   * 
   */
  void tick();
}



#endif
