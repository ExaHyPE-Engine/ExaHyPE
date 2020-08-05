// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_ENCLAVE_TASK_H_
#define _EXAHYPE2_ENCLAVE_TASK_H_


#include "peano4/datamanagement/CellMarker.h"
#include "tarch/multicore/Tasks.h"


#include <functional>


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
 */
class exahype2::EnclaveTask: public tarch::multicore::Task {
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
     * Create plain enclave task.
     */
    EnclaveTask(
      const ::peano4::datamanagement::CellMarker&    marker,
      double*                                        inputValues,
      int                                            numberOfResultValues,
      std::function< void(double* input, double* output, const ::peano4::datamanagement::CellMarker& marker) >                        functor
    );

    EnclaveTask(const EnclaveTask& other) = delete;
    EnclaveTask(const EnclaveTask&& other) = delete;

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
