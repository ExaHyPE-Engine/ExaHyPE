// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_ENCLAVE_GPU_TASK_H_
#define _EXAHYPE2_ENCLAVE_GPU_TASK_H_


#include "peano4/datamanagement/CellMarker.h"
#include "tarch/multicore/Tasks.h"


#include <functional>


namespace exahype2 {
  class EnclaveGPUTask;
  class EnclaveBookkeeping;
}



/**
 * Please study the enclave task in tandem with the EnclaveBookkeeping. Enclave
 * tasks use the bookkeeping to register themselves (obtain a task number). The
 * bookkeeping thus is aware which enclave tasks are outstanding still. Once an
 * enclave task terminates, it registers itself again with the bookkeeping -
 * this time as finished, so anybody can grab the outcome.
 *
 * Enclave tasks per se are application generic. They are given an array of
 * values, they are told how big the output data has to be, and they are also
 * given a functor which tells them what to run on the outcome. They don't know
 * anything about the functor's semantics.
 *
 * <h2> Object lifecycle </h2>
 *
 * Tasks are automatically destroyed when we make their run() method return false.
 * As a consequence, I cannot simply take the task, run it, and then store the
 * task object into the enclave bookkeeping. I can only store the created output
 * memory, but will loose all of my task-specific data.
 */
class exahype2::EnclaveGPUTask: public tarch::multicore::Task {
  private:
    friend class EnclaveBookkeeping;

    static tarch::logging::Log                   _log;

    const ::peano4::datamanagement::CellMarker   _marker;
    double*                                      _inputValues;
    double*                                      _outputValues;
    int                                          _numberOfResultValues;
    std::function< void(double* input, double* output, const ::peano4::datamanagement::CellMarker& marker) >                      _functor;
    const int                                    _taskNumber;

  public:
    /**
     * Construct a task that can run on the GPU
     *
     * But it doesn't have to to run there. It is up to the task to decide whether
     * to offload or not.
     */
    EnclaveGPUTask(
      const ::peano4::datamanagement::CellMarker&    marker,
      double*                                        inputValues,
      int                                            numberOfResultValues,
      std::function< void(double* input, double* output, const ::peano4::datamanagement::CellMarker& marker) >                        functor
    );

    EnclaveGPUTask(const EnclaveGPUTask& other) = delete;
    EnclaveGPUTask(const EnclaveGPUTask&& other) = delete;

    /**
     * Every task grabs a unique task number. You will want to memorise this
     * task number somewhere so you can later poll the enclave task
     * bookkeeping whether a task is already completed.
     *
     * ::exahype2::EnclaveBookkeeping::getInstance().reserveTaskNumber();
     */
    int getTaskNumber() const;

    bool run() override;

    /**
     * nop
     */
    void prefetch() override;
};



#endif
