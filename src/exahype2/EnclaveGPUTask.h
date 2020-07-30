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
 * An enclave task which runs on a GPU
 *
 * Please study the enclave task before. This routine differs in a few ways
 *
 * - I assume that the functor internally invokes a GPU kernel
 * - Therefore, I do not create the output on the CPU but on the accelerator
 * - After the kernel has terminated, I copy the result from the accelerator
 *   data into a local buffer which I hand over to the bookkeeping.
 *
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
    const bool                                   _inputDataCreatedOnDevice;

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
      std::function< void(double* input, double* output, const ::peano4::datamanagement::CellMarker& marker) >                        functor,
	  bool                                           inputDataCreatedOnDevice = true
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

    /**
     * Run the actual functor
     *
     * This operation consists of two big items: The kernel invocation (compute
     * phase) and the clean-up/bookkeeping step.
     *
     * <h2> Asynchronous kernels </h2>
     *
     * Asynchronous kernel invocation is implemented via OpenMP events and
     * subtasks. The pattern is inspired by Michael Klemm's talk with the
     * subtitle "or: Pretty Cool & New OpenMP Stuff). Maybe a better example
     * is however the OpenMP standard
     *
     * https://www.openmp.org/wp-content/uploads/openmp-examples-4.5.0.pdf
     *
     *
     *
     * Here's the steps:
     *
     * We create an instance of omp_event_t called gpu_event within run()
     * - Wrap the functor execution within
     *
     *
     * <h2> Bookkeeping </h2>
     *
     * - Delete the input data. We know that the compute kernel has terminated,
     *   so it is safe to kill this memory region. We only have to take into
     *   account whether this has been allocated on the host or the device.
     * - Create memory on the heap on the host and copy the content of
     *   _outputValues (on the accelerator) over to this heap buffer.
     * - Free _outputValues on the device.
     * - Hand the newly created heap chunk on the host over to the
     *   bookkeeping.
     */
    bool run() override;

    /**
     * nop
     */
    void prefetch() override;
};



#endif
