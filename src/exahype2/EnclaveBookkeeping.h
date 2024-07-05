// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/logging/Log.h"
#include "tarch/multicore/BooleanSemaphore.h"


#include <set>
#include <unordered_map>
#include <string>


namespace exahype2 {
  class EnclaveBookkeeping;
  class EnclaveTask;
}


/**
 * Enclave bookkeeping
 *
 * The enclave bookkeeping is basically a big map which stores results of 
 * enclave tasks. Most of the documentation around enclave tasks can be
 * found in exahype2::EnclaveTask.
 * 
 * The bookkeeping can be used in two ways by the underlying enclave task
 * object: The enclave tasks can allocate data for their results on the heap,
 * hand that heap object over to the bookkeeping, and the bookkeeping then
 * hands out the results to the actual solver by copying it into the right
 * place in the mesh and subsequently freeing the tasks's output.
 *
 * Alternatively, we can work directly on the heap, i.e. the mesh tells us
 * where the result should end up. In our tests, it is not always clear
 * which variant is faster. Which is kind of a surprise. Likely it has to
 * do with memory/cache issues.
 */
class exahype2::EnclaveBookkeeping {
  public:
    struct Entry {
      int      numberOfResultValues;
      double*  data;
      double   maxEigenvalue;

      Entry( int  numberOfResultValues_, double*  data_, double   maxEigenvalue_ );
      std::string toString() const;
    };
  private:
    static tarch::logging::Log          _log;

    const static std::string MemoryAllocationsInLookupTableIdentifier;

    /**
     * Plain map onto ouput array. See lifecycle discussion of EnclaveTask
     * for details.
     */
    std::unordered_map<int, Entry >       _finishedTasks;
    
    std::set<int>  _tasksThatHaveToBeCancelled;

    tarch::multicore::BooleanSemaphore  _finishedTasksSemaphore;

    EnclaveBookkeeping() = default;

    /**
     * Wait for a task result to become available
     *
     * We wait for the task to terminate, and then return the meta object
     * which describes the task properties.
     *
     * @return Entry with all meta data plus a pointer to the actual
     *   task result.
     */
    Entry waitForTaskToTerminateAndReturnResult(int number);
  public:
    static constexpr int NoEnclaveTaskNumber = -1;
    /**
     * Skeletons are not really tasks in the traditional sense. But we can
     * see them as tasks which are immediately ran.
     */
    static constexpr int SkeletonTask        = -2;

    static EnclaveBookkeeping& getInstance();

    /**
     * For debugging only. This routine is not thread-safe.
     */
    void dumpStatistics();

    /**
     * Wait for a task and copy outcome into destination
     *
     * We use waitForTaskToTerminateAndReturnResult() to obtain a handle to
     * the task outcome. Once we get it, we have to check what to do next:
     *
     * If an enclave task had been given the memory location where to dump
     * the result directly, we simply return. In this case, the task outcome
     * pointer and destination point to the same address.
     *
     * If the task had allocated its own output data area on the heap,
     * we copy the content from there into destination and then delete the
     * task's memory location via
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * tarch::freeMemory( result, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory );
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     */
    void waitForTaskToTerminateAndCopyResultOver(int number, double* destination, double& maxEigenvalue);

    /**
     * Wait for a task and copy outcome into destination
     */
    void waitForTaskAndDiscardResult(int number);

    void cancelTask(int number);

    /**
     * Usually called directly by EnclaveTask.
     *
     * Once this routine is called, the ownership of data passes over into the
     * enclave bookkeeping. Basically, the enclave bookkeepting stores the
     * passed pointer into a map and returns.
     *
     * There is one exception to this: If we use optimistic time stepping, then
     * we might run into a situation where a task overwrites a previous result.
     * At the moment, this variant yields a warning, but in any case, the old
     * data is first released, before we store the new pointer.
     *
     * @param data Has to be device memory allocated through tarch::multicore::allocateMemory()
     */
    void finishedTask(int taskNumber, int numberOfResultValues, double* data, double maxEigenvalue);
};

