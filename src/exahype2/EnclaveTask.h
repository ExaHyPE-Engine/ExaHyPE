// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "config.h"

#include "peano4/datamanagement/CellMarker.h"
#include "tarch/multicore/Tasks.h"


#include <functional>


namespace exahype2 {
  class EnclaveTask;
  class EnclaveBookkeeping;
}



/**
 * Base class for all enclave tasks
 *
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
 * ## Data ownership
 *
 * - The run() operation allocates the output data and then invokes the functor.
 * - I assume that the functor frees the input data.
 * - The task allocated the output value within _outputValues itself before it
 *   invokes the actual task functor. This happens in computeTask(). We have the
 *   following sequence: run() calls computeTask() calls the user's functor. The
 *   size of the output field is specified in the constructor argument. The
 *   output will be on the heap and is eventually passed over to the enclave
 *   bookkeeping.
 *
 * ## SmartMPI
 * 
 * If you enable SmartMPI, then all enclave tasks are SmartMPI tasks, too. I could
 * realise this through multiple inheritance, but I have to know excactly how big
 * the memory footprint behind an enclave task is, so I decided against this.
 * Instead, subclasses of the enclave task do specialise w.r.t. SmartMPI.
 */
class exahype2::EnclaveTask: public tarch::multicore::Task {
  public:
    typedef std::function< void() >  Functor;

  protected:
    friend class EnclaveBookkeeping;

    static tarch::logging::Log                   _log;

    /**
     * Each task needs a unique number, so we can look up its output.
     */
    const int                                    _taskNumber;
    const ::peano4::datamanagement::CellMarker   _marker;

    /**
     * These are the reconstructed values in the Finite Volume sense and the
     * linear combination in the DG solver. In both cases, the enclave task
     * will free the memory once it has terminated, so the attribute may not
     * be const here.
     */
    double* __restrict__                         _inputValues;
    double* __restrict__                         _outputValues;
    const double                                 _t;
    const double                                 _dt;
    int                                          _numberOfInputValues;
    int                                          _numberOfResultValues;
    Functor                                      _functor;
    double                                       _maxEigenvalue;
  public:
    /**
     * Create plain enclave task.
     *
     * @param inputValues        Has to be created on heap via tarch::multicore::allocateMemory().
     * @param enclaveTaskTypeId  Please create it through 
     * <pre>
    static int enclaveTaskTypeId = peano4::parallel::Tasks::getTaskType("{{SOLVER_INSTANCE}}");
</pre>    
     *
     * @see _inputValues for a discussion of const qualifiers
     */
    EnclaveTask(
      int                                            enclaveTaskTypeId,
      const ::peano4::datamanagement::CellMarker&    marker,
      double                                         t,
      double                                         dt,
      double* __restrict__                           inputValues,
      int                                            numberOfInputValues,
      int                                            numberOfResultValues,
      Functor                                        functor
    );

    EnclaveTask(const EnclaveTask& other) = delete;
    EnclaveTask(const EnclaveTask&& other) = delete;

    virtual ~EnclaveTask() = default;

    /**
     * Computes a task and bookmarks the outcome
     */
    bool run() override;

    /**
     * Computes a task without bookmarking the outcome.
     * It is important for tasks that implement smartmpi::Task
     * that outcomes can be bookmarked independently of the
     * running of a task, hence why this function lives
     * alongside and is used by run()
     */
    void computeTask();

    /**
     * Return _taskNumber.
     */
    int getTaskId() const;
};



