// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "../config.h"


namespace exahype2 {  
  /**
   * Switch on SmartMPI
   * 
   * If you use SmartMPI, then the bookkeeping registers the the local scheduling.
   * If you don't use SmartMPI, this operation becomes nop, i.e. you can always 
   * call it and configure will decide whether it does something useful.
   */
  void initSmartMPIForEnclaveTasks();
}


#if !defined(_EXAHYPE2_SMART_ENCLAVE_TASK_H_) and defined(UseSmartMPI)
#define _EXAHYPE2_SMART_ENCLAVE_TASK_H_




#include "smartmpi/smartmpi.h"

#include "tarch/logging/Log.h"

#include "peano4/datamanagement/CellMarker.h"

#include "EnclaveTask.h"


namespace exahype2 {
  class SmartEnclaveTask;
  class EnclaveBookkeeping;
  
  /**
   * Switch on SmartMPI
   * 
   * If you use SmartMPI, then the bookkeeping registers the the local scheduling.
   * If you don't use SmartMPI, this operation becomes nop, i.e. you can always 
   * call it and configure will decide whether it does something useful.
   *
   * The event handler is a little bit of a hack: We know that all the tasks we 
   * hand over to SmartMPI are enclave tasks. Therefore, we 
   * brutally cast the task to this type and hand it over to the job scheduling.
   * A nicer way would be to ask which type of tasking backend we use (TBB vs 
   * OpenMP, e.g.) and then to use something target-specific. But this way, I 
   * have the full beauty of the backend-independent tasking.
   */
  void initSmartMPIForSmartEnclaveTasks();
}



/**
 * Smart wrapper around an enclave task
 *
 * 
 */
class exahype2::SmartEnclaveTask: public smartmpi::SmartMPITask {
  private:
    friend class EnclaveBookkeeping;

    static tarch::logging::Log                   _log;

    EnclaveTask*                                 _wrappedTask;

  public:
    /**
     * Create plain enclave task.
     *
     * @param inputValues Has to be created on heap via tarch::multicore::allocateMemory().
     */
    SmartEnclaveTask(
      const ::peano4::datamanagement::CellMarker&    marker,
      double*                                        inputValues,
      int                                            numberOfResultValues,
      std::function< void(double* input, double* output, const ::peano4::datamanagement::CellMarker& marker) >   functor,
      int                                            solverNumber
    );

    SmartEnclaveTask(const SmartEnclaveTask& other) = delete;
    SmartEnclaveTask(const SmartEnclaveTask&& other) = delete;

    ~SmartEnclaveTask() = default;

    void runLocally() override;
    void sendTaskToRank(int rank, int tag, MPI_Comm communicator) override;
    void runLocallyAndSendResultToRank(int rank, int tag, MPI_Comm communicator) override;

    int getTaskId() const;
};



#endif
