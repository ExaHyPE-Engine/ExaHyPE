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
 * enclave tasks.
 * 
 *
@todo Das Ergebnis muss lokal gepuffert werden im bookkeeping.
      Ich brauche zwei Adapter: einen fuer primaer, einen fuer sekundaer. Sonst rekonstruier ich dei Loesung jedes Mal (ist teuer)
      Die Adapter muessen alle das gleiche Bookkeeping verwenden -> singleton
      Ein Task muss sich, sobald er zuende ist, in den Bookkeeping einreihen, d.h. hier gibt es zwei Datenstrukturen: Semaphoren (set) + Tasks (map)
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
    const static std::string LookupMissesIdentifier;

    /**
     * Plain map onto ouput array. See lifecycle discussion of EnclaveTask
     * for details.
     */
    std::unordered_map<int, Entry >       _finishedTasks;
    
    std::set<int>  _tasksThatHaveToBeCancelled;

    tarch::multicore::BooleanSemaphore  _finishedTasksSemaphore;

    EnclaveBookkeeping() = default;
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
     */
    void waitForTaskToTerminateAndCopyResultOver(int number, double* destination, double& maxEigenvalue);

    /**
     * Wait for a task and copy outcome into destination
     */
    void waitForTaskAndDiscardResult(int number);
    
    /**
     * Wait for a task result to become available
     *
     * We check the internal database for the result. If the result is not
     * there yet, we process one task. This processing should be done LIFO,
     * as we assume that traversals produce tasks and the follow-up traversal
     * then consumes the outcome. In the Peano world, two subsequent mesh
     * traversals run through the grid in inverted order, so searching for
     * task outcomes LIFO makes sense. If there are no tasks left in Peano's
     * queues and we still haven't got the result, we yield to give other
     * threads of the runtime the opportunity to do their stuff.
     *
     *
     * You have the sole ownership of the result which should be deleted 
     * later via 
     * 
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * tarch::freeMemory( result, tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory );
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * @return Entry
     */
    Entry waitForTaskToTerminateAndReturnResult(int number);

    void cancelTask(int number);

    /**
     * Usually called directly by EnclaveTask.
     *
     * Once this routine is called, the ownership of data goes
     *
     * @param data Has to be device memory allocated through tarch::multicore::allocateMemory()
     */
    void finishedTask(int taskNumber, int numberOfResultValues, double* data, double maxEigenvalue);
};

