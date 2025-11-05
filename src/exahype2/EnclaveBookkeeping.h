// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/logging/Log.h"
#include "tarch/multicore/BooleanSemaphore.h"


#include "peano4/datamanagement/CellMarker.h"

#include <set>
#include <map>
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
      Entry( const Entry& copy ) = default;

      /**
       * We do not delete data. This is done externally.
       */
      ~Entry() = default;
      std::string toString() const;
    };
  private:
    static tarch::logging::Log          _log;

    static const std::string MemoryAllocationsInLookupTableIdentifier;

    using FinishedTaskMap = std::unordered_map<int, Entry >;

    /**
     * Plain map onto ouput array. See lifecycle discussion of EnclaveTask
     * for details.
     */
    FinishedTaskMap   _finishedTasks;

    #if PeanoDebug>0
    /**
     * Only used for debugging
     */
    std::map<int, ::peano4::datamanagement::CellMarker >       _taskToMarkerMapping;
    #endif

    std::set<int>  _tasksThatHaveToBeCancelled;

    tarch::multicore::BooleanSemaphore  _finishedTasksSemaphore;

    EnclaveBookkeeping() = default;
    EnclaveBookkeeping(const EnclaveBookkeeping&) = delete;

    /**
     * Wait for a task result to become available
     *
     * We wait for the task to terminate, and then return the meta object
     * which describes the task properties. It is up to the calling routine to
     * decide what to do with the result. We might either return the data or
     * throw it away.
     *
     * The routine assumes that a task has dropped its results due to
     * finishedTask() already. To ensure this really happens, we call
     * tarch::multicore::waitForTask() at the start of the routine's body.
     *
     * ## Validation
     *
     * All the validation steps are realised through non-critical
     *
     * @return Entry with all meta data plus a pointer to the actual
     *   task result.
     * @param x This marker is only there for debug reasons. In release
     *   mode, we ignore it.
     * @param h This marker is only there for debug reasons. In release
     *   mode, we ignore it.
     *
     */
    Entry waitForTaskToTerminateAndReturnResult(
      int number,
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h
    );
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
     *
     * @param x This marker is only there for debug reasons. In release
     *   mode, we ignore it.
     * @param h This marker is only there for debug reasons. In release
     *   mode, we ignore it.
     */
    void waitForTaskToTerminateAndCopyResultOver(
      int number,
      double* destination,
      double& maxEigenvalue,
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h
    );

    /**
     * Wait for a task and copy outcome into destination
     *
     * @param x This marker is only there for debug reasons. In release
     *   mode, we ignore it.
     * @param h This marker is only there for debug reasons. In release
     *   mode, we ignore it.
     */
    void waitForTaskAndDiscardResult(
      int number,
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h
    );

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
     *
     * @param x This marker is only there for debug reasons. In release
     *   mode, we ignore it.
     * @param h This marker is only there for debug reasons. In release
     *   mode, we ignore it.
     */
    void finishedTask(
      int taskNumber,
      int numberOfResultValues,
      double* data,
      double maxEigenvalue,
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h
    );

    /**
     * This one is only there for debugging purposes
     *
     * The routine is called by the constructor of EnclaveTask to notify the
     * bookkeepting that a new task has been created with a certain task number
     * for a particular cell.
     */
    void registerTask( int taskNumber, const ::peano4::datamanagement::CellMarker& marker );

};

