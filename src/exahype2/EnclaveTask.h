// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_ENCLAVE_TASK_H_
#define _EXAHYPE2_ENCLAVE_TASK_H_


#include "../config.h"

#include "peano4/datamanagement/CellMarker.h"
#include "tarch/multicore/Tasks.h"


#include <functional>



#ifdef UseSmartMPI
#include "smartmpi/smartmpi.h"
#endif



namespace exahype2 {
  class EnclaveTask;
  class EnclaveBookkeeping;
}



/**
 * Please study the enclave task in tandem with the EnclaveBookkeeping. Enclave
 * tasks use the bookkeeping to register themselves (obtain a task number). The
 * bookkeeping thus is aware which enclave tasks are outstanding still. Once an
 * enclave task terminates, it registers itself again with the bookkeeping -
 * this time as finished, so anybody can grab the outcome.
 *
 * Enclave tasks per se are application-generic. They are given an array of
 * values, they are told how big the output data has to be, and they are also
 * given a functor which tells them what to run on the outcome. They don't know
 * anything about the functor's semantics.
 *
 * Tasks are automatically destroyed when we make their run() method return false.
 * Enclave tasks however are slightly different: They create their outcome array
 * on the heap. This has to be the CPU heap - not a stack, not an accelerator.
 * When they have computed their stuff, they have to hand over this heap (and the
 * responsibility to destroy it) to the bookkeeping and then can be deleted
 * themselves.
 *
 * <h2> SmartMPI </h2>
 * 
 * If you enable SmartMPI, then all enclave tasks are SmartMPI tasks, too. I 
 * realise this through multiple inheritance. Peano's task interface automatically
 */
class exahype2::EnclaveTask: public tarch::multicore::Task {
  public:
    typedef std::function< void(double* input, double* output, const ::peano4::datamanagement::CellMarker& marker, double t, double dt) >  Functor;
  private:
    friend class EnclaveBookkeeping;

    static tarch::logging::Log                   _log;

    const ::peano4::datamanagement::CellMarker   _marker;
    double*                                      _inputValues;
    double*                                      _outputValues;
    double                                       _t;
    double                                       _dt;
    int                                          _numberOfResultValues;
    Functor                                      _functor;

  public:
    /**
     * Create plain enclave task.
     *
     * @param inputValues        Has to be created on heap via tarch::multicore::allocateMemory().
     * @param enclaveTaskTypeId  Please create it through 
     * <pre>
    static int enclaveTaskTypeId = peano4::parallel::Tasks::getTaskType("{{SOLVER_INSTANCE}}");
</pre>    
     */
    EnclaveTask(
      int                                            enclaveTaskTypeId,
      const ::peano4::datamanagement::CellMarker&    marker,
      double                                         t,
      double                                         dt,
      double*                                        inputValues,
      int                                            numberOfResultValues,
      Functor                                        functor
    );

    EnclaveTask(const EnclaveTask& other) = delete;
    EnclaveTask(const EnclaveTask&& other) = delete;

    virtual ~EnclaveTask() = default;

    bool run() override;
};



#endif
