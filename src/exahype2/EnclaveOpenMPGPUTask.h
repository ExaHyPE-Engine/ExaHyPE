// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_ENCLAVE_OPENMP_GPU_TASK_H_
#define _EXAHYPE2_ENCLAVE_OPENMP_GPU_TASK_H_


#include "peano4/datamanagement/CellMarker.h"
#include "tarch/multicore/Tasks.h"


#include <functional>


namespace exahype2 {
  class EnclaveOpenMPGPUTask;
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
class exahype2::EnclaveOpenMPGPUTask: public tarch::multicore::Task {
  public:
	/**
	 * This functor represents what happens in a cell. It accepts the input
	 * data which is passed into the enclave task, and it is given also the
	 * output data which is a local array the enclave task creates. The latter
	 * will eventually be bookkept once the enclave task has terminated, though
	 * this particular implementation can create output as a device array. In
	 * this case, it will copy output over into a host memory region which is
	 * then given the bookkeeping.
	 *
	 * We had quite some issues with declaring the marker as GPU offloadable.
	 * In principle, it should not be a major issue to get it there, but the
	 * marker has some routines which extract marker properties from other
	 * Peano classes. To port them or to extract them would be cumbersome. As
	 * a consequence, the functor might have to copy elements from the marker
	 * manually into scalars before it enters a target region.
	 *
	 * The functor is responsible to access the target! See run() for requirements.
	 */
	typedef std::function< void(double* input, double* output, const ::peano4::datamanagement::CellMarker& marker) >  Functor;

    /**
     * Construct a task that can run on the GPU
     *
     * But it doesn't have to to run there. It is up to the task to decide whether
     * to offload or not.
     */
    EnclaveOpenMPGPUTask(
      const ::peano4::datamanagement::CellMarker&    marker,
      double*                                        inputValues,
      int                                            numberOfResultValues,
	  Functor                                        functor,
	  bool                                           inputDataCreatedOnDevice = true
    );

    EnclaveOpenMPGPUTask(const EnclaveOpenMPGPUTask& other) = delete;
    EnclaveOpenMPGPUTask(const EnclaveOpenMPGPUTask&& other) = delete;

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
     * See Section 4.7.1.
     *
     * The important thing is that your task has to have the clause
     *
     * #pragma omp task depend(out:dependencyMarker)
     *
     * If you wanna further make it asynchronous, please add a nowait to your target
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

  private:
    friend class EnclaveBookkeeping;

    static tarch::logging::Log                   _log;

    const ::peano4::datamanagement::CellMarker   _marker;
    double*                                      _inputValues;
    double*                                      _outputValues;
    int                                          _numberOfResultValues;
    Functor                                      _functor;
    const bool                                   _inputDataCreatedOnDevice;

};



#endif
